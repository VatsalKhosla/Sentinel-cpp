#include "uaf_detector.h"
#include "clang/Basic/SourceManager.h"
#include <iostream>

namespace safecpp {

UAFDetector::UAFDetector(clang::ASTContext *context) : context_(context) {}

std::vector<UAFViolation> UAFDetector::detect(const LifetimeAnalyzer& analyzer) {
    std::vector<UAFViolation> violations;
    
    auto all_violations = analyzer.getViolations();
    
    for (const auto& [var_name, lifetime] : all_violations) {
        UAFViolation violation;
        violation.variable = var_name;
        
        const clang::SourceManager& sm = context_->getSourceManager();
        
        if (lifetime.freed_at) {
            clang::SourceLocation free_loc = lifetime.freed_at->getBeginLoc();
            violation.free_line = sm.getSpellingLineNumber(free_loc);
        }
        
        if (lifetime.used_after_free) {
            clang::SourceLocation use_loc = lifetime.used_after_free->getBeginLoc();
            violation.use_line = sm.getSpellingLineNumber(use_loc);
        }
        
        violation.message = "Use-after-free detected for variable '" + var_name + "'";
        violations.push_back(violation);
    }
    
    return violations;
}

void UAFDetector::report(const std::vector<UAFViolation>& violations) {
    if (violations.empty()) {
        std::cout << "No memory safety issues found!\n";
        return;
    }
    
    std::cout << "\n=== Memory Safety Analysis Results ===\n\n";
    
    for (const auto& violation : violations) {
        std::cout << "error: use-after-free detected\n";
        std::cout << "  Variable: " << violation.variable << "\n";
        std::cout << "  Freed at line: " << violation.free_line << "\n";
        std::cout << "  Used at line: " << violation.use_line << "\n";
        std::cout << "  " << violation.message << "\n\n";
    }
    
    std::cout << "Found " << violations.size() << " memory safety violation(s)\n";
}

} 
