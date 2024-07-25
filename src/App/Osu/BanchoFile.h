#pragma once
#include "cbase.h"

#define READ_BUFFER_SIZE (512 * 4096)
#define WRITE_BUFFER_SIZE (512 * 4096)

struct BanchoFileReader {
    BanchoFileReader(const char* path);
    ~BanchoFileReader();

    void read_bytes(u8* out, size_t len);
    MD5Hash read_hash();
    std::string read_string();
    u32 read_uleb128();
    void skip_bytes(u32 n);
    void skip_string();

    template <typename T>
    T read() {
        static_assert(sizeof(T) < READ_BUFFER_SIZE);
        static u8 NULL_ARRAY[sizeof(T)] = {0};

        if(file == NULL) {
            return *(T*)NULL_ARRAY;
        }

        // XXX: Use proper ring buffer instead of memmove
        if(pos + sizeof(T) > avail) {
            memmove(buffer, buffer + pos, avail);
            pos = 0;
            avail += fread(buffer + avail, 1, READ_BUFFER_SIZE - avail, file);
        }

        if(pos + sizeof(T) > avail) {
            pos = (pos + sizeof(T)) % READ_BUFFER_SIZE;
            avail = 0;
            return *(T*)NULL_ARRAY;
        } else {
            u8* out_ptr = buffer + pos;
            pos = (pos + sizeof(T)) % READ_BUFFER_SIZE;
            avail -= sizeof(T);
            return *(T*)(out_ptr);
        }
    }

   protected:
    FILE* file = NULL;

    u8* buffer = NULL;
    size_t pos = 0;
    size_t avail = 0;
};

struct BanchoFileWriter {
    BanchoFileWriter(const char* path);
    ~BanchoFileWriter();

    void flush();
    void write_bytes(u8* bytes, size_t n);
    void write_hash(MD5Hash hash);
    void write_string(std::string str);
    void write_uleb128(u32 num);

    template <typename T>
    void write(T t) {
        write_bytes((u8*)&t, sizeof(T));
    }

   protected:
    std::string file_path;
    std::string tmp_file_path;
    FILE* file = NULL;

    u8* buffer = NULL;
    size_t pos = 0;
    bool errored = false;
};
