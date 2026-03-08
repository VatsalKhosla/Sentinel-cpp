#include "call_graph.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include <algorithm>

namespace safecpp {

class CallGraphBuilder : public clang::RecursiveASTVisitor<CallGraphBuilder> {
public:
    explicit CallGraphBuilder(CallGraph *cg, clang::ASTContext *context)
        : call_graph_(cg), context_(context), current_func_("") {}
    
    bool VisitFunctionDecl(clang::FunctionDecl *decl) {
        std::string func_name = decl->getNameAsString();
        std::string prev_func = current_func_;
        current_func_ = func_name;
        
        // Record function signature
        FunctionSignature sig;
        sig.name = func_name;
        sig.return_type = decl->getReturnType().getAsString();
        sig.is_definition = decl->hasBody();
        
        for (const auto *param : decl->parameters()) {
            sig.param_types.push_back(param->getType().getAsString());
        }
        
        if (decl->hasBody()) {
            this->TraverseStmt(decl->getBody());
        }
        
        current_func_ = prev_func;
        return true;
    }
    
    bool VisitCallExpr(clang::CallExpr *call) {
        if (!current_func_.empty()) {
            if (const clang::FunctionDecl *callee = call->getDirectCallee()) {
                std::string callee_name = callee->getNameAsString();
                
                // Record the call edge
                const clang::SourceManager& sm = context_->getSourceManager();
                clang::SourceLocation loc = call->getBeginLoc();
                
                CallEdge edge;
                edge.caller = current_func_;
                edge.callee = callee_name;
                edge.call_line = sm.getSpellingLineNumber(loc);
                edge.call_site_file = sm.getFilename(loc).str();
            }
        }
        
        return true;
    }
    
private:
    CallGraph *call_graph_;
    clang::ASTContext *context_;
    std::string current_func_;
};

CallGraph::CallGraph(clang::ASTContext *context) : context_(context) {}

void CallGraph::build() {
    CallGraphBuilder builder(this, context_);
    builder.TraverseDecl(context_->getTranslationUnitDecl());
}

const std::set<std::string>& CallGraph::getCallees(const std::string& caller) const {
    static const std::set<std::string> empty;
    auto it = callees_.find(caller);
    return (it != callees_.end()) ? it->second : empty;
}

const std::set<std::string>& CallGraph::getCallers(const std::string& callee) const {
    static const std::set<std::string> empty;
    auto it = callers_.find(callee);
    return (it != callers_.end()) ? it->second : empty;
}

bool CallGraph::functionExists(const std::string& name) const {
    return signatures_.find(name) != signatures_.end();
}

const FunctionSignature* CallGraph::getFunctionSignature(const std::string& name) const {
    auto it = signatures_.find(name);
    return (it != signatures_.end()) ? &it->second : nullptr;
}

std::set<std::string> CallGraph::getTransitiveCalls(const std::string& func) const {
    std::set<std::string> result;
    std::set<std::string> visited;
    
    std::vector<std::string> worklist;
    worklist.push_back(func);
    
    while (!worklist.empty()) {
        std::string current = worklist.back();
        worklist.pop_back();
        
        if (visited.find(current) != visited.end()) {
            continue;
        }
        visited.insert(current);
        
        const auto& callees = getCallees(current);
        for (const auto& callee : callees) {
            if (visited.find(callee) == visited.end()) {
                result.insert(callee);
                worklist.push_back(callee);
            }
        }
    }
    
    return result;
}

bool CallGraph::canFunctionEscape(const std::string& func, unsigned int param_index) const {
    // Simplified: a parameter can escape if it's passed to another function
    // Real implementation would do full escape analysis
    const auto& callees = getCallees(func);
    return !callees.empty();
}

} // namespace safecpp
