#include "cvm/lexer.hpp"
#include "cvm/parser.hpp"
#include "cvm/compiler.hpp"
#include "cvm/vm.hpp"
#include "cvm/debug.hpp"
#include "cvm/error.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

void run(const std::string& source, bool dumpTokensFlag = false, bool dumpASTFlag = false, bool dumpBytecodeFlag = false) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (dumpTokensFlag) {
            dumpTokens(tokens);
            return;
        }

        Parser parser(std::move(tokens));
        auto program = parser.parse();
        if (dumpASTFlag) {
            dumpAST(program);
            return;
        }

        Compiler compiler;
        auto chunk = compiler.compile(program);
        if (dumpBytecodeFlag) {
            disassemble(chunk, "Disassembly");
            return;
        }

        VM vm;
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
    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpBytecode = false;
    std::string path;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens") dumpTokens = true;
        else if (arg == "--dump-ast") dumpAST = true;
        else if (arg == "--dump-bytecode") dumpBytecode = true;
        else if (path.empty()) path = arg;
    }

    if (!path.empty()) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << path << std::endl;
            exit(1);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        run(buffer.str(), dumpTokens, dumpAST, dumpBytecode);
    } else if (argc == 1 || (path.empty() && argc > 1)) {
        repl();
    } else {
        std::cout << "Usage: cvm [options] [path]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --dump-tokens" << std::endl;
        std::cout << "  --dump-ast" << std::endl;
        std::cout << "  --dump-bytecode" << std::endl;
    }
    return 0;
}
