//========== Copyright (c) 2016, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		file wrapper, for cross-platform unicode path support
//
// $NoKeywords: $file
//===============================================================================//

#pragma once
#ifndef FILE_H
#define FILE_H

#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

class ConVar;
class DirectoryCache;

class File {
   public:
    enum class TYPE : uint8_t { READ, WRITE };

    enum class FILETYPE : uint8_t { NONE, FILE, FOLDER, MAYBE_INSENSITIVE, OTHER };

   public:
    File(std::string filePath, TYPE type = TYPE::READ);
    ~File() = default;

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;
    File(const File &) = delete;
    File(File &&) = delete;

    [[nodiscard]] constexpr bool canRead() const {
        return m_ready && m_ifstream && m_ifstream->good() && m_type == TYPE::READ;
    }
    [[nodiscard]] constexpr bool canWrite() const {
        return m_ready && m_ofstream && m_ofstream->good() && m_type == TYPE::WRITE;
    }

    void write(const char *buffer, size_t size);
    bool writeLine(const std::string &line, bool insertNewline = true);

    std::string readLine();
    std::string readString();
    // WARNING: this is NOT a null-terminated string! DO NOT USE THIS with UString/std::string!
    const char *readFile();

    [[nodiscard]] constexpr size_t getFileSize() const { return m_fileSize; }
    [[nodiscard]] inline std::string getPath() const { return m_filePath; }

    // public path resolution methods
    // modifies the input path with the actual found path
    [[nodiscard]] static File::FILETYPE existsCaseInsensitive(std::string &filePath);
    [[nodiscard]] static File::FILETYPE exists(const std::string &filePath);

   private:
    std::string m_filePath;
    TYPE m_type;
    bool m_ready;
    size_t m_fileSize;

    // file streams
    std::unique_ptr<std::ifstream> m_ifstream;
    std::unique_ptr<std::ofstream> m_ofstream;

    // buffer for full file reading
    std::vector<char> m_fullBuffer;

    // private implementation helpers
    bool openForReading();
    bool openForWriting();

    // internal path resolution helpers
    [[nodiscard]] static File::FILETYPE existsCaseInsensitive(std::string &filePath, std::filesystem::path &path);
    [[nodiscard]] static File::FILETYPE exists(const std::string &filePath, const std::filesystem::path &path);

    // directory cache
    static std::unique_ptr<DirectoryCache> s_directoryCache;
};

#endif
