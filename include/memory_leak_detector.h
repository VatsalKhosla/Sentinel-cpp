#ifndef MEMORY_LEAK_DETECTOR_H
#define MEMORY_LEAK_DETECTOR_H

#include "lifetime_analyzer.h"
#include "clang/AST/ASTContext.h"
#include <vector>
#include <string>

namespace safecpp {

struct MemoryLeakViolation {
    std::string variable;
    unsigned int alloc_line;
    std::string message;
    std::string scope;
    std::string file_path;
};

class MemoryLeakDetector {
public:
    explicit MemoryLeakDetector(clang::ASTContext *context);
    
    std::vector<MemoryLeakViolation> detect(const LifetimeAnalyzer& analyzer);
    void report(const std::vector<MemoryLeakViolation>& violations);
    
private:
    clang::ASTContext *context_;
};

} // namespace safecpp

#endif // MEMORY_LEAK_DETECTOR_H
