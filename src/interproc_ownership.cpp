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
    
    // Analyze parameters for pointer types
    for (size_t i = 0; i < sig->param_types.size(); ++i) {
        const auto& param_type = sig->param_types[i];
        
        // Check if this is a pointer parameter
        if (param_type.find('*') != std::string::npos) {
            // Heuristic: if parameter name suggests allocation, mark it
            if (param_type.find("const") == std::string::npos) {
                summary.escaping_params.insert(i);
            }
        }
    }
    
    // Analyze return type
    if (sig->return_type.find('*') != std::string::npos) {
        // Function returns a pointer
        if (sig->return_type.find("const") == std::string::npos) {
            summary.returns_borrowed_pointer = true;
            
            // Check if it appears to allocate memory (heuristic)
            // In a real implementation, this would use dataflow analysis
            if (func.find("new") != std::string::npos || 
                func.find("alloc") != std::string::npos ||
                func.find("create") != std::string::npos) {
                summary.returns_allocated_memory = true;
            }
        }
    }
    
    summaries_[func] = summary;
}

void InterprocOwnershipAnalyzer::propagateOwnership() {
    // Walk call graph edges and propagate ownership information
    // Build ownership transfer graph by analyzing function calls
    
    for (const auto& edge : call_graph_.getEdges()) {
        // For each call from caller to callee:
        // 1. Determine which parameters are pointer types
        // 2. Track if pointers escape through return value
        // 3. Track if pointers escape to other functions
        
        const FunctionSignature* callee_sig = call_graph_.getFunctionSignature(edge.callee);
        if (!callee_sig) continue;
        
        // Analyze each parameter of the callee function
        for (size_t param_idx = 0; param_idx < callee_sig->param_types.size(); ++param_idx) {
            const auto& param_type = callee_sig->param_types[param_idx];
            
            // Check if parameter is a pointer type
            // Simple heuristic: contains '*' in the type string
            if (param_type.find('*') != std::string::npos) {
                OwnershipTransfer transfer;
                transfer.source_func = edge.caller;
                transfer.target_func = edge.callee;
                transfer.param_index = param_idx;
                transfer.transfer_line = edge.call_line;
                transfer.variable = "param_" + std::to_string(param_idx);
                
                // Determine transfer type based on parameter type
                if (param_type.find("const") != std::string::npos) {
                    transfer.transfer_type = "pass_by_const_ptr";
                } else if (param_type.find("*&") != std::string::npos) {
                    transfer.transfer_type = "pass_by_reference";
                } else {
                    transfer.transfer_type = "pass_by_value";
                }
                
                transfers_.push_back(transfer);
            }
        }
        
        // Also track return value if it's a pointer
        if (callee_sig->return_type.find('*') != std::string::npos) {
            OwnershipTransfer transfer;
            transfer.source_func = edge.callee;
            transfer.target_func = edge.caller;
            transfer.param_index = static_cast<unsigned int>(-1);  // Special value for return
            transfer.transfer_line = edge.call_line;
            transfer.variable = "return_value";
            transfer.transfer_type = "return_value";
            
            transfers_.push_back(transfer);
        }
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
    
    // Cross-function UAF detection via ownership transfer analysis
    // Algorithm:
    // 1. For each ownership transfer in the call graph
    // 2. Track the variable through the call chain
    // 3. If the variable is freed in any callee and used in a caller or sibling, flag it
    
    // Check each transfer for potential issues
    for (const auto& transfer : transfers_) {
        // Find all functions reachable from target_func
        auto callees = call_graph_.getTransitiveCalls(transfer.target_func);
        
        // For each variable that could be passed through this transfer,
        // check if it's freed downstream
        const auto& all_lifetimes = lifetime_analyzer_.getAllLifetimes();
        
        for (const auto& [var_name, lifetime] : all_lifetimes) {
            // Skip if not a potential issue
            if (!lifetime.freed_at || !lifetime.used_after_free) {
                continue;
            }
            
            // Check if this variable could flow through the transfer
            // A variable flows if:
            // 1. It matches a parameter name (heuristic)
            // 2. Or it appears in an escaped pointer set
            
            // For now, use name-based heuristic matching
            // In a full implementation, we'd use points-to analysis
            
            // Build violation if potential cross-function issue detected
            // This is conservative - real implementation uses alias analysis
            
            // Check if freed_func is reachable from source_func
            std::vector<std::string> path;
            if (transfer.target_func != transfer.source_func &&
                callees.find(transfer.target_func) != callees.end()) {
                // There's potential for pointer flow through this call
                
                CrossFunctionUAFViolation violation;
                violation.variable = var_name;
                violation.transfer_type = transfer.transfer_type;
                
                // Build ownership chain
                violation.ownership_chain.push_back(transfer.source_func);
                violation.ownership_chain.push_back(transfer.target_func);
                
                // Record line numbers if available
                violation.free_line = 0;     // Would need LifetimeAnalyzer enhancement
                violation.use_line = 0;      // Would need LifetimeAnalyzer enhancement
                
                // Only report high-confidence violations
                // (requires actual pointer tracking, for now this is infrastructure)
                // violations.push_back(violation);
            }
        }
    }
    
    return violations;
}

} // namespace safecpp
