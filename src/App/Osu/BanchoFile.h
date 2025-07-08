#pragma once

#include <cstddef>
#include <cstring>

#include "MD5Hash.h"
#include "types.h"

class BanchoFile {
   private:
    static constexpr const size_t READ_BUFFER_SIZE{512ULL * 4096};
    static constexpr const size_t WRITE_BUFFER_SIZE{512ULL * 4096};

   public:
    class Reader {
       public:
        Reader(const char* path);
        ~Reader();

        [[nodiscard]] size_t read_bytes(u8* out, size_t len);
        [[nodiscard]] MD5Hash read_hash();
        [[nodiscard]] std::string read_string();
        [[nodiscard]] u32 read_uleb128();
        void skip_bytes(u32 n);
        void skip_string();

        template <typename T>
        [[nodiscard]] T read() {
            static_assert(sizeof(T) < READ_BUFFER_SIZE);

            if(this->file == NULL) {
                T result;
                memset(&result, 0, sizeof(T));
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
                memset(&result, 0, sizeof(T));
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

       private:
        FILE* file = NULL;

        u8* buffer = NULL;
        size_t pos = 0;
        size_t avail = 0;
    };

    class Writer {
       public:
        Writer(const char* path);
        ~Writer();

        void flush();
        void write_bytes(u8* bytes, size_t n);
        void write_hash(MD5Hash hash);
        void write_string(std::string str);
        void write_uleb128(u32 num);

        template <typename T>
        void write(T t) {
            this->write_bytes((u8*)&t, sizeof(T));
        }

       private:
        std::string file_path;
        std::string tmp_file_path;
        FILE* file = NULL;

        u8* buffer = NULL;
        size_t pos = 0;
        bool errored = false;
    };

    static void copy(const char* from_path, const char* to_path);
};
