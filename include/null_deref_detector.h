#ifndef NULL_DEREF_DETECTOR_H
#define NULL_DEREF_DETECTOR_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <vector>
#include <string>
#include <map>
#include <set>

namespace safecpp {

struct NullDerefViolation {
    std::string variable;
    unsigned int null_assign_line;
    unsigned int deref_line;
    std::string message;
};

class NullDerefDetector : public clang::RecursiveASTVisitor<NullDerefDetector> {
public:
    explicit NullDerefDetector(clang::ASTContext *context);
    
    bool VisitVarDecl(clang::VarDecl *decl);
    bool VisitBinaryOperator(clang::BinaryOperator *op);
    bool VisitUnaryOperator(clang::UnaryOperator *op);
    bool VisitMemberExpr(clang::MemberExpr *expr);
    bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr);
    
    std::vector<NullDerefViolation> getViolations() const;
    void report(const std::vector<NullDerefViolation>& violations);
    
private:
    clang::ASTContext *context_;
    std::map<std::string, const clang::Stmt*> null_assignments_;
    std::vector<NullDerefViolation> violations_;
    
    void trackNullAssignment(const std::string& var_name, const clang::Stmt* stmt);
    void checkNullDereference(const std::string& var_name, const clang::Stmt* stmt);
    bool isNullPointer(const clang::Expr* expr);
    std::string getVarName(const clang::Expr* expr);
};

} // namespace safecpp

#endif // NULL_DEREF_DETECTOR_H
