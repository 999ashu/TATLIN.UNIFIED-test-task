#pragma once

#include <concepts>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <tape.h>

namespace fs = std::filesystem;

namespace detail {

constexpr std::size_t MAX_TMP = 8;
constexpr std::size_t СHUNK_SIZE = 2 * 1024 * 1024;

template <std::integral T>
void merge_tapes(const std::vector<fs::path>& tapes, const std::string& out_path, const std::string& cfg) {
    if (tapes.empty()) {
        return;
    }

    if (tapes.size() == 1) {
        std::error_code ec;
        fs::copy_file(tapes[0], out_path, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            throw std::runtime_error("Copy failed: " + ec.message());
        }
        return;
    }

    auto comparator = [](const std::pair<T, std::size_t>& a, const std::pair<T, std::size_t>& b) {
        return a.first > b.first;
    };
    std::priority_queue<std::pair<T, std::size_t>,
                        std::vector<std::pair<T, std::size_t>>,
                        decltype(comparator)> queue;

    std::vector<Tape<T>> inputs;
    inputs.reserve(tapes.size());
    for (std::size_t i = 0; i < tapes.size(); ++i) {
        try {
            inputs.emplace_back(tapes[i].string(), "r", cfg);
            auto value = inputs[i].read();
            queue.push({value, i});
        } catch (const std::runtime_error& ignored) {}
    }

    Tape<T> out{out_path, "w", cfg};
    bool first = true;
    while (!queue.empty()) {
        auto [value, index] = queue.top();
        queue.pop();
        if (!first) {
            out.move_forward();
        }
        first = false;
        out.write(value);
        try {
            inputs[index].move_forward();
            queue.push({inputs[index].read(), index});
        } catch (const std::runtime_error&) {}
    }
}

template <std::integral T>
std::pair<std::vector<fs::path>, std::size_t> partial_sort(const fs::path& in_path, const fs::path& tmpDir,
                                                           const fs::path& cfg) {
    const std::size_t elements = СHUNK_SIZE / sizeof(T);
    std::vector<fs::path> tapes;
    std::size_t total = 0;

    std::vector<fs::path> tmp_files;
    tmp_files.reserve(MAX_TMP);
    for (std::size_t i = 0; i < MAX_TMP; ++i) {
        tmp_files.push_back(tmpDir / ("tape_" + std::to_string(i) + ".bin"));
    }

    std::size_t ringIdx = 0;
    std::size_t mergeIdx = 0;

    Tape<T> in{in_path, "r", cfg};
    while (true) {
        if (tapes.size() >= MAX_TMP) {
            fs::path merged = tmpDir / ("merged_" + std::to_string(mergeIdx % 2) + ".bin");
            merge_tapes<T>(tapes, merged.string(), cfg);
            tapes.clear();
            tapes.push_back(merged);
            mergeIdx++;
        }

        std::vector<T> buf;
        buf.reserve(elements);

        try {
            for (std::size_t i = 0; i < elements; ++i) {
                buf.emplace_back(in.read());
                in.move_forward();
            }
        } catch (const std::runtime_error& ignored) {}

        if (buf.empty()) { break; }

        total += buf.size();
        std::sort(buf.begin(), buf.end());

        const auto& buffTape = tmp_files[ringIdx % tmp_files.size()];
        tapes.push_back(buffTape);
        ringIdx++;

        Tape<T> out{buffTape.string(), "w", cfg};
        if (!buf.empty()) {
            out.write(buf[0]);
            for (std::size_t i = 1; i < buf.size(); ++i) {
                out.move_forward();
                out.write(buf[i]);
            }
        }
    }

    return {tapes, total};
}
} // namespace detail

template <std::integral T>
void sort(const std::string& in_path, const std::string& out_path, const std::string& cfg = "") {
    const fs::path tmpDir = "tmpDir";

    fs::create_directories(tmpDir);
    auto [tapes, total] = detail::partial_sort<T>(in_path, tmpDir, cfg);
    if (!tapes.empty()) {
        detail::merge_tapes<T>(tapes, out_path, cfg);
    }
    fs::remove_all(tmpDir);
}
