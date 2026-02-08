#ifndef UAF_DETECTOR_H
#define UAF_DETECTOR_H

#include "lifetime_analyzer.h"
#include "clang/AST/ASTContext.h"

namespace safecpp {

struct UAFViolation {
    std::string variable;
    unsigned int free_line;
    unsigned int use_line;
    std::string message;
};

class UAFDetector {
public:
    explicit UAFDetector(clang::ASTContext *context);
    
    std::vector<UAFViolation> detect(const LifetimeAnalyzer& analyzer);
    void report(const std::vector<UAFViolation>& violations);
    
private:
    clang::ASTContext *context_;
};

} // namespace safecpp

#endif // UAF_DETECTOR_H
