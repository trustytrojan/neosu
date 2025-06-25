#include "Archival.h"

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <cstring>
#include <fstream>

#include "Engine.h"
#include "UString.h"

//------------------------------------------------------------------------------
// Archive::Entry implementation
//------------------------------------------------------------------------------

Archive::Entry::Entry(struct archive* archive, struct archive_entry* entry) {
    if(!entry) {
        // create empty entry
        m_filename = "";
        m_uncompressedSize = 0;
        m_compressedSize = 0;
        m_isDirectory = false;
        return;
    }

    m_filename = archive_entry_pathname(entry);
    m_uncompressedSize = archive_entry_size(entry);
    m_compressedSize = 0;  // libarchive doesn't always provide compressed size
    m_isDirectory = archive_entry_filetype(entry) == AE_IFDIR;

    // extract data immediately while archive is positioned correctly
    if(!m_isDirectory && archive) {
        if(m_uncompressedSize > 0) {
            m_data.reserve(m_uncompressedSize);
        }

        const void* buff;
        size_t size;
        la_int64_t offset;

        while(archive_read_data_block(archive, &buff, &size, &offset) == ARCHIVE_OK) {
            const u8* bytes = static_cast<const u8*>(buff);
            m_data.insert(m_data.end(), bytes, bytes + size);
        }
    }
}

std::string Archive::Entry::getFilename() const { return m_filename; }

size_t Archive::Entry::getUncompressedSize() const { return m_uncompressedSize; }

size_t Archive::Entry::getCompressedSize() const { return m_compressedSize; }

bool Archive::Entry::isDirectory() const { return m_isDirectory; }

bool Archive::Entry::isFile() const { return !m_isDirectory; }

std::vector<u8> Archive::Entry::extractToMemory() const { return m_data; }

