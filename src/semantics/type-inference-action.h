#ifndef TYPE_INFERENCE_ACTION_H
#define TYPE_INFERENCE_ACTION_H
#include "swift_conf.h"
#include "ast/node-visitor.h"
#include "semantic-node-visitor.h"
SWIFT_NS_BEGIN


class Type;
class Expression;
class TypeInferenceAction : public SemanticNodeVisitor
{
public:
    TypeInferenceAction(SymbolRegistry* symbolRegistry, CompilerResults* compilerResults);
public:
    void visitFunctionCall(const IdentifierPtr& name, const FunctionCallPtr& node);
    virtual void visitFunctionCall(const FunctionCallPtr& node);
    virtual void visitOperator(const OperatorDefPtr& node);
    virtual void visitVariable(const VariablePtr& node);
    virtual void visitConstant(const ConstantPtr& node);
    virtual void visitString(const StringLiteralPtr& node);
    virtual void visitInteger(const IntegerLiteralPtr& node);
    virtual void visitFloat(const FloatLiteralPtr& node);
    virtual void visitConditionalOperator(const ConditionalOperatorPtr& node);
    virtual void visitBinaryOperator(const BinaryOperatorPtr& node);
    virtual void visitUnaryOperator(const UnaryOperatorPtr& node);
    virtual void visitTuple(const TuplePtr& node);
    virtual void visitIdentifier(const IdentifierPtr& node);
    virtual void visitCompileConstant(const CompileConstantPtr& node);
private:
    void checkTupleDefinition(const TuplePtr& tuple, const ExpressionPtr& initializer);
    TypePtr evaluateType(const ExpressionPtr& expr);

    /**
     * Calculates the fit score of arguments on given function
     * @return -1 if the type is not matched
     */
    float calculateFitScore(const FunctionSymbolPtr& func, const ParenthesizedExpressionPtr& arguments, bool supressErrors);
private:
    TypePtr t_int;
    TypePtr t_double;
    TypePtr t_string;
    TypePtr t_bool;

};


SWIFT_NS_END


#endif//TYPE_INFERENCE_ACTION_H