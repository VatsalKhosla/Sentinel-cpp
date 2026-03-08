#include "interproc_ownership.h"
#include <algorithm>
#include <queue>

namespace safecpp {

InterprocOwnershipAnalyzer::InterprocOwnershipAnalyzer(
    const CallGraph& call_graph,
    const LifetimeAnalyzer& lifetime_analyzer)
    : call_graph_(call_graph), lifetime_analyzer_(lifetime_analyzer) {}

void InterprocOwnershipAnalyzer::analyze() {
    // First pass: analyze individual functions
    // (In a real implementation, we'd iterate through all analyzed functions)
    
    // Second pass: propagate ownership information
    propagateOwnership();
    
    // Third pass: detect cross-function UAF violations
    auto violations = detectCrossFunctionUAF();
    uaf_violations_ = violations;
}

const FunctionOwnershipSummary* InterprocOwnershipAnalyzer::getFunctionSummary(
    const std::string& func) const {
    auto it = summaries_.find(func);
    return (it != summaries_.end()) ? &it->second : nullptr;
}

void InterprocOwnershipAnalyzer::analyzeFunctionOwnership(const std::string& func) {
    FunctionOwnershipSummary summary;
    summary.function_name = func;
    summary.returns_allocated_memory = false;
    summary.returns_borrowed_pointer = false;
    
    // Get function signature
    const FunctionSignature* sig = call_graph_.getFunctionSignature(func);
    if (!sig) {
        return;
    }
    
    // Analyze parameters and return value
    // (Simplified - real implementation would use deep flow analysis)
    
    summaries_[func] = summary;
}

void InterprocOwnershipAnalyzer::propagateOwnership() {
    // Walk call graph edges and propagate ownership information
    for (const auto& edge : call_graph_.getEdges()) {
        // If caller passes pointer to callee, record the transfer
        OwnershipTransfer transfer;
        transfer.source_func = edge.caller;
        transfer.target_func = edge.callee;
        transfer.transfer_line = edge.call_line;
        transfer.transfer_type = "pass_by_reference";  // Default
        
        // Would determine actual transfer type based on function signature
        transfers_.push_back(transfer);
    }
}

bool InterprocOwnershipAnalyzer::findCallPath(
    const std::string& from_func, const std::string& to_func,
    std::vector<std::string>& path) const {
    
    if (from_func == to_func) {
        path.push_back(from_func);
        return true;
    }
    
    // BFS to find path in call graph
    std::queue<std::pair<std::string, std::vector<std::string>>> q;
    std::set<std::string> visited;
    
    q.push({from_func, {from_func}});
    visited.insert(from_func);
    
    while (!q.empty()) {
        auto [curr_func, curr_path] = q.front();
        q.pop();
        
        auto callees = call_graph_.getCallees(curr_func);
        for (const auto& callee : callees) {
            if (callee == to_func) {
                curr_path.push_back(callee);
                path = curr_path;
                return true;
            }
            
            if (visited.find(callee) == visited.end()) {
                visited.insert(callee);
                auto new_path = curr_path;
                new_path.push_back(callee);
                q.push({callee, new_path});
            }
        }
    }
    
    return false;
}

bool InterprocOwnershipAnalyzer::canPointerFlow(
    const std::string& ptr_var, const std::string& source_func,
    const std::string& target_func) const {
    
    // Check if there's a call path from source to target
    std::vector<std::string> path;
    return findCallPath(source_func, target_func, path);
}

std::vector<CrossFunctionUAFViolation> InterprocOwnershipAnalyzer::detectCrossFunctionUAF() const {
    std::vector<CrossFunctionUAFViolation> violations;
    
    // For cross-function UAF detection, we need to analyze the flow of pointers
    // across function boundaries. This is a complex analysis that requires:
    // 1. Tracking which pointers are passed as parameters or return values
    // 2. Understanding ownership semantics at function boundaries
    // 3. Detecting if a pointer is freed in one function and used in another
    
    // For now, this is a placeholder that could be enhanced with:
    // - Parameter alias analysis (which parameters point to the same object)
    // - Interprocedural points-to analysis
    // - Escape analysis to determine which pointers can leave a function
    
    // Get all function lifetimes
    const auto& all_lifetimes = lifetime_analyzer_.getAllLifetimes();
    
    for (const auto& [var_name, lifetime] : all_lifetimes) {
        // For each variable with a potential use-after-free
        if (lifetime.freed_at && lifetime.used_after_free) {
            // Check if the free and use are in different functions
            // This would require source location tracking in the LifetimeAnalyzer
            // which we can enhance in future versions
            
            CrossFunctionUAFViolation violation;
            violation.variable = var_name;
            violation.free_func = "unknown";  // Would need function context from lifetime info
            violation.free_line = 0;
            violation.use_func = "unknown";
            violation.use_line = 0;
            // violations.push_back(violation);
        }
    }
    
    return violations;
}

} // namespace safecpp
