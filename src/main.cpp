#include <tape_sorter.h>
#include <iostream>

int main(const int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_tape> <output_tape>" << std::endl;
        return 1;
    }
    try {
        sort<int>(argv[1], argv[2]);
        std::cout << "Done." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception while sorting: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
