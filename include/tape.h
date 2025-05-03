#pragma once

#include <concepts>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace detail {
constexpr std::size_t BUFFER_SIZE = 1 << 7;

struct FileDeleter {
    void operator()(FILE* file) const { fclose(file); }
};
}  // namespace detail

template <std::integral T>
class Tape {
    using UniqueFile = std::unique_ptr<FILE, detail::FileDeleter>;

    T* buffer_;
    UniqueFile file_;
    std::size_t slice{};
    std::size_t idx{};

   public:
    Tape() = delete;

    Tape(const std::filesystem::path& path, const char* mode) : buffer_(new T[detail::BUFFER_SIZE]) {
        FILE* file = fopen(path.c_str(), mode);
        if (!file) {
            throw std::runtime_error("Can't open: " + std::string(path.c_str()) + strerror(errno));
        }
        file_ = UniqueFile(file);
    }

    ~Tape() { delete[] buffer_; }

    T read() {

    }

    void write() {

    }

    static void move(size_t steps) {

    };
};
