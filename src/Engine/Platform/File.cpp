#include "File.h"

#include <filesystem>

#include "ConVar.h"
#include "Engine.h"

File::File(std::string filePath, TYPE type) {
    this->sFilePath = filePath;
    this->bRead = (type == File::TYPE::READ);

    this->bReady = false;
    this->iFileSize = 0;

    if(this->bRead) {
        this->ifstream.open(std::filesystem::u8path(filePath.c_str()), std::ios::in | std::ios::binary);

        // check if we could open it at all
        if(!this->ifstream.good()) {
            debugLog("File Error: Couldn't open() file %s\n", filePath.c_str());
            return;
        }

        // get and check filesize
        this->ifstream.seekg(0, std::ios::end);
        this->iFileSize = this->ifstream.tellg();
        this->ifstream.seekg(0, std::ios::beg);

        if(this->iFileSize < 1) {
            debugLog("File Error: FileSize is < 0\n");
            return;
        } else if(this->iFileSize > 1024 * 1024 * cv_file_size_max.getInt())  // size sanity check
        {
            debugLog("File Error: FileSize is > %i MB!!!\n", cv_file_size_max.getInt());
            return;
        }

        // check if directory
        // on some operating systems, std::ifstream may also be a directory (!)
        // we need to call getline() before any error/fail bits are set, which we can then check with good()
        std::string tempLine;
        std::getline(this->ifstream, tempLine);
        if(!this->ifstream.good() && this->iFileSize < 1) {
            debugLog("File Error: File %s is a directory.\n", filePath.c_str());
            return;
        }
        this->ifstream.clear();  // clear potential error state due to the check above
        this->ifstream.seekg(0, std::ios::beg);
    } else {  // WRITE
        this->ofstream.open(std::filesystem::u8path(filePath.c_str()),
                            std::ios::out | std::ios::trunc | std::ios::binary);

        // check if we could open it at all
        if(!this->ofstream.good()) {
            debugLog("File Error: Couldn't open() file %s\n", filePath.c_str());
            return;
        }
    }

    if(cv_debug.getBool()) debugLog("File: Opening %s\n", filePath.c_str());

    this->bReady = true;
}

std::string File::readString() {
    const int size = this->getFileSize();
    if(size < 1) return std::string();

    return std::string((const char *)this->readFile(), size);
}

bool File::canRead() const { return this->bReady && this->ifstream.good() && this->bRead; }

bool File::canWrite() const { return this->bReady && this->ofstream.good() && !this->bRead; }

void File::write(const u8 *buffer, size_t size) {
    if(!this->canWrite()) return;

    this->ofstream.write((const char *)buffer, size);
}

std::string File::readLine() {
    if(!this->canRead()) return "";

    std::string line;
    this->bRead = (bool)std::getline(this->ifstream, line);

    // std::getline keeps line delimiters in the output for some reason
    if(!line.empty() && line.back() == '\n') line.pop_back();
    if(!line.empty() && line.back() == '\r') line.pop_back();

    return line;
}

const u8 *File::readFile() {
    if(cv_debug.getBool()) debugLog("File::readFile() on %s\n", this->sFilePath.c_str());

    if(this->fullBuffer.size() > 0) return &this->fullBuffer[0];

    if(!this->bReady || !this->canRead()) return NULL;

    this->fullBuffer.resize(this->iFileSize);
    return (this->ifstream.read((char *)this->fullBuffer.data(), this->iFileSize) ? &this->fullBuffer[0] : NULL);
}

size_t File::getFileSize() const { return this->iFileSize; }
