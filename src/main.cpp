#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/AST/ASTConsumer.h"
#include "llvm/Support/CommandLine.h"

#include "ast_walker.h"
#include "lifetime_analyzer.h"
#include "uaf_detector.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory SafeCppCategory("safecpp options");

class SafeCppConsumer : public ASTConsumer {
public:
    explicit SafeCppConsumer(ASTContext *context)
        : analyzer_(), walker_(context, analyzer_), detector_(context) {}
    
    virtual void HandleTranslationUnit(ASTContext &context) override {
        walker_.TraverseDecl(context.getTranslationUnitDecl());
        
        auto violations = detector_.detect(analyzer_);
        detector_.report(violations);
    }
    
private:
    safecpp::LifetimeAnalyzer analyzer_;
    safecpp::ASTWalker walker_;
    safecpp::UAFDetector detector_;
};

class SafeCppAction : public ASTFrontendAction {
public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &compiler, StringRef file) override {
        return std::make_unique<SafeCppConsumer>(&compiler.getASTContext());
    }
};

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, SafeCppCategory);
    
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    
    return Tool.run(newFrontendActionFactory<SafeCppAction>().get());
}
