#ifndef AST_WALKER_H
#define AST_WALKER_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "lifetime_analyzer.h"

namespace safecpp {

class ASTWalker : public clang::RecursiveASTVisitor<ASTWalker> {
public:
    explicit ASTWalker(clang::ASTContext *context, LifetimeAnalyzer& analyzer);
    
    bool VisitVarDecl(clang::VarDecl *decl);
    bool VisitCallExpr(clang::CallExpr *expr);
    bool VisitDeclRefExpr(clang::DeclRefExpr *expr);
    bool VisitCXXDeleteExpr(clang::CXXDeleteExpr *expr);
    
private:
    clang::ASTContext *context_;
    LifetimeAnalyzer& analyzer_;
    
    bool isFreeCall(const clang::CallExpr *expr);
    bool isNewExpr(const clang::Expr *expr);
    std::string getVarName(const clang::Expr *expr);
};

} // namespace safecpp

#endif // AST_WALKER_H
