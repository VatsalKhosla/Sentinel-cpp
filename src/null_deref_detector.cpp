#include "null_deref_detector.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include <iostream>

namespace safecpp {

NullDerefDetector::NullDerefDetector(clang::ASTContext *context) 
    : context_(context) {}

bool NullDerefDetector::TraverseIfStmt(clang::IfStmt *if_stmt) {
    if (!if_stmt) {
        return true;
    }

    std::string guarded_var;
    bool then_non_null = false;
    bool has_guard = getNullGuardInfo(if_stmt->getCond(), guarded_var, then_non_null);

    auto saved_state = null_assignments_;

    if (if_stmt->getInit()) {
        this->TraverseStmt(if_stmt->getInit());
    }
    if (if_stmt->getConditionVariableDeclStmt()) {
        this->TraverseStmt(if_stmt->getConditionVariableDeclStmt());
    }
    if (if_stmt->getCond()) {
        this->TraverseStmt(if_stmt->getCond());
    }

    if (has_guard) {
        if (then_non_null) {
            clearNullAssignment(guarded_var);
        } else {
            trackNullAssignment(guarded_var, if_stmt);
        }
    }

    if (if_stmt->getThen()) {
        this->TraverseStmt(if_stmt->getThen());
    }

    null_assignments_ = saved_state;

    if (if_stmt->getElse()) {
        if (has_guard) {
            if (then_non_null) {
                trackNullAssignment(guarded_var, if_stmt);
            } else {
                clearNullAssignment(guarded_var);
            }
        }
        this->TraverseStmt(if_stmt->getElse());
    }

    null_assignments_ = saved_state;
    return true;
}

bool NullDerefDetector::VisitVarDecl(clang::VarDecl *decl) {
    if (decl->hasInit()) {
        const clang::Expr* init = decl->getInit();
        std::string var_name = decl->getNameAsString();
        if (var_name.empty()) {
            return true;
        }

        if (isNullPointer(init)) {
            trackNullAssignment(var_name, init);
        } else {
            clearNullAssignment(var_name);
        }
    }
    return true;
}

bool NullDerefDetector::VisitBinaryOperator(clang::BinaryOperator *op) {
    if (op->getOpcode() == clang::BO_Assign) {
        std::string var_name = getVarName(op->getLHS());
        if (var_name.empty()) {
            return true;
        }

        if (isNullPointer(op->getRHS())) {
            trackNullAssignment(var_name, op);
        } else {
            clearNullAssignment(var_name);
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

void NullDerefDetector::clearNullAssignment(const std::string& var_name) {
    null_assignments_.erase(var_name);
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
    expr = expr->IgnoreParenImpCasts();
    
    if (llvm::isa<clang::CXXNullPtrLiteralExpr>(expr)) {
        return true;
    }
    
    if (const auto *intLit = llvm::dyn_cast<clang::IntegerLiteral>(expr)) {
        return intLit->getValue() == 0;
    }
    
    return false;
}

std::string NullDerefDetector::getVarName(const clang::Expr* expr) {
    expr = expr->IgnoreParenImpCasts();
    
    if (const auto *declRef = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        return declRef->getNameInfo().getAsString();
    }
    
    return "";
}

bool NullDerefDetector::getNullGuardInfo(const clang::Expr* cond, std::string& var_name, bool& then_non_null) {
    if (!cond) {
        return false;
    }

    cond = cond->IgnoreParenImpCasts();

    if (const auto *decl_ref = llvm::dyn_cast<clang::DeclRefExpr>(cond)) {
        var_name = decl_ref->getNameInfo().getAsString();
        then_non_null = true;
        return !var_name.empty();
    }

    if (const auto *unary = llvm::dyn_cast<clang::UnaryOperator>(cond)) {
        if (unary->getOpcode() == clang::UO_LNot) {
            std::string inner_var = getVarName(unary->getSubExpr());
            if (!inner_var.empty()) {
                var_name = inner_var;
                then_non_null = false;
                return true;
            }
        }
    }

    const auto *binary = llvm::dyn_cast<clang::BinaryOperator>(cond);
    if (!binary) {
        return false;
    }

    if (binary->getOpcode() != clang::BO_EQ && binary->getOpcode() != clang::BO_NE) {
        return false;
    }

    const clang::Expr *lhs = binary->getLHS();
    const clang::Expr *rhs = binary->getRHS();

    const clang::Expr *var_expr = nullptr;
    if (isNullPointer(lhs)) {
        var_expr = rhs;
    } else if (isNullPointer(rhs)) {
        var_expr = lhs;
    }

    if (!var_expr) {
        return false;
    }

    var_name = getVarName(var_expr);
    if (var_name.empty()) {
        return false;
    }

    then_non_null = (binary->getOpcode() == clang::BO_NE);
    return true;
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
