#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <zlib.h>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <openssl/sha.h>
#include <filesystem>


#define CHUNK 16384
#define COMPRESSIONLEVEL 3

// Adapted from zpipe.c written by Mark Adler
int def(FILE *source, FILE *dest, int level)
{
    
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

// Adapted from zpipe.c written by Mark Adler
int inf(FILE *source, FILE *dest)
{
    char type[8];
    int size;
    int header = 1;
    int headerlen = 0;

    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (header) {
                sscanf((char*) (&out[0]), "%s %d\0", type, &size);
                header = 0;
                headerlen = strlen(type) + std::to_string(size).length() + 2;
            }
            if (fwrite(out + headerlen, 1, have - headerlen, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
            headerlen = 0;
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int main(int argc, char* argv[]) {
    // You can use print statements as follows for debugging, they'll be visible when running tests.
    
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    std::string command = argv[1];
    
    if (command == "init") {
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");
    
            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) {
                headFile << "ref: refs/heads/master\n";
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
           
            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }  else if (command == "cat-file") {
        if (argc != 4 || strcmp(argv[2], "-p")) {
            std::cerr << "-p argument missing.\n";
        }
        try {
            std::string blob_sha = argv[3];
            FILE* blob_file = fopen((".git/objects/" + blob_sha.insert(2, "/")).c_str(), "r");
            FILE* customStdout = fdopen(1, "w");
            inf(blob_file, customStdout);
            fclose(blob_file);
            fclose(customStdout);

        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }     
    } else if (command == "hash-object") {
        if (argc != 4 || strcmp(argv[2], "-w")) {
            std::cerr << "-w argument missing.\n";
        }
        std::ifstream inputFile(argv[3]);
        if (!inputFile.is_open()) {
            std::cerr << "Error opening file: " << argv[3] << std::endl;
            return EXIT_FAILURE;
        }

        // Loading the file into RAM is not ideal since it won't work for larger files
        std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
        int file_size = fileContent.length();
        std::string header = "blob " + std::to_string(file_size) + '\0';
        std::string fileContentHeader = header + fileContent;
        unsigned char digest[20];
        SHA1((const unsigned char*)fileContentHeader.c_str(), fileContentHeader.length(), digest);
        char hash[41];
        for (int i = 0; i < 20; i++) {
            sprintf(hash + i * 2, "%02x", (unsigned char)digest[i]);
        }
        hash[40] = '\0';

        FILE* source = fmemopen((void *)fileContentHeader.c_str(), fileContentHeader.length(), "r");
        std::string folder(hash, 2);
        folder = ".git/objects/" + folder + '/';
        if (!std::filesystem::exists(folder)) {
            std::filesystem::create_directories(folder);
        }
        std::string tmp = hash;
        FILE* dest = fopen((folder + std::string({hash + 2, 39})).c_str(), "w");
        def(source, dest, COMPRESSIONLEVEL);

        std::cout << hash << "\n";;

        inputFile.close();
        fclose(source);
        fclose(dest);
    } else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

