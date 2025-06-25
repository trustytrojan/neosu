#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types.h"

struct archive;
struct archive_entry;

class Archive {
   public:
    class Entry {
       public:
        Entry(struct archive* archive, struct archive_entry* entry);
        ~Entry() = default;

        // entry information
        [[nodiscard]] std::string getFilename() const;
        [[nodiscard]] size_t getUncompressedSize() const;
        [[nodiscard]] size_t getCompressedSize() const;
        [[nodiscard]] bool isDirectory() const;
        [[nodiscard]] bool isFile() const;

        // extraction methods
        [[nodiscard]] std::vector<u8> extractToMemory() const;
        [[nodiscard]] bool extractToFile(const std::string& outputPath) const;

       private:
        struct archive* m_archive;
        struct archive_entry* m_entry;
        std::string m_filename;
        size_t m_uncompressedSize;
        size_t m_compressedSize;
        bool m_isDirectory;
    };

   public:
    // construct from file path
    explicit Archive(const std::string& filePath);

    // construct from memory buffer
    Archive(const u8* data, size_t size);

    ~Archive();

    // no copy/move for simplicity
    Archive(const Archive&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive(Archive&&) = delete;
    Archive& operator=(Archive&&) = delete;

    // check if archive was opened successfully
    [[nodiscard]] bool isValid() const { return m_valid; }

    // get all entries at once (useful for separating files/dirs)
    std::vector<Entry> getAllEntries();

    // iteration interface
    bool hasNext();
    Entry getCurrentEntry();
    bool moveNext();

    // convenience methods
    Entry* findEntry(const std::string& filename);
    bool extractAll(const std::string& outputDir, const std::vector<std::string>& ignorePaths = {},
                    bool skipDirectories = false);

   private:
    void initFromMemory(const u8* data, size_t size);
    void initFromFile(const std::string& filePath);
    void cleanup();
    bool createDirectoryRecursive(const std::string& path);
    bool isPathSafe(const std::string& path);

    struct archive* m_archive;
    std::vector<u8> m_memoryBuffer;  // keep buffer alive for memory-based archives
    bool m_valid;
    bool m_iterationStarted;
    std::unique_ptr<Entry> m_currentEntry;
};
