#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <algorithm>

#include "MD5Hash.h"
#include "types.h"

#ifdef _MSC_VER
#define always_inline_attr __forceinline
#elif defined(__GNUC__)
#define always_inline_attr [[gnu::always_inline]] inline
#else
#define always_inline_attr
#endif

class ByteBufferedFile {
   private:
    static constexpr const size_t READ_BUFFER_SIZE{512ULL * 4096};
    static constexpr const size_t WRITE_BUFFER_SIZE{512ULL * 4096};

   public:
    class Reader {
       public:
        Reader(const char* path);
        ~Reader();

        Reader &operator=(const Reader &) = delete;
        Reader &operator=(Reader &&) = delete;
        Reader(const Reader &) = delete;
        Reader(Reader &&) = delete;

        // always_inline is a 2x speedup here
        [[nodiscard]] always_inline_attr size_t read_bytes(u8* out, size_t len) {
            if(this->error_flag || !this->file) {
                if(out != NULL) {
                    memset(out, 0, len);
                }
                return 0;
            }

            if(len > READ_BUFFER_SIZE) {
                this->set_error("Attempted to read " + std::to_string(len) + " bytes (exceeding buffer size " +
                                std::to_string(READ_BUFFER_SIZE) + ")");
                if(out != NULL) {
                    memset(out, 0, len);
                }
                return 0;
            }

            // make sure the ring buffer has enough data
            if(this->buffered_bytes < len) {
                // calculate available space for reading more data
                size_t available_space = READ_BUFFER_SIZE - this->buffered_bytes;
                size_t bytes_to_read = available_space;

                if(this->write_pos + bytes_to_read <= READ_BUFFER_SIZE) {
                    // no wrap needed, read directly
                    size_t bytes_read = fread(this->buffer + this->write_pos, 1, bytes_to_read, this->file);
                    this->write_pos = (this->write_pos + bytes_read) % READ_BUFFER_SIZE;
                    this->buffered_bytes += bytes_read;
                } else {
                    // wrap needed, read in two parts
                    size_t first_part = READ_BUFFER_SIZE - this->write_pos;
                    size_t bytes_read = fread(this->buffer + this->write_pos, 1, first_part, this->file);

                    if(bytes_read == first_part && bytes_to_read > first_part) {
                        size_t second_part = bytes_to_read - first_part;
                        size_t second_read = fread(this->buffer, 1, second_part, this->file);
                        bytes_read += second_read;
                        this->write_pos = second_read;
                    } else {
                        this->write_pos = (this->write_pos + bytes_read) % READ_BUFFER_SIZE;
                    }

                    this->buffered_bytes += bytes_read;
                }
            }

            if(this->buffered_bytes < len) {
                // couldn't read enough data
                if(out != NULL) {
                    memset(out, 0, len);
                }
                return 0;
            }

            // read from ring buffer
            if(out != NULL) {
                if(this->read_pos + len <= READ_BUFFER_SIZE) {
                    // no wrap needed
                    memcpy(out, this->buffer + this->read_pos, len);
                } else {
                    // wrap needed
                    size_t first_part = std::min(len, READ_BUFFER_SIZE - this->read_pos);
                    size_t second_part = len - first_part;

                    memcpy(out, this->buffer + this->read_pos, first_part);
                    memcpy(out + first_part, this->buffer, second_part);
                }
            }

            this->read_pos = (this->read_pos + len) % READ_BUFFER_SIZE;
            this->buffered_bytes -= len;
            this->total_pos += len;

            return len;
        }

        template <typename T>
        [[nodiscard]] T read() {
            static_assert(sizeof(T) < READ_BUFFER_SIZE);

            T result;
            if((this->read_bytes((u8*)&result, sizeof(T))) != sizeof(T)) {
                memset(&result, 0, sizeof(T));
            }
            return result;
        }

        always_inline_attr void skip_bytes(u32 n) {
            if(this->error_flag || !this->file) {
                return;
            }

            // if we can skip entirely within the buffered data
            if(n <= this->buffered_bytes) {
                this->read_pos = (this->read_pos + n) % READ_BUFFER_SIZE;
                this->buffered_bytes -= n;
                this->total_pos += n;
                return;
            }

            // we need to skip more than what's buffered
            u32 skip_from_buffer = this->buffered_bytes;
            u32 skip_from_file = n - skip_from_buffer;

            // skip what's in the buffer
            this->total_pos += skip_from_buffer;

            // seek in the file to skip the rest
            if(fseek(this->file, skip_from_file, SEEK_CUR) != 0) {
                this->set_error("Failed to seek " + std::to_string(skip_from_file) + " bytes");
                return;
            }

            this->total_pos += skip_from_file;

            // since we've moved past buffered data, reset buffer state
            this->read_pos = 0;
            this->write_pos = 0;
            this->buffered_bytes = 0;
        }

        template <typename T>
        void skip() {
            static_assert(sizeof(T) < READ_BUFFER_SIZE);
            this->skip_bytes(sizeof(T));
        }

        [[nodiscard]] bool good() const { return !this->error_flag; }
        [[nodiscard]] std::string_view error() const { return this->last_error; }

        [[nodiscard]] MD5Hash read_hash();
        [[nodiscard]] std::string read_string();
        [[nodiscard]] u32 read_uleb128();

        void skip_string();

        size_t total_size{0};
        size_t total_pos{0};

       private:
        void set_error(const std::string& error_msg);

        FILE* file{nullptr};

        u8* buffer{nullptr};
        size_t read_pos{0};        // current read position in ring buffer
        size_t write_pos{0};       // current write position in ring buffer
        size_t buffered_bytes{0};  // amount of data currently buffered

        bool error_flag{false};
        std::string last_error;
    };

    class Writer {
       public:
        Writer(const char* path);
        ~Writer();

        Writer &operator=(const Writer &) = delete;
        Writer &operator=(Writer &&) = delete;
        Writer(const Writer &) = delete;
        Writer(Writer &&) = delete;

        [[nodiscard]] bool good() const { return !this->error_flag; }
        [[nodiscard]] std::string_view error() const { return this->last_error; }

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
        void set_error(const std::string& error_msg);

        std::string file_path;
        std::string tmp_file_path;
        FILE* file{nullptr};

        u8* buffer{nullptr};
        size_t pos{0};
        bool error_flag{false};
        std::string last_error;
    };

    static void copy(const char* from_path, const char* to_path);
};
