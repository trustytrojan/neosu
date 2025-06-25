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

        if(this->file == NULL) {
            T result;
            memcpy(&result, NULL_ARRAY, sizeof(T));
            return result;
        }

        // XXX: Use proper ring buffer instead of memmove
        if(this->pos + sizeof(T) > this->avail) {
            memmove(this->buffer, this->buffer + this->pos, this->avail);
            this->pos = 0;
            this->avail += fread(this->buffer + this->avail, 1, READ_BUFFER_SIZE - this->avail, this->file);
        }

        if(this->pos + sizeof(T) > this->avail) {
            this->pos = (this->pos + sizeof(T)) % READ_BUFFER_SIZE;
            this->avail = 0;
            this->total_pos = this->total_size;
            T result;
            memcpy(&result, NULL_ARRAY, sizeof(T));
            return result;
        } else {
            u8* out_ptr = this->buffer + this->pos;
            this->pos = (this->pos + sizeof(T)) % READ_BUFFER_SIZE;
            this->avail -= sizeof(T);
            this->total_pos += sizeof(T);

            // use memcpy to avoid misaligned access
            T result;
            memcpy(&result, out_ptr, sizeof(T));
            return result;
        }
    }

    size_t total_size = 0;
    size_t total_pos = 0;

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
        this->write_bytes((u8*)&t, sizeof(T));
    }

   protected:
    std::string file_path;
    std::string tmp_file_path;
    FILE* file = NULL;

    u8* buffer = NULL;
    size_t pos = 0;
    bool errored = false;
};

void copy(const char* from_path, const char* to_path);
