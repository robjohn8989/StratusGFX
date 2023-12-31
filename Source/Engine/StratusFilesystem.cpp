#include <fstream>
#include <sstream>
#include <iostream>

#include "StratusFilesystem.h"
#include "StratusLog.h"

namespace stratus {
/**
 * @see http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
 */
std::vector <char> Filesystem::ReadBinary(const std::string &file) {
    std::ifstream binaryFile(file, std::ios::in | std::ios::binary);
    if (!binaryFile.is_open()) {
        STRATUS_ERROR << "[error] Unable to open " << file << std::endl;
        return std::vector<char>();
    }

    // First we need to get the size
    int64_t fileSize;
    binaryFile.seekg(0, std::ios::end);
    fileSize = binaryFile.tellg();
    binaryFile.seekg(0, std::ios::beg);

    // Create a buffer with the right size
    std::vector<char> buffer(static_cast<uint64_t>(fileSize));
    binaryFile.read(&buffer[0], buffer.size());

    // Close the file and return
    binaryFile.close();
    return buffer;
}

/**
 * @see https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
 */
std::string Filesystem::ReadAscii(const std::string &file) {
    std::ifstream asciiFile(file, std::ios::in);
    if (!asciiFile.is_open()) {
        STRATUS_ERROR << "[error] Unable to open " << file << std::endl;
        return std::string();
    }
    auto s = std::string((std::istreambuf_iterator<char>(asciiFile)),
            std::istreambuf_iterator<char>());
    asciiFile.close();
    return s;
}

std::filesystem::path Filesystem::CurrentPath() {
    return std::filesystem::current_path();
}
}