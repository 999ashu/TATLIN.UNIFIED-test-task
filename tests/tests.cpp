#include <gtest/gtest.h>
#include <tape.h>
#include <tape_sorter.h>

#include <fstream>
#include <random>
#include <tuple>

class TapeTest : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
protected:
    static std::string GetConfigPath() {
        return std::get<0>(GetParam());
    }

    static std::string GetTestName() {
        return std::get<1>(GetParam());
    }
};

TEST_P(TapeTest, Write) {
    {
        Tape<int> tape("tape.bin", "w", GetConfigPath());
        tape.write(2);
        tape.write(1);
        tape.move_forward();
        tape.write(2);
        tape.move_forward();
        tape.write(3);
    }

    std::ifstream file("tape.bin", std::ios::binary);
    ASSERT_TRUE(file.is_open());

    int value;

    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    EXPECT_EQ(1, value);
    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    EXPECT_EQ(2, value);
    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    EXPECT_EQ(3, value);

    file.close();

    std::filesystem::remove("tape.bin");
}

TEST_P(TapeTest, Read) {
    std::ofstream file("tape.bin", std::ios::binary);
    int values[] = {1, 2, 3};
    file.write(reinterpret_cast<char*>(values), sizeof(values));
    file.close();

    Tape<int> tape("tape.bin", "r", GetConfigPath());

    EXPECT_EQ(1, tape.read());
    EXPECT_EQ(1, tape.read());
    tape.move_forward();
    EXPECT_EQ(2, tape.read());
    tape.move_forward();
    EXPECT_EQ(3, tape.read());
    tape.move_forward();
    EXPECT_THROW(tape.read(), std::runtime_error);

    std::filesystem::remove("tape.bin");
}

TEST_P(TapeTest, Move) {
    std::ofstream file("tape.bin", std::ios::binary);
    for (int i = 0; i < 512; ++i) {
        file.write(reinterpret_cast<char*>(&i), sizeof(int));
    }
    file.close();

    {
        Tape<int> tape("tape.bin", "r", GetConfigPath());
        EXPECT_THROW(tape.move_backward(), std::runtime_error);
    }

    {
        Tape<int> tape("tape.bin", "r", GetConfigPath());
        EXPECT_THROW(tape.move(-20), std::runtime_error);
    }

    Tape<int> tape("tape.bin", "r+", GetConfigPath());

    EXPECT_EQ(0, tape.read());
    tape.move(19);
    EXPECT_EQ(19, tape.read());
    tape.move_forward();
    EXPECT_EQ(20, tape.read());
    tape.move_backward();
    EXPECT_EQ(19, tape.read());
    tape.move(190);
    EXPECT_EQ(209, tape.read());
    tape.move(-45);
    EXPECT_EQ(164, tape.read());
    tape.write(1337);
    tape.move(200);
    EXPECT_EQ(364, tape.read());
    tape.move(-200);
    EXPECT_EQ(1337, tape.read());

    EXPECT_THROW(tape.move(2000), std::runtime_error);

    std::filesystem::remove("tape.bin");
}

TEST_P(TapeTest, Stress) {
    constexpr size_t NUM_ELEMENTS = 2500000;
    constexpr size_t NUM_OPERATIONS = 100000;

    std::mt19937 gen(735675);
    std::uniform_int_distribution value_dist(1, 1000000);
    std::uniform_int_distribution op_dist(0, 3);
    std::uniform_int_distribution move_dist(-100, 100);

    std::vector<int> reference_data(NUM_ELEMENTS);
    for (auto& val : reference_data)
        val = value_dist(gen);

    {
        std::ofstream file("tape_stress.bin", std::ios::binary);
        file.write(reinterpret_cast<char*>(reference_data.data()), reference_data.size() * sizeof(int));
    }

    {
        Tape<int> tape("tape_stress.bin", "r+", GetConfigPath(), 1 << 7);
        size_t current_pos = 0;
        for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
            switch (op_dist(gen)) {
                case 1: {
                    int new_value = value_dist(gen);
                    tape.write(new_value);
                    reference_data[current_pos] = new_value;
                    break;
                }
                case 2: {
                    if (current_pos < NUM_ELEMENTS - 1) {
                        tape.move_forward();
                        ++current_pos;
                    }
                    break;
                }
                case 3: {
                    if (current_pos > 0) {
                        tape.move_backward();
                        --current_pos;
                    }
                    break;
                }
                default: {
                    EXPECT_EQ(reference_data[current_pos], tape.read());
                }
            }

            if (i > 0 && i % 100 == 0) {
                int steps = move_dist(gen);
                if (steps < 0 && static_cast<size_t>(std::abs(steps)) > current_pos) {
                    steps = -static_cast<int>(current_pos);
                }
                if (steps > 0 && current_pos + steps >= NUM_ELEMENTS) {
                    steps = NUM_ELEMENTS - current_pos - 1;
                }
                tape.move(steps);
                current_pos += steps;
                EXPECT_EQ(reference_data[current_pos], tape.read());
            }
        }
    }

    {
        std::ifstream file("tape_stress.bin", std::ios::binary);
        std::vector<int> read_data(NUM_ELEMENTS);
        file.read(reinterpret_cast<char*>(read_data.data()), NUM_ELEMENTS * sizeof(int));
        EXPECT_EQ(reference_data, read_data);
    }

    std::filesystem::remove("tape_stress.bin");
}

INSTANTIATE_TEST_SUITE_P(
    TapeTests,
    TapeTest,
    ::testing::Values(
        std::make_tuple("", "WithoutConfig"), std::make_tuple("config.txt", "WithConfig")
    ),
    [](const testing::TestParamInfo<TapeTest::ParamType>& info) {
    return std::get<1>(info.param);
    });

TEST(SortingTest, Sort) {
    const std::string inPath = "input.bin";
    const std::string outPath = "output.bin";

    constexpr size_t elements = 33 * 1024 * 1024 / sizeof(int);
    {
        std::mt19937 gen(73427);
        std::uniform_int_distribution dist(1, 10000000);

        std::ofstream fstream(inPath, std::ios::binary);
        for (size_t i = 0; i < elements; ++i) {
            int value = dist(gen);
            fstream.write(reinterpret_cast<char*>(&value), sizeof(int));
        }
        fstream.close();

        std::ofstream out(outPath, std::ios::binary);
        out.seekp(elements * sizeof(int));
        out.write("", 1);
        out.close();
    }

    sort<int>(inPath, outPath);

    {
        Tape<int> output(outPath, "r");
        int prev = output.read();

        for (size_t i = 1; i < elements; ++i) {
            output.move_forward();
            int curr = output.read();
            EXPECT_LE(prev, curr);
            prev = curr;
        }
    }

    std::filesystem::remove(inPath);
    std::filesystem::remove(outPath);
}