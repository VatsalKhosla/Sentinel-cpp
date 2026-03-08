#ifndef INTERPROC_OWNERSHIP_H
#define INTERPROC_OWNERSHIP_H

#include "call_graph.h"
#include "lifetime_analyzer.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace safecpp {

// Represents ownership transfer across function boundary
struct OwnershipTransfer {
    std::string variable;        // Variable name
    std::string source_func;     // Function where pointer originates
    std::string target_func;     // Function it's passed to
    unsigned int param_index;    // Parameter index in target function
    std::string transfer_type;   // "pass_by_value", "pass_by_reference", "return_value"
    unsigned int transfer_line;  // Where the transfer happens
};

// Cross-function UAF violation
struct CrossFunctionUAFViolation {
    std::string variable;
    std::string alloc_func;      // Where allocated
    unsigned int alloc_line;
    std::string free_func;       // Where freed
    unsigned int free_line;
    std::string use_func;        // Where used after free
    unsigned int use_line;
    std::string transfer_type;   // How pointer was passed (pass_by_value, pass_by_reference, return_value)
    std::vector<std::string> ownership_chain;  // [func1 -> func2 -> func3]
};

// Tracks which pointers can escape from a function
struct FunctionOwnershipSummary {
    std::string function_name;
    
    // Parameters that are allocated inside and returned
    std::set<unsigned int> allocated_params;
    
    // Parameters that can escape (passed to other functions)
    std::set<unsigned int> escaping_params;
    
    // Return value ownership
    bool returns_allocated_memory;
    bool returns_borrowed_pointer;
};

class InterprocOwnershipAnalyzer {
public:
    explicit InterprocOwnershipAnalyzer(const CallGraph& call_graph, const LifetimeAnalyzer& lifetime_analyzer);
    
    // Analyze ownership flows
    void analyze();
    
    // Get ownership transfers
    const std::vector<OwnershipTransfer>& getTransfers() const { return transfers_; }
    
    // Get function summaries
    const FunctionOwnershipSummary* getFunctionSummary(const std::string& func) const;
    
    // Cross-function UAF detection
    std::vector<CrossFunctionUAFViolation> detectCrossFunctionUAF() const;
    
private:
    const CallGraph& call_graph_;
    const LifetimeAnalyzer& lifetime_analyzer_;
    
    std::vector<OwnershipTransfer> transfers_;
    std::vector<CrossFunctionUAFViolation> uaf_violations_;
    std::map<std::string, FunctionOwnershipSummary> summaries_;
    
    // Analyze individual function for ownership
    void analyzeFunctionOwnership(const std::string& func);
    
    // Propagate ownership through call graph
    void propagateOwnership();
    
    // Helper: check if pointer can flow from source to target function
    bool canPointerFlow(const std::string& ptr_var, const std::string& source_func, const std::string& target_func) const;
    
    // Helper: get path from function A to function B in call graph
    bool findCallPath(const std::string& from_func, const std::string& to_func, 
                     std::vector<std::string>& path) const;
};

} // namespace safecpp

#endif // INTERPROC_OWNERSHIP_H
