#include "cvm/lexer.hpp"
#include "cvm/parser.hpp"
#include "cvm/compiler.hpp"
#include "cvm/vm.hpp"
#include "cvm/error.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

void run(const std::string& source, bool debug = false) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        auto program = parser.parse();

        Compiler compiler;
        auto chunk = compiler.compile(program);

        VM vm(debug);
        vm.execute(chunk);
    } catch (const CVMError& e) {
        if (e.line != -1) {
            std::cerr << "[Error] line " << e.line << ": " << e.what() << std::endl;
        } else {
            std::cerr << "[Error] " << e.what() << std::endl;
        }
        exit(1);
    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        exit(1);
    }
}

void runFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << path << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    run(buffer.str());
}

void repl(bool debug = false) {
    VM vm(debug);
    std::string line;
    std::cout << "cvm++ REPL (type 'exit' to quit)" << std::endl;
    while (true) {
        std::cout << "cvm> ";
        if (!std::getline(std::cin, line) || line == "exit") break;
        if (line.empty()) continue;

        // Ensure line ends with a newline for the parser
        if (line.back() != '\n') line += '\n';

        try {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();

            Parser parser(std::move(tokens));
            auto program = parser.parse();

            Compiler compiler;
            auto chunk = compiler.compile(program);

            vm.execute(chunk);
        } catch (const CVMError& e) {
            std::cerr << "[Error] " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Fatal Error] " << e.what() << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        runFile(argv[1]);
    } else if (argc == 1) {
        repl();
    } else {
        std::cout << "Usage: cvm [path]" << std::endl;
    }
    return 0;
}
