#include "ast_walker.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ParentMapContext.h"

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
    if (isInFreeOrDeleteContext(expr)) {
        return true;
    }

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

bool ASTWalker::isFreeCall(const clang::CallExpr *expr) const {
    if (const clang::FunctionDecl *func = expr->getDirectCallee()) {
        std::string name = func->getNameAsString();
        return name == "free";
    }
    return false;
}

bool ASTWalker::isNewExpr(const clang::Expr *expr) {
    return llvm::isa<clang::CXXNewExpr>(expr);
}

bool ASTWalker::isInFreeOrDeleteContext(const clang::DeclRefExpr *expr) const {
    clang::DynTypedNode current = clang::DynTypedNode::create(*expr);

    for (int depth = 0; depth < 12; ++depth) {
        auto parents = context_->getParents(current);
        if (parents.empty()) {
            return false;
        }

        const clang::DynTypedNode &parent = parents[0];

        if (const auto *delete_expr = parent.get<clang::CXXDeleteExpr>()) {
            const clang::Expr *arg = delete_expr->getArgument();
            const std::string ref_name = expr->getNameInfo().getAsString();
            return !ref_name.empty() && getVarName(arg) == ref_name;
        }

        if (const auto *call_expr = parent.get<clang::CallExpr>()) {
            if (isFreeCall(call_expr) && call_expr->getNumArgs() > 0) {
                const std::string ref_name = expr->getNameInfo().getAsString();
                return !ref_name.empty() && getVarName(call_expr->getArg(0)) == ref_name;
            }
        }

        if (const auto *binary_op = parent.get<clang::BinaryOperator>()) {
            if (binary_op->isAssignmentOp()) {
                const std::string ref_name = expr->getNameInfo().getAsString();
                return !ref_name.empty() && getVarName(binary_op->getLHS()) == ref_name;
            }
        }

        current = parent;
    }

    return false;
}

std::string ASTWalker::getVarName(const clang::Expr *expr) const {
    expr = expr->IgnoreParenCasts();
    
    if (const auto *declRef = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        return declRef->getNameInfo().getAsString();
    }
    
    return "";
}

} 
