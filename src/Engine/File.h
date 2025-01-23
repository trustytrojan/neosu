#pragma once
#include "cbase.h"

class ConVar;

class File {
   public:
    enum class TYPE { READ, WRITE };

   public:
    File(std::string filePath, TYPE type = TYPE::READ);

    bool canRead() const;
    bool canWrite() const;

    void write(const u8 *buffer, size_t size);

    std::string readLine();
    std::string readString();
    const u8 *readFile();  // WARNING: this is NOT a null-terminated string! DO NOT USE THIS with UString/std::string!
    size_t getFileSize() const;

   private:
    std::string sFilePath;

    bool bReady;
    bool bRead;

    std::ifstream ifstream;
    std::ofstream ofstream;
    size_t iFileSize;

    // full reader
    std::vector<u8> fullBuffer;
};
