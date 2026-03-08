#include "null_deref_detector.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/SourceManager.h"
#include <iostream>

namespace safecpp {

NullDerefDetector::NullDerefDetector(clang::ASTContext *context) 
    : context_(context) {}

bool NullDerefDetector::VisitVarDecl(clang::VarDecl *decl) {
    if (decl->hasInit()) {
        const clang::Expr* init = decl->getInit();
        if (isNullPointer(init)) {
            std::string var_name = decl->getNameAsString();
            if (!var_name.empty()) {
                trackNullAssignment(var_name, init);
            }
        }
    }
    return true;
}

bool NullDerefDetector::VisitBinaryOperator(clang::BinaryOperator *op) {
    if (op->getOpcode() == clang::BO_Assign) {
        if (isNullPointer(op->getRHS())) {
            std::string var_name = getVarName(op->getLHS());
            if (!var_name.empty()) {
                trackNullAssignment(var_name, op);
            }
        }
    }
    return true;
}

bool NullDerefDetector::VisitUnaryOperator(clang::UnaryOperator *op) {
    if (op->getOpcode() == clang::UO_Deref) {
        std::string var_name = getVarName(op->getSubExpr());
        if (!var_name.empty()) {
            checkNullDereference(var_name, op);
        }
    }
    return true;
}

bool NullDerefDetector::VisitMemberExpr(clang::MemberExpr *expr) {
    if (expr->isArrow()) {
        std::string var_name = getVarName(expr->getBase());
        if (!var_name.empty()) {
            checkNullDereference(var_name, expr);
        }
    }
    return true;
}

bool NullDerefDetector::VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr) {
    std::string var_name = getVarName(expr->getBase());
    if (!var_name.empty()) {
        checkNullDereference(var_name, expr);
    }
    return true;
}

void NullDerefDetector::trackNullAssignment(const std::string& var_name, const clang::Stmt* stmt) {
    null_assignments_[var_name] = stmt;
}

void NullDerefDetector::checkNullDereference(const std::string& var_name, const clang::Stmt* stmt) {
    if (null_assignments_.find(var_name) != null_assignments_.end()) {
        NullDerefViolation violation;
        violation.variable = var_name;
        
        const clang::SourceManager& sm = context_->getSourceManager();
        
        clang::SourceLocation null_loc = null_assignments_[var_name]->getBeginLoc();
        violation.null_assign_line = sm.getSpellingLineNumber(null_loc);
        
        clang::SourceLocation deref_loc = stmt->getBeginLoc();
        violation.deref_line = sm.getSpellingLineNumber(deref_loc);
        
        violation.message = "Null pointer dereference: variable '" + var_name + 
                           "' may be null";
        violations_.push_back(violation);
    }
}

bool NullDerefDetector::isNullPointer(const clang::Expr* expr) {
    expr = expr->IgnoreParenCasts();
    
    if (llvm::isa<clang::CXXNullPtrLiteralExpr>(expr)) {
        return true;
    }
    
    if (const auto *intLit = llvm::dyn_cast<clang::IntegerLiteral>(expr)) {
        return intLit->getValue() == 0;
    }
    
    return false;
}

std::string NullDerefDetector::getVarName(const clang::Expr* expr) {
    expr = expr->IgnoreParenCasts();
    
    if (const auto *declRef = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        return declRef->getNameInfo().getAsString();
    }
    
    return "";
}

std::vector<NullDerefViolation> NullDerefDetector::getViolations() const {
    return violations_;
}

void NullDerefDetector::report(const std::vector<NullDerefViolation>& violations) {
    if (violations.empty()) {
        return;
    }
    
    std::cout << "\n=== Null Pointer Dereference Detection Results ===\n\n";
    
    for (const auto& violation : violations) {
        std::cout << "error: null pointer dereference\n";
        std::cout << "  Variable: " << violation.variable << "\n";
        std::cout << "  Assigned null at line: " << violation.null_assign_line << "\n";
        std::cout << "  Dereferenced at line: " << violation.deref_line << "\n";
        std::cout << "  " << violation.message << "\n\n";
    }
    
    std::cout << "Found " << violations.size() << " null pointer dereference(s)\n";
}

} // namespace safecpp
