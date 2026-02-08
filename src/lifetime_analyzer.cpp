#include "lifetime_analyzer.h"

namespace safecpp {

void LifetimeAnalyzer::trackAllocation(const std::string& var_name, const clang::Stmt* stmt) {
    lifetimes_[var_name].name = var_name;
    lifetimes_[var_name].state = LifetimeState::ALIVE;
}

void LifetimeAnalyzer::trackFree(const std::string& var_name, const clang::Stmt* stmt) {
    if (lifetimes_.find(var_name) != lifetimes_.end()) {
        lifetimes_[var_name].state = LifetimeState::FREED;
        lifetimes_[var_name].freed_at = stmt;
    }
}

void LifetimeAnalyzer::trackUse(const std::string& var_name, const clang::Stmt* stmt) {
    if (lifetimes_.find(var_name) != lifetimes_.end()) {
        if (lifetimes_[var_name].state == LifetimeState::FREED) {
            lifetimes_[var_name].used_after_free = stmt;
        }
    }
}

bool LifetimeAnalyzer::isFreed(const std::string& var_name) const {
    auto it = lifetimes_.find(var_name);
    if (it != lifetimes_.end()) {
        return it->second.state == LifetimeState::FREED;
    }
    return false;
}

bool LifetimeAnalyzer::hasUseAfterFree(const std::string& var_name) const {
    auto it = lifetimes_.find(var_name);
    if (it != lifetimes_.end()) {
        return it->second.used_after_free != nullptr;
    }
    return false;
}

const VariableLifetime* LifetimeAnalyzer::getLifetime(const std::string& var_name) const {
    auto it = lifetimes_.find(var_name);
    if (it != lifetimes_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::pair<std::string, VariableLifetime>> LifetimeAnalyzer::getViolations() const {
    std::vector<std::pair<std::string, VariableLifetime>> violations;
    
    for (const auto& [name, lifetime] : lifetimes_) {
        if (lifetime.used_after_free != nullptr) {
            violations.push_back({name, lifetime});
        }
    }
    
    return violations;
}

} 
