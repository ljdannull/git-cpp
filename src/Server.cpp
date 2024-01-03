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
#include <vector>
#include <algorithm>
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
        inf(blob_file, customStdout, 1);
        fclose(blob_file);
        fclose(customStdout);

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }  
    return EXIT_SUCCESS;  
}

std::string hashobject(char* filepath) {
    std::ifstream inputFile(filepath);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return NULL;
    }

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
    std::string destpath = folder + std::string({hash + 2, 39});
    if (! std::filesystem::exists(destpath)) {
        FILE* dest = fopen(destpath.c_str(), "w");
        def(source, dest, COMPRESSIONLEVEL);
        fclose(dest);
    }

    inputFile.close();
    fclose(source);
    

    return hash;
}

int lstree(char* tree_sha) {
    std::string path = ".git/objects/";
    path += std::string(tree_sha, strlen(tree_sha)).insert(2, "/");
    
    FILE* source = fopen(path.c_str(), "r");
    FILE* dest = fopen(".git/objects/tmp", "w");
    
    inf(source, dest, 0);
    fseek(dest, 0L, SEEK_SET);
    fclose(source);
    fclose(dest);

    std::ifstream input(".git/objects/tmp", std::ios::binary );
    std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});
    input.close();
    std::string str(buffer.begin(), buffer.end());
    str = str.substr(str.find('\0') + 1);
    std::vector<char> new_buffer;
    int skip = 0;

    // get rid of the hash digests
    for (char c : str) {
        if (skip) {
            skip = std::max(skip - 1, 0);
            continue;
        }
        if ((int)c == 0) {
            skip = 20;
        }
        new_buffer.push_back(c);
    }
    std::string newstr(new_buffer.begin(), new_buffer.end());
    std::istringstream iss(newstr);
    std::string s;

    std::vector<std::string> names;
    int space;
    while (std::getline(iss, s, '\0')) {
        space = s.find(' ');
        names.push_back(s.substr(space + 1));
        
    }
    std::sort(names.begin(), names.end());
    for (std::string i : names) {
        std::cout << i << "\n";
    }
    
    std::remove(".git/objects/tmp");

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
            std::cerr << "Incorrect arguments.\n";
        }
        return catfile(argv[3]);
    } else if (command == "hash-object") {
        if (argc != 4 || strcmp(argv[2], "-w")) {
            std::cerr << "Incorrect arguments.\n";
        }
        std::cout <<  hashobject(argv[3]) << "\n";
    } else if (command == "ls-tree") {
        if (argc != 4 || strcmp(argv[2], "--name-only")) {
            std::cerr << "Incorrect arguments.\n";
        }
        return lstree(argv[3]);
    } else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

