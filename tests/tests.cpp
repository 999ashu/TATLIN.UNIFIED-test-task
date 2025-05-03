#include <gtest/gtest.h>
#include <tape.h>

#include <fstream>

TEST(Tape, write) {
    {
        Tape<int> tape("tape", "w");
        tape.write(2);
        tape.write(1);
        tape.moveForward();
        tape.write(2);
        tape.moveForward();
        tape.write(3);
    }

    std::ifstream file("tape", std::ios::binary);
    ASSERT_TRUE(file.is_open());

    int value;

    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    EXPECT_EQ(1, value);
    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    EXPECT_EQ(2, value);
    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    EXPECT_EQ(3, value);

    file.close();
}

TEST(Tape, read) {
    std::ofstream file("tape", std::ios::binary);
    int values[] = {1, 2, 3};
    file.write(reinterpret_cast<char*>(values), sizeof(values));
    file.close();

    Tape<int> tape("tape", "r");

    EXPECT_EQ(1, tape.read());
    EXPECT_EQ(1, tape.read());
    tape.moveForward();
    EXPECT_EQ(2, tape.read());
    tape.moveForward();
    EXPECT_EQ(3, tape.read());
    tape.moveForward();
    EXPECT_THROW(tape.read(), std::runtime_error);
}

TEST(Tape, move) {
    std::ofstream file("tape", std::ios::binary);
    for (int i = 0; i < 512; ++i) {
        file.write(reinterpret_cast<char*>(&i), sizeof(int));
    }
    file.close();

    {
        Tape<int> tape("tape", "r");
        EXPECT_THROW(tape.moveBackward(), std::runtime_error);
    }

    {
        Tape<int> tape("tape", "r");
        EXPECT_THROW(tape.move(-20), std::runtime_error);
    }

    Tape<int> tape("tape", "r+");

    EXPECT_EQ(0, tape.read());
    tape.move(19);
    EXPECT_EQ(19, tape.read());
    tape.moveForward();
    EXPECT_EQ(20, tape.read());
    tape.moveBackward();
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
}
