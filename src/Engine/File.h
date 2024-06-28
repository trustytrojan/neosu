#pragma once
#include "cbase.h"

class BaseFile;
class ConVar;

class File {
   public:
    static ConVar *debug;
    static ConVar *size_max;

    enum class TYPE { READ, WRITE };

   public:
    File(std::string filePath, TYPE type = TYPE::READ);
    virtual ~File();

    bool canRead() const;
    bool canWrite() const;

    void write(const u8 *buffer, size_t size);

    std::string readLine();
    std::string readString();
    const u8 *readFile();  // WARNING: this is NOT a null-terminated string! DO NOT USE THIS with UString/std::string!
    size_t getFileSize() const;

   private:
    BaseFile *m_file;
};

class BaseFile {
   public:
    virtual ~BaseFile() { ; }

    virtual bool canRead() const = 0;
    virtual bool canWrite() const = 0;

    virtual void write(const u8 *buffer, size_t size) = 0;

    virtual std::string readLine() = 0;
    virtual const u8 *readFile() = 0;
    virtual size_t getFileSize() const = 0;
};

// std implementation of File
class StdFile : public BaseFile {
   public:
    StdFile(std::string filePath, File::TYPE type);
    virtual ~StdFile();

    bool canRead() const;
    bool canWrite() const;

    void write(const u8 *buffer, size_t size);

    std::string readLine();
    const u8 *readFile();
    size_t getFileSize() const;

   private:
    std::string m_sFilePath;

    bool m_bReady;
    bool m_bRead;

    std::ifstream m_ifstream;
    std::ofstream m_ofstream;
    size_t m_iFileSize;

    // full reader
    std::vector<u8> m_fullBuffer;
};
