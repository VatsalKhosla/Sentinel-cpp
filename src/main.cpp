#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/AST/ASTConsumer.h"
#include "llvm/Support/CommandLine.h"

#include <iostream>
#include "ast_walker.h"
#include "lifetime_analyzer.h"
#include "uaf_detector.h"
#include "memory_leak_detector.h"
#include "null_deref_detector.h"
#include "config.h"
#include "report_generator.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory SafeCppCategory("safecpp options");

static cl::opt<std::string> OutputFormat(
    "format",
    cl::desc("Output format (text, json, html, sarif)"),
    cl::value_desc("format"),
    cl::init("text"),
    cl::cat(SafeCppCategory));

static cl::opt<std::string> OutputFile(
    "output",
    cl::desc("Output file (default: stdout for text/json, or auto-generated filename)"),
    cl::value_desc("filename"),
    cl::init(""),
    cl::cat(SafeCppCategory));

static cl::opt<std::string> ConfigFile(
    "config",
    cl::desc("Configuration file path"),
    cl::value_desc("path"),
    cl::init(""),
    cl::cat(SafeCppCategory));

static cl::opt<bool> Verbose(
    "verbose",
    cl::desc("Enable verbose output"),
    cl::init(false),
    cl::cat(SafeCppCategory));

class SafeCppConsumer : public ASTConsumer {
public:
    explicit SafeCppConsumer(ASTContext *context, safecpp::Config& config, safecpp::AnalysisResults& results)
        : config_(config), results_(results), analyzer_(), walker_(context, analyzer_), 
          uaf_detector_(context), leak_detector_(context), null_detector_(context) {}
    
    virtual void HandleTranslationUnit(ASTContext &context) override {
        walker_.TraverseDecl(context.getTranslationUnitDecl());
        null_detector_.TraverseDecl(context.getTranslationUnitDecl());
        
        // Collect all violations
        results_.uaf_violations = uaf_detector_.detect(analyzer_);
        results_.leak_violations = leak_detector_.detect(analyzer_);
        results_.null_violations = null_detector_.getViolations();
    }
    
private:
    safecpp::Config& config_;
    safecpp::AnalysisResults& results_;
    safecpp::LifetimeAnalyzer analyzer_;
    safecpp::ASTWalker walker_;
    safecpp::UAFDetector uaf_detector_;
    safecpp::MemoryLeakDetector leak_detector_;
    safecpp::NullDerefDetector null_detector_;
};

class SafeCppAction : public ASTFrontendAction {
public:
    explicit SafeCppAction(safecpp::Config& config, safecpp::AnalysisResults& results)
        : config_(config), results_(results) {}
    
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &compiler, StringRef file) override {
        return std::make_unique<SafeCppConsumer>(&compiler.getASTContext(), config_, results_);
    }
    
private:
    safecpp::Config& config_;
    safecpp::AnalysisResults& results_;
};

class SafeCppActionFactory : public FrontendActionFactory {
public:
    explicit SafeCppActionFactory(safecpp::Config& config, safecpp::AnalysisResults& results)
        : config_(config), results_(results) {}
    
    std::unique_ptr<FrontendAction> create() override {
        return std::make_unique<SafeCppAction>(config_, results_);
    }
    
private:
    safecpp::Config& config_;
    safecpp::AnalysisResults& results_;
};

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, SafeCppCategory);
    
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    
    // Load configuration
    safecpp::Config config = safecpp::Config::getDefault();
    
    if (!ConfigFile.empty()) {
        config.loadFromFile(ConfigFile);
    }
    
    // Parse command-line format option
    std::string format_str = OutputFormat;
    if (format_str == "json") {
        config.setOutputFormat(safecpp::OutputFormat::JSON);
    } else if (format_str == "html") {
        config.setOutputFormat(safecpp::OutputFormat::HTML);
    } else if (format_str == "sarif") {
        config.setOutputFormat(safecpp::OutputFormat::SARIF);
    } else {
        config.setOutputFormat(safecpp::OutputFormat::TEXT);
    }
    
    if (!OutputFile.empty()) {
        config.setOutputFile(OutputFile);
    }
    
    config.setVerbose(Verbose);
    
    // Run analysis
    safecpp::AnalysisResults results;
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    
    auto factory = std::make_unique<SafeCppActionFactory>(config, results);
    int status = Tool.run(factory.get());
    
    // Generate report
    safecpp::ReportGenerator reporter(config);
    reporter.generate(results);
    
    return status;
}
