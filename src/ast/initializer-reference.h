#ifndef INITIALIZER_REFERENCE_H
#define INITIALIZER_REFERENCE_H
#include "expression.h"

SWIFT_NS_BEGIN

class InitializerReference : public Expression
{
public:
    InitializerReference(Expression* expr);
public:
    void setExpression(Expression* expr);
    Expression* getExpression();
public:
    virtual void serialize(std::wostream& out);
};

SWIFT_NS_END

#endif//INITIALIZER_REFERENCE_H