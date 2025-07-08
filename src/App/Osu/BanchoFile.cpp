#include "BanchoFile.h"

#include "Engine.h"

BanchoFile::Reader::Reader(const char *path) {
    this->file = fopen(path, "rb");
    if(this->file == NULL) {
        debugLog("Failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    fseek(this->file, 0, SEEK_END);
    this->total_size = ftell(this->file);
    rewind(this->file);

    this->buffer = (u8 *)malloc(READ_BUFFER_SIZE);
}

BanchoFile::Reader::~Reader() {
    if(this->file == NULL) return;

    free(this->buffer);
    fclose(this->file);
}

size_t BanchoFile::Reader::read_bytes(u8 *out, size_t len) {
    if(this->file == NULL) {
        if(out != NULL) {
            memset(out, 0, len);
        }
        return 0;
    }

    if(len > READ_BUFFER_SIZE) {
        // TODO @kiwec: handle this gracefully
        debugLog("Tried to read %d bytes (exceeding %d)!\n", len, READ_BUFFER_SIZE);
        abort();
        return 0;
    }

    // XXX: Use proper ring buffer instead of memmove
    if(this->pos + len > this->avail) {
        memmove(this->buffer, this->buffer + this->pos, this->avail);
        this->pos = 0;
        this->avail += fread(this->buffer + this->avail, 1, READ_BUFFER_SIZE - this->avail, this->file);
    }

    if(this->pos + len > this->avail) {
        this->pos = (this->pos + len) % READ_BUFFER_SIZE;
        this->avail = 0;
        this->total_pos = this->total_size;
    } else {
        if(out != NULL) {
            memcpy(out, this->buffer + this->pos, len);
        }
        this->pos = (this->pos + len) % READ_BUFFER_SIZE;
        this->avail -= len;
        this->total_pos += len;
    }

    return len;
}

MD5Hash BanchoFile::Reader::read_hash() {
    MD5Hash hash;

    u8 empty_check = this->read<u8>();
    if(empty_check == 0) return hash;

    u32 len = this->read_uleb128();
    u32 extra = 0;
    if(len > 32) {
        debugLog("WARNING: Expected 32 bytes for hash, got %d!\n");
        extra = len - 32;
        len = 32;
    }

    if(this->read_bytes((u8 *)hash.hash, len) != len) {
        debugLogF("WARNING: failed to read {} bytes to obtain hash.\n", len);
        extra = len;
    }
    this->skip_bytes(extra);
    hash.hash[len] = '\0';
    return hash;
}

std::string BanchoFile::Reader::read_string() {
    u8 empty_check = this->read<u8>();
    if(empty_check == 0) return std::string();

    u32 len = this->read_uleb128();
    u8 *str = new u8[len + 1];
    if(this->read_bytes(str, len) != len) {
        debugLogF("WARNING: failed to read {} bytes.\n", len);
        return {""};
    }

    std::string str_out((const char *)str, len);
    delete[] str;

    return str_out;
}

u32 BanchoFile::Reader::read_uleb128() {
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

void BanchoFile::Reader::skip_bytes(u32 n) {
    if(n > READ_BUFFER_SIZE) {
        debugLog("WARNING: Skipping %d bytes (exceeding %d)!\n", n, READ_BUFFER_SIZE);
    }

    while(n > 0) {
        u32 chunk_len = n > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : n;
        if(this->read_bytes(NULL, chunk_len) != chunk_len) {
            debugLogF("WARNING: failed to read {} bytes, {} remaining\n", chunk_len, n);
            break;
        }
        n -= chunk_len;
    }
}

void BanchoFile::Reader::skip_string() {
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
        debugLog("Failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    this->buffer = (u8 *)malloc(WRITE_BUFFER_SIZE);
}

BanchoFile::Writer::~Writer() {
    if(this->file == NULL) return;

    this->flush();

    free(this->buffer);
    fclose(this->file);

    if(!this->errored) {
        env->deleteFile(this->file_path);  // Windows (the Microsoft docs are LYING)
        env->renameFile(this->tmp_file_path, this->file_path);
    }
}

void BanchoFile::Writer::write_hash(MD5Hash hash) {
    this->write<u8>(0x0B);
    this->write<u8>(0x20);
    this->write_bytes((u8 *)hash.hash, 32);
}

void BanchoFile::Writer::write_string(std::string str) {
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
    if(fwrite(this->buffer, 1, this->pos, this->file) != this->pos) {
        this->errored = true;
        debugLog("Failed to write to %s: %s\n", tmp_file_path.c_str(), strerror(errno));
    }
    this->pos = 0;
}

void BanchoFile::Writer::write_bytes(u8 *bytes, size_t n) {
    if(this->file == NULL || this->errored) return;

    if(this->pos + n > WRITE_BUFFER_SIZE) {
        this->flush();
    }
    if(this->pos + n > WRITE_BUFFER_SIZE) {
        // TODO @kiwec: handle this gracefully
        debugLog("Tried to write %d bytes (exceeding %d)!\n", n, WRITE_BUFFER_SIZE);
        abort();
    }

    memcpy(this->buffer + this->pos, bytes, n);
    this->pos += n;
}

void BanchoFile::Writer::write_uleb128(u32 num) {
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

    u8 *buf = (u8 *)malloc(READ_BUFFER_SIZE);
    u32 remaining = from.total_size;
    while(remaining > 0) {
        u32 len = std::min(remaining, (u32)READ_BUFFER_SIZE);
        if(from.read_bytes(buf, len) != len) {
            debugLogF("WARNING: failed to copy {} bytes, {} remaining\n", len, remaining);
            break;
        }
        to.write_bytes(buf, len);
        remaining -= len;
    }
    free(buf);
}
