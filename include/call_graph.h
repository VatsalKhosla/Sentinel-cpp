#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace safecpp {

// Represents a single function call
struct CallEdge {
    std::string caller;           // Function that makes the call
    std::string callee;           // Function being called
    unsigned int call_line;       // Line where call occurs
    std::string call_site_file;   // File where call occurs
};

// Tracks function signatures and parameters
struct FunctionSignature {
    std::string name;
    std::vector<std::string> param_types;
    std::string return_type;
    bool is_definition;  // true if this is a function definition, false if just declaration
};

class CallGraph {
public:
    explicit CallGraph(clang::ASTContext *context);
    
    // Build call graph from AST
    void build();
    
    // Query functions
    const std::set<std::string>& getCallees(const std::string& caller) const;
    const std::set<std::string>& getCallers(const std::string& callee) const;
    bool functionExists(const std::string& name) const;
    const FunctionSignature* getFunctionSignature(const std::string& name) const;
    
    // Get all edges
    const std::vector<CallEdge>& getEdges() const { return edges_; }
    
    // Interprocedural queries
    std::set<std::string> getTransitiveCalls(const std::string& func) const;
    bool canFunctionEscape(const std::string& func, unsigned int param_index) const;
    
private:
    clang::ASTContext *context_;
    
    // Adjacency lists for call graph
    std::map<std::string, std::set<std::string>> callees_;  // func -> functions it calls
    std::map<std::string, std::set<std::string>> callers_;  // func -> functions that call it
    
    // All call edges
    std::vector<CallEdge> edges_;
    
    // Function signatures
    std::map<std::string, FunctionSignature> signatures_;
    
    // Visitor to build graph
    void visitDecl(clang::Decl *decl);
    void visitFunctionDecl(clang::FunctionDecl *func_decl);
};

} // namespace safecpp

#endif // CALL_GRAPH_H
