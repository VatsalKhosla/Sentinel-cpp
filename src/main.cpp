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
#include "call_graph.h"
#include "interproc_ownership.h"
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

static bool hasBlockingFindings(const safecpp::AnalysisResults& results, const safecpp::Config& config) {
    if (config.getUseAfterFreeConfig().enabled &&
        config.getUseAfterFreeConfig().severity == safecpp::Severity::ERROR &&
        !results.uaf_violations.empty()) {
        return true;
    }

    if (config.getMemoryLeakConfig().enabled &&
        config.getMemoryLeakConfig().severity == safecpp::Severity::ERROR &&
        !results.leak_violations.empty()) {
        return true;
    }

    if (config.getNullDerefConfig().enabled &&
        config.getNullDerefConfig().severity == safecpp::Severity::ERROR &&
        !results.null_violations.empty()) {
        return true;
    }

    return false;
}

class SafeCppConsumer : public ASTConsumer {
public:
    explicit SafeCppConsumer(ASTContext *context, safecpp::Config& config, safecpp::AnalysisResults& results)
        : config_(config), results_(results), analyzer_(), walker_(context, analyzer_), 
          uaf_detector_(context), leak_detector_(context), null_detector_(context),
          call_graph_(context) {}
    
    virtual void HandleTranslationUnit(ASTContext &context) override {
        // Intraprocedural analysis (current)
        walker_.TraverseDecl(context.getTranslationUnitDecl());
        null_detector_.TraverseDecl(context.getTranslationUnitDecl());

        // Build call graph for interprocedural analysis
        call_graph_.build();
        
        // Interprocedural ownership analysis
        safecpp::InterprocOwnershipAnalyzer interproc_analyzer(call_graph_, analyzer_);
        interproc_analyzer.analyze();
        
        // Detect violations
        results_.uaf_violations = uaf_detector_.detect(analyzer_);
        results_.leak_violations = leak_detector_.detect(analyzer_);
        results_.null_violations = null_detector_.getViolations();
        
        // Add cross-function UAF violations
        auto cross_func_uaf = interproc_analyzer.detectCrossFunctionUAF();
        if (config_.getUseAfterFreeConfig().enabled && !cross_func_uaf.empty()) {
            if (Verbose) {
                std::cout << "Detected " << cross_func_uaf.size() << " cross-function UAF violations\n";
            }
        }
    }
    
private:
    safecpp::Config& config_;
    safecpp::AnalysisResults& results_;
    safecpp::LifetimeAnalyzer analyzer_;
    safecpp::ASTWalker walker_;
    safecpp::UAFDetector uaf_detector_;
    safecpp::MemoryLeakDetector leak_detector_;
    safecpp::NullDerefDetector null_detector_;
    safecpp::CallGraph call_graph_;
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

    safecpp::Config config = safecpp::Config::getDefault();
    
    if (!ConfigFile.empty()) {
        config.loadFromFile(ConfigFile);
    }

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

    safecpp::AnalysisResults results;
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    
    auto factory = std::make_unique<SafeCppActionFactory>(config, results);
    int status = Tool.run(factory.get());

    safecpp::ReportGenerator reporter(config);
    reporter.generate(results);

    if (status != 0) {
        return status;
    }

    return hasBlockingFindings(results, config) ? 2 : 0;
}
