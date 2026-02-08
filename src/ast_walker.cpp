#include "ast_walker.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"

namespace safecpp {

ASTWalker::ASTWalker(clang::ASTContext *context, LifetimeAnalyzer& analyzer)
    : context_(context), analyzer_(analyzer) {}

bool ASTWalker::VisitVarDecl(clang::VarDecl *decl) {
    if (decl->hasInit()) {
        if (isNewExpr(decl->getInit())) {
            const clang::Expr* init = decl->getInit();
            analyzer_.trackAllocation(decl->getNameAsString(), init);
        }
    }
    return true;
}

bool ASTWalker::VisitCallExpr(clang::CallExpr *expr) {
    if (isFreeCall(expr)) {
        if (expr->getNumArgs() > 0) {
            std::string var_name = getVarName(expr->getArg(0));
            if (!var_name.empty()) {
                analyzer_.trackFree(var_name, expr);
            }
        }
    }
    return true;
}

bool ASTWalker::VisitDeclRefExpr(clang::DeclRefExpr *expr) {
    std::string var_name = expr->getNameInfo().getAsString();
    analyzer_.trackUse(var_name, expr);
    return true;
}

bool ASTWalker::VisitCXXDeleteExpr(clang::CXXDeleteExpr *expr) {
    std::string var_name = getVarName(expr->getArgument());
    if (!var_name.empty()) {
        analyzer_.trackFree(var_name, expr);
    }
    return true;
}

bool ASTWalker::isFreeCall(const clang::CallExpr *expr) {
    if (const clang::FunctionDecl *func = expr->getDirectCallee()) {
        std::string name = func->getNameAsString();
        return name == "free";
    }
    return false;
}

bool ASTWalker::isNewExpr(const clang::Expr *expr) {
    return llvm::isa<clang::CXXNewExpr>(expr);
}

std::string ASTWalker::getVarName(const clang::Expr *expr) {
    expr = expr->IgnoreParenCasts();
    
    if (const auto *declRef = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        return declRef->getNameInfo().getAsString();
    }
    
    return "";
}

} 
