#include "interproc_ownership.h"
#include <algorithm>

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
        // transfers_.push_back(transfer);
    }
}

std::vector<std::string> InterprocOwnershipAnalyzer::detectCrossFunctionUAF() const {
    std::vector<std::string> violations;
    
    // For each ownership transfer, check if:
    // 1. Pointer is freed in one function
    // 2. Used in another function after the free
    
    // Simplified implementation - real one would do full interprocedural flow analysis
    
    return violations;
}

} // namespace safecpp
