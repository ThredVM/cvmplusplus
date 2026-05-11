#include <iostream>

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::cout << "Running script: " << argv[1] << std::endl;
    } else {
        std::cout << "Entering REPL (placeholder)" << std::endl;
    }
    return 0;
}
