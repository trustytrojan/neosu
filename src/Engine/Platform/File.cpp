#include "File.h"

#include <filesystem>

#include "ConVar.h"
#include "Engine.h"

File::File(std::string filePath, TYPE type) {
    m_sFilePath = filePath;
    m_bRead = (type == File::TYPE::READ);

    m_bReady = false;
    m_iFileSize = 0;

    if(m_bRead) {
        m_ifstream.open(std::filesystem::u8path(filePath.c_str()), std::ios::in | std::ios::binary);

        // check if we could open it at all
        if(!m_ifstream.good()) {
            debugLog("File Error: Couldn't open() file %s\n", filePath.c_str());
            return;
        }

        // get and check filesize
        m_ifstream.seekg(0, std::ios::end);
        m_iFileSize = m_ifstream.tellg();
        m_ifstream.seekg(0, std::ios::beg);

        if(m_iFileSize < 1) {
            debugLog("File Error: FileSize is < 0\n");
            return;
        } else if(m_iFileSize > 1024 * 1024 * cv_file_size_max.getInt())  // size sanity check
        {
            debugLog("File Error: FileSize is > %i MB!!!\n", cv_file_size_max.getInt());
            return;
        }

        // check if directory
        // on some operating systems, std::ifstream may also be a directory (!)
        // we need to call getline() before any error/fail bits are set, which we can then check with good()
        std::string tempLine;
        std::getline(m_ifstream, tempLine);
        if(!m_ifstream.good() && m_iFileSize < 1) {
            debugLog("File Error: File %s is a directory.\n", filePath.c_str());
            return;
        }
        m_ifstream.clear();  // clear potential error state due to the check above
        m_ifstream.seekg(0, std::ios::beg);
    } else {  // WRITE
        m_ofstream.open(std::filesystem::u8path(filePath.c_str()), std::ios::out | std::ios::trunc | std::ios::binary);

        // check if we could open it at all
        if(!m_ofstream.good()) {
            debugLog("File Error: Couldn't open() file %s\n", filePath.c_str());
            return;
        }
    }

    if(cv_debug.getBool()) debugLog("File: Opening %s\n", filePath.c_str());

    m_bReady = true;
}

std::string File::readString() {
    const int size = getFileSize();
    if(size < 1) return std::string();

    return std::string((const char *)readFile(), size);
}

bool File::canRead() const { return m_bReady && m_ifstream.good() && m_bRead; }

bool File::canWrite() const { return m_bReady && m_ofstream.good() && !m_bRead; }

void File::write(const u8 *buffer, size_t size) {
    if(!canWrite()) return;

    m_ofstream.write((const char *)buffer, size);
}

std::string File::readLine() {
    if(!canRead()) return "";

    std::string line;
    m_bRead = (bool)std::getline(m_ifstream, line);

    // std::getline keeps line delimiters in the output for some reason
    if(!line.empty() && line.back() == '\n') line.pop_back();
    if(!line.empty() && line.back() == '\r') line.pop_back();

    return line;
}

const u8 *File::readFile() {
    if(cv_debug.getBool()) debugLog("File::readFile() on %s\n", m_sFilePath.c_str());

    if(m_fullBuffer.size() > 0) return &m_fullBuffer[0];

    if(!m_bReady || !canRead()) return NULL;

    m_fullBuffer.resize(m_iFileSize);
    return (m_ifstream.read((char *)m_fullBuffer.data(), m_iFileSize) ? &m_fullBuffer[0] : NULL);
}

size_t File::getFileSize() const { return m_iFileSize; }
