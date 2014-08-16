#ifndef FUNCTION_SYMBOL_H
#define FUNCTION_SYMBOL_H
#include "swift_conf.h"
#include "symbol.h"
#include <vector>

SWIFT_NS_BEGIN
typedef std::shared_ptr<Symbol> SymbolPtr;
typedef std::shared_ptr<class FunctionDef> FunctionDefPtr;
class FunctionSymbol : public Symbol
{
    friend class FunctionOverloadedSymbol;
    friend class SymbolRegistry;
public:
    FunctionSymbol(const std::wstring& name, const TypePtr& functionType, const FunctionDefPtr& definition);
public:
    virtual const std::wstring& getName() const;
    TypePtr getReturnType()const;
    TypePtr getType();
    FunctionDefPtr getDefinition()const;
private:
    std::wstring name;
    TypePtr type;
    FunctionDefPtr definition;
};


SWIFT_NS_END
#endif//FUNCTION_SYMBOL_H