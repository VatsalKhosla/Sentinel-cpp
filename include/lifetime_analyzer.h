#ifndef LIFETIME_ANALYZER_H
#define LIFETIME_ANALYZER_H

#include <string>
#include <map>
#include <set>
#include "clang/AST/AST.h"

namespace safecpp {

enum class LifetimeState {
    ALIVE,
    FREED,
    UNKNOWN
};

struct VariableLifetime {
    std::string name;
    LifetimeState state;
    const clang::Stmt* freed_at;
    const clang::Stmt* used_after_free;
    
    VariableLifetime() : state(LifetimeState::UNKNOWN), freed_at(nullptr), used_after_free(nullptr) {}
};

class LifetimeAnalyzer {
public:
    LifetimeAnalyzer() = default;
    
    void trackAllocation(const std::string& var_name, const clang::Stmt* stmt);
    void trackFree(const std::string& var_name, const clang::Stmt* stmt);
    void trackUse(const std::string& var_name, const clang::Stmt* stmt);
    
    bool isFreed(const std::string& var_name) const;
    bool hasUseAfterFree(const std::string& var_name) const;
    
    const VariableLifetime* getLifetime(const std::string& var_name) const;
    
    std::vector<std::pair<std::string, VariableLifetime>> getViolations() const;
    
private:
    std::map<std::string, VariableLifetime> lifetimes_;
};

} // namespace safecpp

#endif // LIFETIME_ANALYZER_H
