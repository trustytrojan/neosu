#include "BanchoFile.h"

#include "Engine.h"

BanchoFileReader::BanchoFileReader(const char *path) {
    file = fopen(path, "rb");
    if(file == NULL) {
        debugLog("Failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    fseek(file, 0, SEEK_END);
    total_size = ftell(file);
    rewind(file);

    buffer = (u8 *)malloc(READ_BUFFER_SIZE);
}

BanchoFileReader::~BanchoFileReader() {
    if(file == NULL) return;

    free(buffer);
    fclose(file);
}

void BanchoFileReader::read_bytes(u8 *out, size_t len) {
    if(file == NULL) {
        if(out != NULL) {
            memset(out, 0, len);
        }
        return;
    }

    if(len > READ_BUFFER_SIZE) {
        // TODO @kiwec: handle this gracefully
        debugLog("Tried to read %d bytes (exceeding %d)!\n", len, READ_BUFFER_SIZE);
        abort();
    }

    // XXX: Use proper ring buffer instead of memmove
    if(pos + len > avail) {
        memmove(buffer, buffer + pos, avail);
        pos = 0;
        avail += fread(buffer + avail, 1, READ_BUFFER_SIZE - avail, file);
    }

    if(pos + len > avail) {
        pos = (pos + len) % READ_BUFFER_SIZE;
        avail = 0;
        total_pos = total_size;
    } else {
        if(out != NULL) {
            memcpy(out, buffer + pos, len);
        }
        pos = (pos + len) % READ_BUFFER_SIZE;
        avail -= len;
        total_pos += len;
    }
}

MD5Hash BanchoFileReader::read_hash() {
    MD5Hash hash;

    u8 empty_check = read<u8>();
    if(empty_check == 0) return hash;

    u32 len = read_uleb128();
	u32 extra = 0;
    if(len > 32) {
        debugLog("WARNING: Expected 32 bytes for hash, got %d!\n");
        extra = len - 32;
        len = 32;
    }

    read_bytes((u8 *)hash.hash, len);
    skip_bytes(extra);
    hash.hash[len] = '\0';
    return hash;
}

std::string BanchoFileReader::read_string() {
    u8 empty_check = read<u8>();
    if(empty_check == 0) return std::string();

    u32 len = read_uleb128();
    u8 *str = new u8[len + 1];
    read_bytes(str, len);

    std::string str_out((const char *)str, len);
    delete[] str;

    return str_out;
}

u32 BanchoFileReader::read_uleb128() {
    u32 result = 0;
    u32 shift = 0;
    u8 byte = 0;

    do {
        byte = read<u8>();
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while(byte & 0x80);

    return result;
}

void BanchoFileReader::skip_bytes(u32 n) {
    if(n > READ_BUFFER_SIZE) {
        debugLog("WARNING: Skipping %d bytes (exceeding %d)!\n", n, READ_BUFFER_SIZE);
    }

    while(n > 0) {
        u32 chunk_len = n > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : n;
        read_bytes(NULL, chunk_len);
        n -= chunk_len;
    }
}

void BanchoFileReader::skip_string() {
    u8 empty_check = read<u8>();
    if(empty_check == 0) return;

    u32 len = read_uleb128();
    skip_bytes(len);
}

BanchoFileWriter::BanchoFileWriter(const char *path) {
    file_path = path;
    tmp_file_path = file_path;
    tmp_file_path.append(".tmp");

    file = fopen(tmp_file_path.c_str(), "wb");
    if(file == NULL) {
        debugLog("Failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    buffer = (u8 *)malloc(WRITE_BUFFER_SIZE);
}

BanchoFileWriter::~BanchoFileWriter() {
    if(file == NULL) return;

    flush();

    free(buffer);
    fclose(file);

    if(!errored) {
        env->deleteFile(file_path);  // Windows (the Microsoft docs are LYING)
        env->renameFile(tmp_file_path, file_path);
    }
}

void BanchoFileWriter::write_hash(MD5Hash hash) {
    write<u8>(0x0B);
    write<u8>(0x20);
    write_bytes((u8 *)hash.hash, 32);
}

void BanchoFileWriter::write_string(std::string str) {
    if(str[0] == '\0') {
        u8 zero = 0;
        write<u8>(zero);
        return;
    }

    u8 empty_check = 11;
    write<u8>(empty_check);

    u32 len = str.length();
    write_uleb128(len);
    write_bytes((u8 *)str.c_str(), len);
}

void BanchoFileWriter::flush() {
    if(fwrite(buffer, 1, pos, file) != pos) {
        errored = true;
        debugLog("Failed to write to %s: %s\n", tmp_file_path.c_str(), strerror(errno));
    }
    pos = 0;
}

void BanchoFileWriter::write_bytes(u8 *bytes, size_t n) {
    if(file == NULL || errored) return;

    if(pos + n > WRITE_BUFFER_SIZE) {
        flush();
    }
    if(pos + n > WRITE_BUFFER_SIZE) {
        // TODO @kiwec: handle this gracefully
        debugLog("Tried to write %d bytes (exceeding %d)!\n", n, WRITE_BUFFER_SIZE);
        abort();
    }

    memcpy(buffer + pos, bytes, n);
    pos += n;
}

void BanchoFileWriter::write_uleb128(u32 num) {
    if(num == 0) {
        u8 zero = 0;
        write<u8>(zero);
        return;
    }

    while(num != 0) {
        u8 next = num & 0x7F;
        num >>= 7;
        if(num != 0) {
            next |= 0x80;
        }
        write<u8>(next);
    }
}

void copy(const char *from_path, const char *to_path) {
    BanchoFileReader from(from_path);
    BanchoFileWriter to(to_path);

    u8 *buf = (u8 *)malloc(READ_BUFFER_SIZE);
    u32 remaining = from.total_size;
    while(remaining > 0) {
        u32 len = std::min(remaining, (u32)READ_BUFFER_SIZE);
        from.read_bytes(buf, len);
        to.write_bytes(buf, len);
        remaining -= len;
    }
    free(buf);
}
