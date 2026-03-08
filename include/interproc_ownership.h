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
    std::vector<std::string> detectCrossFunctionUAF() const;
    
private:
    const CallGraph& call_graph_;
    const LifetimeAnalyzer& lifetime_analyzer_;
    
    std::vector<OwnershipTransfer> transfers_;
    std::map<std::string, FunctionOwnershipSummary> summaries_;
    
    // Analyze individual function for ownership
    void analyzeFunctionOwnership(const std::string& func);
    
    // Propagate ownership through call graph
    void propagateOwnership();
};

} // namespace safecpp

#endif // INTERPROC_OWNERSHIP_H
