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
#include "zpipe.cpp"


int gitinit() {
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
    return EXIT_SUCCESS;
}

int catfile(char* filepath) {
    try {
        std::string blob_sha = filepath;
        FILE* blob_file = fopen((".git/objects/" + blob_sha.insert(2, "/")).c_str(), "r");
        FILE* customStdout = fdopen(1, "w");
        inf(blob_file, customStdout);
        fclose(blob_file);
        fclose(customStdout);

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }  
    return EXIT_SUCCESS;  
}

int hashobject(char* filepath) {
    std::ifstream inputFile(filepath);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << filepath << std::endl;
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

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    // You can use print statements as follows for debugging, they'll be visible when running tests.
    
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    std::string command = argv[1];
    
    if (command == "init") {
       return gitinit();
    }  else if (command == "cat-file") {
        if (argc != 4 || strcmp(argv[2], "-p")) {
            std::cerr << "-p argument missing.\n";
        }
        return catfile(argv[3]);
    } else if (command == "hash-object") {
        if (argc != 4 || strcmp(argv[2], "-w")) {
            std::cerr << "-w argument missing.\n";
        }
        return hashobject(argv[3]);
    } else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

