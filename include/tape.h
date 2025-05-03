#pragma once

#include <concepts>
#include <cstdio>
#include <cstring>
#include <filesystem>


template <std::integral T>
class Tape {
    T* buffer;
    FILE* file;
    std::size_t fileSize{};
    uint32_t slice{};
    uint32_t idx{};
    const uint32_t bufferSize;
    bool modified = false;

    void loadSlice() {
        fseek(file, slice * bufferSize * sizeof(T), SEEK_SET);

        uint32_t canRead = bufferSize;
        if (slice * bufferSize + canRead > fileSize) {
            canRead = fileSize - slice * bufferSize;
        }

        uint32_t available = fread(buffer, sizeof(T), canRead, file);

        if (available < bufferSize) {
            std::memset(buffer + available, 0, (bufferSize - available) * sizeof(T));
        }

        modified = false;
    }

    void flushBuffer() {
        if (!modified) {
            return;
        }

        fseek(file, slice * bufferSize * sizeof(T), SEEK_SET);

        uint32_t canWrite = bufferSize;
        if (slice * bufferSize + canWrite > fileSize) {
            canWrite = fileSize - slice * bufferSize;
        }

        fwrite(buffer, sizeof(T), canWrite, file);

        modified = false;
    }

public:
    Tape() = delete;

    Tape(const std::filesystem::path& path, const char* mode, const std::size_t size = 1 << 7)
        : buffer(new T[size]), bufferSize(size) {
        file = fopen(path.c_str(), mode);
        if (!file) {
            throw std::runtime_error("Can't open: " + std::string(path.c_str()) + strerror(errno));
        }

        fseek(file, 0, SEEK_END);
        const long sizeBytes = ftell(file);
        fileSize = sizeBytes / sizeof(T);
        fseek(file, 0, SEEK_SET);

        loadSlice();
    }

    ~Tape() {
        if (modified) {
            flushBuffer();
        }
        fclose(file);
        delete[] buffer;
    }

    T read() {
        if (slice * bufferSize + idx >= fileSize) {
            throw std::runtime_error("End of tape reached");
        }

        return buffer[idx];
    }

    void write(T value) {
        buffer[idx] = value;
        modified = true;
        const uint32_t pos = slice * bufferSize + idx;
        if (pos >= fileSize) {
            fileSize = pos + 1;
        }
    }

    void moveForward() {
        ++idx;
        if (idx >= bufferSize) {
            flushBuffer();
            ++slice;
            idx = 0;
            if (slice * bufferSize >= fileSize)
                throw std::runtime_error("Cannot move forward beyond the end of the tape");
            loadSlice();
        }
    }

    void moveBackward() {
        if (idx == 0 && slice == 0) {
            throw std::runtime_error("Cannot move backwards beyond the beginning of the tape");
        }
        if (idx == 0) {
            flushBuffer();
            --slice;
            idx = bufferSize - 1;
            loadSlice();
        } else {
            --idx;
        }
    }

    void move(const int32_t steps) {
        flushBuffer();

        const uint32_t pos = slice * bufferSize + idx;

        if (steps < 0 && std::abs(steps) > pos) {
            throw std::runtime_error("Cannot move backwards beyond the beginning of the tape");
        }
        const uint32_t newPos = pos + steps;
        if (newPos >= fileSize) {
            throw std::runtime_error("Cannot move forward beyond the end of the tape");
        }

        const uint32_t newSlice = newPos / bufferSize;
        const uint32_t newIdx = newPos % bufferSize;
        if (newSlice != slice) {
            slice = newSlice;
            idx = newIdx;
            loadSlice();
        } else {
            idx = newIdx;
        }
    }
};
