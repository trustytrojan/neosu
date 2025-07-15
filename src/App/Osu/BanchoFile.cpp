#include "BanchoFile.h"

#include "Engine.h"

BanchoFile::Reader::Reader(const char *path) {
    this->file = fopen(path, "rb");
    if(this->file == NULL) {
        this->set_error("Failed to open file for reading: " + std::string(strerror(errno)));
        debugLog("Failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    if(fseek(this->file, 0, SEEK_END)) {
        goto seek_error;
    }

    this->total_size = ftell(this->file);

    if(fseek(this->file, 0, SEEK_SET)) {
        goto seek_error;
    }

    this->buffer = (u8 *)malloc(READ_BUFFER_SIZE);

    if(this->buffer == NULL) {
        this->set_error("Failed to allocate read buffer");
        fclose(this->file);
        this->file = NULL;
        return;
    }

    return; // success

seek_error:
    this->set_error("Failed to initialize file reader: " + std::string(strerror(errno)));
    debugLog("Failed to initialize file reader '%s': %s\n", path, strerror(errno));
    fclose(this->file);
    this->file = NULL;
    return;
}

BanchoFile::Reader::~Reader() {
    if(this->file != NULL) {
        fclose(this->file);
    }
    if(this->buffer != NULL) {
        free(this->buffer);
    }
}

void BanchoFile::Reader::set_error(const std::string &error_msg) {
    if(!this->error_flag) {  // only set first error
        this->error_flag = true;
        this->last_error = error_msg;
    }
}

MD5Hash BanchoFile::Reader::read_hash() {
    MD5Hash hash;

    if(this->error_flag) {
        return hash;
    }

    u8 empty_check = this->read<u8>();
    if(empty_check == 0) return hash;

    u32 len = this->read_uleb128();
    u32 extra = 0;
    if(len > 32) {
        // just continue, don't set error flag
        debugLog("WARNING: Expected 32 bytes for hash, got %d!\n", len);
        extra = len - 32;
        len = 32;
    }

    if(this->read_bytes((u8 *)hash.hash.data(), len) != len) {
        // just continue, don't set error flag
        debugLog("WARNING: failed to read %d bytes to obtain hash.\n", len);
        extra = len;
    }
    this->skip_bytes(extra);
    hash.hash[len] = '\0';
    return hash;
}

std::string BanchoFile::Reader::read_string() {
    if(this->error_flag) {
        return {};
    }

    u8 empty_check = this->read<u8>();
    if(empty_check == 0) return {};

    u32 len = this->read_uleb128();
    u8 *str = new u8[len + 1];
    if(this->read_bytes(str, len) != len) {
        this->set_error("Failed to read " + std::to_string(len) + " bytes for string");
        delete[] str;
        return {};
    }

    std::string str_out((const char *)str, len);
    delete[] str;

    return str_out;
}

u32 BanchoFile::Reader::read_uleb128() {
    if(this->error_flag) {
        return 0;
    }

    u32 result = 0;
    u32 shift = 0;
    u8 byte = 0;

    do {
        byte = this->read<u8>();
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while(byte & 0x80);

    return result;
}

void BanchoFile::Reader::skip_string() {
    if(this->error_flag) {
        return;
    }

    u8 empty_check = this->read<u8>();
    if(empty_check == 0) return;

    u32 len = this->read_uleb128();
    this->skip_bytes(len);
}

BanchoFile::Writer::Writer(const char *path) {
    this->file_path = path;
    this->tmp_file_path = this->file_path;
    this->tmp_file_path.append(".tmp");

    this->file = fopen(this->tmp_file_path.c_str(), "wb");
    if(this->file == NULL) {
        this->set_error("Failed to open file for writing: " + std::string(strerror(errno)));
        debugLog("Failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    this->buffer = (u8 *)malloc(WRITE_BUFFER_SIZE);
    if(this->buffer == NULL) {
        this->set_error("Failed to allocate write buffer");
        fclose(this->file);
        this->file = NULL;
        return;
    }
}

BanchoFile::Writer::~Writer() {
    if(this->file != NULL) {
        this->flush();
        fclose(this->file);

        if(!this->error_flag) {
            env->deleteFile(this->file_path);  // Windows (the Microsoft docs are LYING)
            env->renameFile(this->tmp_file_path, this->file_path);
        }
    }

    if(this->buffer != NULL) {
        free(this->buffer);
    }
}

void BanchoFile::Writer::set_error(const std::string &error_msg) {
    if(!this->error_flag) {  // only set first error
        this->error_flag = true;
        this->last_error = error_msg;
    }
}

void BanchoFile::Writer::write_hash(MD5Hash hash) {
    if(this->error_flag) {
        return;
    }

    this->write<u8>(0x0B);
    this->write<u8>(0x20);
    this->write_bytes((u8 *)hash.hash.data(), 32);
}

void BanchoFile::Writer::write_string(std::string str) {
    if(this->error_flag) {
        return;
    }

    if(str[0] == '\0') {
        u8 zero = 0;
        this->write<u8>(zero);
        return;
    }

    u8 empty_check = 11;
    this->write<u8>(empty_check);

    u32 len = str.length();
    this->write_uleb128(len);
    this->write_bytes((u8 *)str.c_str(), len);
}

void BanchoFile::Writer::flush() {
    if(this->error_flag || this->file == NULL) {
        return;
    }

    if(fwrite(this->buffer, 1, this->pos, this->file) != this->pos) {
        this->set_error("Failed to write to file: " + std::string(strerror(errno)));
        return;
    }
    this->pos = 0;
}

void BanchoFile::Writer::write_bytes(u8 *bytes, size_t n) {
    if(this->error_flag || this->file == NULL) {
        return;
    }

    if(this->pos + n > WRITE_BUFFER_SIZE) {
        this->flush();
        if(this->error_flag) {
            return;
        }
    }

    if(this->pos + n > WRITE_BUFFER_SIZE) {
        this->set_error("Attempted to write " + std::to_string(n) + " bytes (exceeding buffer size " +
                        std::to_string(WRITE_BUFFER_SIZE) + ")");
        return;
    }

    memcpy(this->buffer + this->pos, bytes, n);
    this->pos += n;
}

void BanchoFile::Writer::write_uleb128(u32 num) {
    if(this->error_flag) {
        return;
    }

    if(num == 0) {
        u8 zero = 0;
        this->write<u8>(zero);
        return;
    }

    while(num != 0) {
        u8 next = num & 0x7F;
        num >>= 7;
        if(num != 0) {
            next |= 0x80;
        }
        this->write<u8>(next);
    }
}

void BanchoFile::copy(const char *from_path, const char *to_path) {
    Reader from(from_path);
    Writer to(to_path);

    if(!from.good()) {
        debugLog("Failed to open source file for copying: %s\n", from.error().data());
        return;
    }

    if(!to.good()) {
        debugLog("Failed to open destination file for copying: %s\n", to.error().data());
        return;
    }

    u8 *buf = (u8 *)malloc(READ_BUFFER_SIZE);
    if(buf == NULL) {
        debugLog("Failed to allocate copy buffer\n");
        return;
    }

    u32 remaining = from.total_size;
    while(remaining > 0 && from.good() && to.good()) {
        u32 len = std::min(remaining, (u32)READ_BUFFER_SIZE);
        if(from.read_bytes(buf, len) != len) {
            debugLog("Copy failed: could not read %u bytes, %u remaining\n", len, remaining);
            break;
        }
        to.write_bytes(buf, len);
        remaining -= len;
    }

    if(!from.good()) {
        debugLog("Copy failed during read: %s\n", from.error().data());
    }
    if(!to.good()) {
        debugLog("Copy failed during write: %s\n", to.error().data());
    }

    free(buf);
}