bool Archive::Entry::extractToFile(const std::string& outputPath) const {
    if(m_isDirectory) return false;

    std::ofstream file(outputPath, std::ios::binary);
    if(!file.good()) {
        debugLog("Archive: failed to create output file %s\n", outputPath.c_str());
        return false;
    }

    if(!m_data.empty()) {
        file.write(reinterpret_cast<const char*>(m_data.data()), static_cast<std::streamsize>(m_data.size()));
        if(file.bad()) {
            debugLog("Archive: failed to write to file %s\n", outputPath.c_str());
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
// Archive implementation
//------------------------------------------------------------------------------

Archive::Archive(const std::string& filePath) : m_archive(nullptr), m_valid(false), m_iterationStarted(false) {
    initFromFile(filePath);
}

Archive::Archive(const u8* data, size_t size) : m_archive(nullptr), m_valid(false), m_iterationStarted(false) {
    initFromMemory(data, size);
}

Archive::~Archive() { cleanup(); }

void Archive::initFromFile(const std::string& filePath) {
    m_archive = archive_read_new();
    if(!m_archive) {
        debugLog("Archive: failed to create archive reader\n");
        return;
    }

    // enable all supported formats and filters
    archive_read_support_format_all(m_archive);
    archive_read_support_filter_all(m_archive);

    int r = archive_read_open_filename(m_archive, filePath.c_str(), 10240);
    if(r != ARCHIVE_OK) {
        debugLog("Archive: failed to open file %s: %s\n", filePath.c_str(), archive_error_string(m_archive));
        cleanup();
        return;
    }

    m_valid = true;
}

void Archive::initFromMemory(const u8* data, size_t size) {
    if(!data || size == 0) {
        debugLog("Archive: invalid memory buffer\n");
        return;
    }

    // copy data to our own buffer to ensure it stays alive
    m_memoryBuffer.assign(data, data + size);

    m_archive = archive_read_new();
    if(!m_archive) {
        debugLog("Archive: failed to create archive reader\n");
        return;
    }

    // enable all supported formats and filters
    archive_read_support_format_all(m_archive);
    archive_read_support_filter_all(m_archive);

    int r = archive_read_open_memory(m_archive, m_memoryBuffer.data(), m_memoryBuffer.size());
    if(r != ARCHIVE_OK) {
        debugLog("Archive: failed to open memory buffer: %s\n", archive_error_string(m_archive));
        cleanup();
        return;
    }

    m_valid = true;
}

void Archive::cleanup() {
    m_currentEntry.reset();
    if(m_archive) {
        archive_read_free(m_archive);
        m_archive = nullptr;
    }
    m_valid = false;
    m_iterationStarted = false;
}

std::vector<Archive::Entry> Archive::getAllEntries() {
    std::vector<Entry> entries;
    if(!m_valid) return entries;

    // restart iteration if needed
    if(m_iterationStarted) {
        cleanup();
        if(!m_memoryBuffer.empty()) {
            initFromMemory(m_memoryBuffer.data(), m_memoryBuffer.size());
        } else {
            debugLog("Archive: cannot restart iteration on file-based archive\n");
            return entries;
        }
    }

    struct archive_entry* entry;
    while(archive_read_next_header(m_archive, &entry) == ARCHIVE_OK) {
        entries.emplace_back(m_archive, entry);
        // skip file data to move to next entry
        archive_read_data_skip(m_archive);
    }

    m_iterationStarted = true;
    return entries;
}

bool Archive::hasNext() {
    if(!m_valid) return false;

    if(!m_iterationStarted) {
        struct archive_entry* entry;
        int r = archive_read_next_header(m_archive, &entry);
        if(r == ARCHIVE_OK) {
            m_currentEntry = std::make_unique<Entry>(m_archive, entry);
            m_iterationStarted = true;
            return true;
        } else if(r == ARCHIVE_EOF) {
            return false;
        } else {
            debugLog("Archive: error reading next header: %s\n", archive_error_string(m_archive));
            return false;
        }
    }

    return m_currentEntry != nullptr;
}

Archive::Entry Archive::getCurrentEntry() {
    if(!hasNext()) {
        // return empty entry if none available
        struct archive_entry* dummy = archive_entry_new();
        Entry empty(nullptr, dummy);
        archive_entry_free(dummy);
        return empty;
    }

    return *m_currentEntry;
}

bool Archive::moveNext() {
    if(!m_valid) return false;

    // skip current entry data
    if(m_currentEntry) {
        archive_read_data_skip(m_archive);
        m_currentEntry.reset();
    }

    struct archive_entry* entry;
    int r = archive_read_next_header(m_archive, &entry);
    if(r == ARCHIVE_OK) {
        m_currentEntry = std::make_unique<Entry>(m_archive, entry);
        return true;
    } else if(r == ARCHIVE_EOF) {
        return false;
    } else {
        debugLog("Archive: error reading next header: %s\n", archive_error_string(m_archive));
        return false;
    }
}

Archive::Entry* Archive::findEntry(const std::string& filename) {
    auto entries = getAllEntries();

    auto it =
        std::ranges::find_if(entries, [&filename](const Entry& entry) { return entry.getFilename() == filename; });

    if(it != entries.end()) {
        // note: this returns a pointer to a temporary object
        // caller should use immediately or copy the entry
        static Entry found = *it;
        found = *it;
        return &found;
    }

    return nullptr;
}

bool Archive::extractAll(const std::string& outputDir, const std::vector<std::string>& ignorePaths,
                         bool skipDirectories) {
    if(!m_valid) return false;

    auto entries = getAllEntries();
    if(entries.empty()) return false;

    // separate directories and files
    std::vector<Entry> directories, files;
    for(const auto& entry : entries) {
        if(entry.isDirectory()) {
            directories.push_back(entry);
        } else {
            files.push_back(entry);
        }
    }

    // create directories first (unless skipping)
    if(!skipDirectories) {
        for(const auto& dir : directories) {
            std::string dirPath = outputDir + "/" + dir.getFilename();

            // check ignore list
            bool shouldIgnore = std::ranges::any_of(ignorePaths, [&dirPath](const std::string& ignorePath) {
                return dirPath.find(ignorePath) != std::string::npos;
            });

            if(shouldIgnore) continue;

            if(!isPathSafe(dir.getFilename())) {
                debugLog("Archive: skipping unsafe directory path %s\n", dir.getFilename().c_str());
                continue;
            }

            createDirectoryRecursive(dirPath);
        }
    }

    // extract files
    for(const auto& file : files) {
        std::string filePath = outputDir + "/" + file.getFilename();

        // check ignore list
        bool shouldIgnore = std::ranges::any_of(ignorePaths, [&filePath](const std::string& ignorePath) {
            return filePath.find(ignorePath) != std::string::npos;
        });

        if(shouldIgnore) {
            debugLog("Archive: ignoring file %s\n", filePath.c_str());
            continue;
        }

        if(!isPathSafe(file.getFilename())) {
            debugLog("Archive: skipping unsafe file path %s\n", file.getFilename().c_str());
            continue;
        }

        // ensure parent directory exists
        auto folders = UString(file.getFilename()).split("/");
        std::string currentPath = outputDir;
        for(size_t i = 0; i < folders.size() - 1; i++) {
            currentPath = UString::fmt("{}/{}", currentPath, folders[i]).toUtf8();
            createDirectoryRecursive(currentPath);
        }

        if(!file.extractToFile(filePath)) {
            debugLog("Archive: failed to extract file %s\n", filePath.c_str());
            return false;
        }
    }

    return true;
}

bool Archive::createDirectoryRecursive(const std::string& path) {
    if(env->directoryExists(path)) return true;

    return env->createDirectory(path);
}

bool Archive::isPathSafe(const std::string& path) {
    // check for path traversal attempts
    return path.find("..") == std::string::npos;
}
