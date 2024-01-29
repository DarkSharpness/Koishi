#include "antlr4-runtime.h"
#include "MxLexer.h"
#include "MxParser.h"

#include "ASTerror.h"
#include "ASTbuilder.h"
#include "ASTchecker.h"

#include "IRbase.h"

#include <memory>

std::unique_ptr <dark::AST::ASTbuilder> parse_input() {
    auto input = std::make_unique <antlr4::ANTLRInputStream> (std::cin);

    auto lexer = std::make_unique <MxLexer> (input.get());
    lexer->removeErrorListeners();

    auto listener = std::make_unique <MxErrorListener> ();
    lexer->addErrorListener(listener.get());

    auto tokens = std::make_unique <antlr4::CommonTokenStream> (lexer.get());
    tokens->fill();

    auto parser = std::make_unique <MxParser> (tokens.get());
    parser->removeErrorListeners();
    parser->addErrorListener(listener.get());

    return std::make_unique <dark::AST::ASTbuilder> (parser->file_Input());
}

std::unique_ptr <dark::AST::ASTchecker> check_input(dark::AST::ASTbuilder *ptr) {
    return std::make_unique <dark::AST::ASTchecker> (ptr);
}

void compiler_work() {
    auto Wankupi    = parse_input();
    auto Hastin     = check_input(Wankupi.get());
    std::cerr << Wankupi->ASTtree();

    Hastin.reset();
    Wankupi.reset();
}


signed main(int argc, char** argv) {
    // try {
        compiler_work();
    // } catch(const std::exception& e) {
    //     /* If non-dark-error, speak out what. */
    //     if (!dynamic_cast <const dark::error *> (&e))
    //         std::cerr << "Error: " << e.what() << std::endl;
    //     return 1;
    // } catch(...) {
    //     std::cerr << "Unknown error!" << std::endl;
    //     return 1;
    // }
    std::cerr << "No error." << std::endl; // "No error.
    return 0;
}
