#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <zlib.h>

#define CHUNK 16384

int main(int argc, char* argv[]) {
    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!\n";
    
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
        if (argc < 4 || argv[3] != "-p") {
            std::cerr << "-p argument missing.\n";
        }
        try {
            std::string blob_sha = argv[4];
            std::ifstream blob_file(".git/objects/" + blob_sha.insert(2, "/"), std::ios::binary);
            if (!blob_file.is_open()) {
                std::cerr << "Failed to find blob.\n";
                return EXIT_FAILURE;
            }
            int ret, flush;
            unsigned have;
            z_stream strm;
            unsigned char in[CHUNK];
            unsigned char out[CHUNK];
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = 0;
            strm.next_in = Z_NULL;
            ret = inflateInit(&strm);
            if (ret != Z_OK)
                return ret;
            
            while (blob_file.read(in, CHUNK)) {
                do {
                    strm.avail_in = fread(in, 1, CHUNK, source);
                    if (ferror(source)) {
                        (void)inflateEnd(&strm);
                        return Z_ERRNO;
                    }
                    if (strm.avail_in == 0)
                        break;
                    strm.next_in = in;
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
                        std::cout.write(out, CHUNK);
                    } while (strm.avail_out == 0);
                } while (ret != Z_STREAM_END);
                (void)inflateEnd(&strm);
                return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
            }

        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }

        
    } else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
