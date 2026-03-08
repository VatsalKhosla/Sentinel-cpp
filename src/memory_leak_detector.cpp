#include "memory_leak_detector.h"
#include "clang/Basic/SourceManager.h"
#include <iostream>

namespace safecpp {

MemoryLeakDetector::MemoryLeakDetector(clang::ASTContext *context) 
    : context_(context) {}

std::vector<MemoryLeakViolation> MemoryLeakDetector::detect(const LifetimeAnalyzer& analyzer) {
    std::vector<MemoryLeakViolation> violations;
    
    auto all_lifetimes = analyzer.getAllLifetimes();
    
    for (const auto& [var_name, lifetime] : all_lifetimes) {
        if (lifetime.state == LifetimeState::ALIVE && 
            lifetime.allocated_at != nullptr && 
            !lifetime.freed_at) {
            
            MemoryLeakViolation violation;
            violation.variable = var_name;
            
            const clang::SourceManager& sm = context_->getSourceManager();
            clang::SourceLocation alloc_loc = lifetime.allocated_at->getBeginLoc();
            violation.alloc_line = sm.getSpellingLineNumber(alloc_loc);
            
            std::string filename = sm.getFilename(alloc_loc).str();
            violation.scope = filename;
            
            violation.message = "Memory leak: variable '" + var_name + 
                               "' allocated but never freed";
            violations.push_back(violation);
        }
    }
    
    return violations;
}

void MemoryLeakDetector::report(const std::vector<MemoryLeakViolation>& violations) {
    if (violations.empty()) {
        return;
    }
    
    std::cout << "\n=== Memory Leak Detection Results ===\n\n";
    
    for (const auto& violation : violations) {
        std::cout << "warning: memory leak detected\n";
        std::cout << "  Variable: " << violation.variable << "\n";
        std::cout << "  Allocated at line: " << violation.alloc_line << "\n";
        std::cout << "  " << violation.message << "\n\n";
    }
    
    std::cout << "Found " << violations.size() << " potential memory leak(s)\n";
}

} // namespace safecpp
