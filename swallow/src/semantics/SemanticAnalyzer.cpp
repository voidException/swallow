/* SemanticAnalyzer.cpp --
 *
 * Copyright (c) 2014, Lex Chou <lex at chou dot it>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Swallow nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "semantics/SemanticAnalyzer.h"
#include "semantics/InitializationTracer.h"
#include <cassert>
#include "semantics/SymbolScope.h"
#include "ast/ast.h"
#include "common/Errors.h"
#include "semantics/GenericDefinition.h"
#include "semantics/GenericArgument.h"
#include <string>
#include "semantics/TypeBuilder.h"
#include "common/CompilerResults.h"
#include "semantics/SymbolRegistry.h"
#include "ast/utils/NodeSerializer.h"
#include "semantics/GlobalScope.h"
#include "ast/NodeFactory.h"
#include "semantics/FunctionOverloadedSymbol.h"
#include "semantics/FunctionSymbol.h"
#include "common/ScopedValue.h"
#include <set>
#include <semantics/Symbol.h>
#include "semantics/DeclarationAnalyzer.h"

USE_SWALLOW_NS
using namespace std;


SemanticAnalyzer::SemanticAnalyzer(SymbolRegistry* symbolRegistry, CompilerResults* compilerResults)
:SemanticPass(symbolRegistry, compilerResults)
{
    declarationAnalyzer = new DeclarationAnalyzer(this, &ctx);
    lazyDeclaration = true;
}
SemanticAnalyzer::~SemanticAnalyzer()
{
    delete declarationAnalyzer;
}

std::wstring SemanticAnalyzer::generateTempName()
{
    std::wstringstream ss;
    ss<<L"#"<<ctx.numTemporaryNames++;
    return ss.str();
}

/*!
 * Check if the declaration is declared as global
 */
bool SemanticAnalyzer::isGlobal(const DeclarationPtr& node)
{
    NodePtr parent = node->getParentNode();
    bool ret(parent == nullptr || parent->getNodeType() == NodeType::Program);
    return ret;
}

static std::wstring getDeclarationName(const DeclarationPtr& node)
{
    if(FunctionDefPtr func = dynamic_pointer_cast<FunctionDef>(node))
    {
        return func->getName();
    }
    if(TypeDeclarationPtr type = dynamic_pointer_cast<TypeDeclaration>(node))
    {
        return type->getIdentifier()->getName();
    }
    return L"";
}
/*!
  * Mark this declaration node as lazy declaration, it will be processed only when being used or after the end of the program.
  */
void SemanticAnalyzer::delayDeclare(const DeclarationPtr& node)
{
    //map<wstring, list<DeclarationPtr>> lazyDeclarations;
    std::wstring name = getDeclarationName(node);
    auto iter = lazyDeclarations.find(name);
    if(iter == lazyDeclarations.end())
    {
        list<DeclarationPtr> decls = {node};
        lazyDeclarations.insert(make_pair(name, decls));
        return;
    }
    iter->second.push_back(node);
}
/*!
 * The declarations that marked as lazy will be declared immediately
 */
void SemanticAnalyzer::declareImmediately(const std::wstring& name)
{
    auto entry = lazyDeclarations.find(name);
    if(entry == lazyDeclarations.end())
        return;
    list<DeclarationPtr> decls;
    std::swap(decls, entry->second);
    if(lazyDeclaration)
        lazyDeclarations.erase(entry);
    SymbolScope* currentScope = symbolRegistry->getCurrentScope();
    symbolRegistry->setCurrentScope(symbolRegistry->getFileScope());
    try
    {

        while (!decls.empty())
        {
            DeclarationPtr decl = decls.front();
            decls.pop_front();
            decl->accept(this);
        }
    }
    catch(...)
    {
        symbolRegistry->setCurrentScope(currentScope);
        throw;
    }
    symbolRegistry->setCurrentScope(currentScope);
}
void SemanticAnalyzer::visitProgram(const ProgramPtr& node)
{
    InitializationTracer tracer(nullptr, InitializationTracer::Sequence);
    SCOPED_SET(ctx.currentInitializationTracer, &tracer);

    SemanticPass::visitProgram(node);
    //now we'll deal with the lazy declaration of functions and classes
    lazyDeclaration = false;
    while(!lazyDeclarations.empty())
    {
        auto entry = lazyDeclarations.begin();
        list<DeclarationPtr>& decls = entry->second;
        while(!decls.empty())
        {
            DeclarationPtr decl = decls.front();
            decls.pop_front();
            decl->accept(this);
        }
        lazyDeclarations.erase(entry);
    }
}


TypePtr SemanticAnalyzer::lookupType(const TypeNodePtr& type, bool supressErrors)
{
    if(!type)
        return nullptr;
    TypePtr ret = type->getType();
    if(!ret)
    {
        ret = lookupTypeImpl(type, supressErrors);
        type->setType(ret);
    }
    return ret;
}
TypePtr SemanticAnalyzer::lookupTypeImpl(const TypeNodePtr &type, bool supressErrors)
{
    NodeType::T nodeType = type->getNodeType();
    switch(nodeType)
    {

        case NodeType::TypeIdentifier:
        {
            TypeIdentifierPtr id = std::static_pointer_cast<TypeIdentifier>(type);
            declareImmediately(id->getName());
            //TODO: make a generic type if possible
            TypePtr ret = symbolRegistry->lookupType(id->getName());
            if (!ret)
            {
                if (!supressErrors)
                {
                    std::wstring str = toString(type);
                    error(type, Errors::E_USE_OF_UNDECLARED_TYPE_1, str);
                    abort();
                }
                return nullptr;
            }
            GenericDefinitionPtr generic = ret->getGenericDefinition();
            if (generic == nullptr && id->numGenericArguments() == 0)
                return ret;
            if (generic == nullptr && id->numGenericArguments() > 0)
            {
                if (!supressErrors)
                {
                    std::wstring str = toString(type);
                    error(id, Errors::E_CANNOT_SPECIALIZE_NON_GENERIC_TYPE_1, str);
                }
                return nullptr;
            }
            if (generic != nullptr && id->numGenericArguments() == 0)
            {
                if (!supressErrors)
                {
                    std::wstring str = toString(type);
                    error(id, Errors::E_GENERIC_TYPE_ARGUMENT_REQUIRED, str);
                }
                return nullptr;
            }
            if (id->numGenericArguments() > generic->numParameters())
            {
                if (!supressErrors)
                {
                    std::wstring str = toString(type);
                    std::wstring got = toString(id->numGenericArguments());
                    std::wstring expected = toString(generic->numParameters());
                    error(id, Errors::E_GENERIC_TYPE_SPECIALIZED_WITH_TOO_MANY_TYPE_PARAMETERS_3, str, got, expected);
                }
                return nullptr;
            }
            if (id->numGenericArguments() < generic->numParameters())
            {
                if (!supressErrors)
                {
                    std::wstring str = toString(type);
                    std::wstring got = toString(id->numGenericArguments());
                    std::wstring expected = toString(generic->numParameters());
                    error(id, Errors::E_GENERIC_TYPE_SPECIALIZED_WITH_INSUFFICIENT_TYPE_PARAMETERS_3, str, got, expected);
                }
                return nullptr;
            }
            //check type
            GenericArgumentPtr genericArgument(new GenericArgument(generic));
            for (auto arg : *id)
            {
                TypePtr argType = lookupType(arg);
                if (!argType)
                    return nullptr;
                genericArgument->add(argType);
            }
            TypePtr base = Type::newSpecializedType(ret, genericArgument);
            ret = base;
            //access rest nested types
            for (TypeIdentifierPtr n = id->getNestedType(); n != nullptr; n = n->getNestedType())
            {
                TypePtr childType = ret->getAssociatedType(n->getName());
                if (!childType)
                {
                    if (!supressErrors)
                        error(n, Errors::E_A_IS_NOT_A_MEMBER_TYPE_OF_B_2, n->getName(), ret->toString());
                    return nullptr;
                }
                if (n->numGenericArguments())
                {
                    //nested type is always a non-generic type
                    if (!supressErrors)
                        error(n, Errors::E_CANNOT_SPECIALIZE_NON_GENERIC_TYPE_1, childType->toString());
                    return nullptr;
                }
                ret = childType;
            }

            return ret;
        }
        case NodeType::TupleType:
        {
            TupleTypePtr tuple = std::static_pointer_cast<TupleType>(type);
            std::vector<TypePtr> elementTypes;
            for (const TupleType::TupleElement &e : *tuple)
            {
                TypePtr t = lookupType(e.type);
                elementTypes.push_back(t);
            }
            return Type::newTuple(elementTypes);
        }
        case NodeType::ArrayType:
        {
            ArrayTypePtr array = std::static_pointer_cast<ArrayType>(type);
            GlobalScope *global = symbolRegistry->getGlobalScope();
            TypePtr innerType = lookupType(array->getInnerType());
            assert(innerType != nullptr);
            TypePtr ret = global->makeArray(innerType);
            return ret;
        }
        case NodeType::DictionaryType:
        {
            DictionaryTypePtr array = std::static_pointer_cast<DictionaryType>(type);
            GlobalScope *scope = symbolRegistry->getGlobalScope();
            TypePtr keyType = lookupType(array->getKeyType());
            TypePtr valueType = lookupType(array->getValueType());
            assert(keyType != nullptr);
            assert(valueType != nullptr);
            TypePtr ret = scope->makeDictionary(keyType, valueType);
            return ret;
        }
        case NodeType::OptionalType:
        {
            OptionalTypePtr opt = std::static_pointer_cast<OptionalType>(type);
            GlobalScope *global = symbolRegistry->getGlobalScope();
            TypePtr innerType = lookupType(opt->getInnerType());
            assert(innerType != nullptr);
            TypePtr ret = global->makeOptional(innerType);
            return ret;
        }
        case NodeType::ImplicitlyUnwrappedOptional:
        {
            ImplicitlyUnwrappedOptionalPtr opt = std::static_pointer_cast<ImplicitlyUnwrappedOptional>(type);
            GlobalScope *global = symbolRegistry->getGlobalScope();
            TypePtr innerType = lookupType(opt->getInnerType());
            assert(innerType != nullptr);
            TypePtr ret = global->makeImplicitlyUnwrappedOptional(innerType);
            return ret;
        };
        case NodeType::FunctionType:
        {
            FunctionTypePtr func = std::static_pointer_cast<FunctionType>(type);
            TypePtr retType = nullptr;
            if (func->getReturnType())
            {
                retType = lookupType(func->getReturnType());
            }
            vector<Parameter> params;
            for (auto p : *func->getArgumentsType())
            {
                TypePtr paramType = lookupType(p.type);
                params.push_back(Parameter(p.name, p.inout, paramType));
            }
            TypePtr ret = Type::newFunction(params, retType, false, nullptr);
            return ret;
        }
        case NodeType::ProtocolComposition:
        {
            ProtocolCompositionPtr composition = static_pointer_cast<ProtocolComposition>(type);
            vector<TypePtr> protocols;
            for(const TypeIdentifierPtr& p : *composition)
            {
                TypePtr protocol = lookupType(p);
                //it must be a protocol type
                assert(protocol != nullptr);
                if(protocol->getCategory() != Type::Protocol)
                {
                    error(p, Errors::E_NON_PROTOCOL_TYPE_A_CANNOT_BE_USED_WITHIN_PROTOCOL_COMPOSITION_1, p->getName());
                    return nullptr;
                }
                protocols.push_back(protocol);
            }
            TypePtr ret = Type::newProtocolComposition(protocols);
            return ret;
        };
        default:
        {
            assert(0 && "Unsupported type");
            return nullptr;
        }
    }
}
/*!
 * Convert expression node to type node
 */
TypeNodePtr SemanticAnalyzer::expressionToType(const ExpressionPtr& expr)
{
    assert(expr != nullptr);
    NodeType::T nodeType = expr->getNodeType();
    switch(nodeType)
    {
        case NodeType::Identifier:
        {
            IdentifierPtr id = static_pointer_cast<Identifier>(expr);
            TypeIdentifierPtr ret = id->getNodeFactory()->createTypeIdentifier(*id->getSourceInfo());
            ret->setName(id->getIdentifier());
            if(id->getGenericArgumentDef())
            {
                for(const TypeNodePtr& type : *id->getGenericArgumentDef())
                {
                    ret->addGenericArgument(type);
                }
            }
            return ret;
        }
        case NodeType::MemberAccess:
        {
            MemberAccessPtr memberAccess = static_pointer_cast<MemberAccess>(expr);
            TypeIdentifierPtr self = dynamic_pointer_cast<TypeIdentifier>(expressionToType(memberAccess->getSelf()));
            assert(memberAccess->getField() != nullptr);
            TypeIdentifierPtr tail = dynamic_pointer_cast<TypeIdentifier>(expressionToType(memberAccess->getField()));
            if(!self || !tail)
                return nullptr;
            //append the tail to the end of the expression
            TypeIdentifierPtr p = self;
            while(p->getNestedType())
            {
                p = p->getNestedType();
            }
            p->setNestedType(tail);
            return self;
        }
        case NodeType::Tuple:
        {
            TuplePtr tuple = static_pointer_cast<Tuple>(expr);
            TupleTypePtr ret = tuple->getNodeFactory()->createTupleType(*tuple->getSourceInfo());
            for(const PatternPtr& pattern : *tuple)
            {
                ExpressionPtr expr = dynamic_pointer_cast<Expression>(pattern);
                TypeNodePtr t = expressionToType(expr);
                if(!t)
                    return nullptr;
                ret->add(false, L"", t);
            }
            return ret;
        }
        default:
            return nullptr;
    }
}

/*!
 * Expand given expression to given Optional<T> type by adding implicit Optional<T>.Some calls
 * Return false if the given expression cannot be conform to given optional type
 */
bool SemanticAnalyzer::expandOptional(const TypePtr& optionalType, ExpressionPtr& expr)
{
    GlobalScope* global = symbolRegistry->getGlobalScope();
    TypePtr exprType = expr->getType();
    assert(exprType != nullptr);
    if(exprType->canAssignTo(optionalType))
        return true;
    if(optionalType->getCategory() == Type::Specialized && (optionalType->getInnerType() == global->Optional() || optionalType->getInnerType() == global->ImplicitlyUnwrappedOptional()))
    {
        TypePtr innerType = optionalType->getGenericArguments()->get(0);
        bool ret = expandOptional(innerType, expr);
        if(!ret)
            return ret;
        //wrap it with an Optional
        NodeFactory* factory = expr->getNodeFactory();
        MemberAccessPtr ma = factory->createMemberAccess(*expr->getSourceInfo());
        IdentifierPtr Some = factory->createIdentifier(*expr->getSourceInfo());
        Some->setIdentifier(L"Some");
        ma->setType(optionalType);
        ma->setField(Some);
        ParenthesizedExpressionPtr args = factory->createParenthesizedExpression(*expr->getSourceInfo());
        args->append(expr);
        FunctionCallPtr call = factory->createFunctionCall(*expr->getSourceInfo());
        call->setFunction(ma);
        call->setArguments(args);
        call->setType(optionalType);
        expr = call;
        return ret;
    }
    else
    {
        return false;
    }
}
/*!
 * Returns the final actual type of Optional in a sequence of Optional type chain.
 * e.g. T?????? will return T
 */
TypePtr SemanticAnalyzer::finalTypeOfOptional(const TypePtr& optionalType)
{
    if(optionalType->getCategory() == Type::Specialized)
    {
        TypePtr innerType = optionalType->getInnerType();
        GlobalScope* g = symbolRegistry->getGlobalScope();
        if(innerType != g->Optional() && innerType != g->ImplicitlyUnwrappedOptional())
            return optionalType;
    }
    TypePtr inner = optionalType->getGenericArguments()->get(0);
    return finalTypeOfOptional(inner);
}
/*!
 * Expand variable access into self.XXXX if XXXX is defined in type hierachy
 */
ExpressionPtr SemanticAnalyzer::expandSelfAccess(const ExpressionPtr& expr)
{
    NodeType::T nodeType = expr->getNodeType();
    switch(nodeType)
    {
        case NodeType::Identifier:
        {
            IdentifierPtr id = static_pointer_cast<Identifier>(expr);
            const std::wstring& name = id->getIdentifier();
            SymbolPtr sym = symbolRegistry->lookupSymbol(name);
            if(sym && !sym->hasFlags(SymbolFlagMember))
                return expr;// it refers to a non-member symbol
            if(!ctx.currentFunction || ctx.currentFunction->hasFlags(SymbolFlagStatic))
                return expr;// self access will not be expanded within a static/class method
            if(!sym || sym->hasFlags(SymbolFlagMember))
            {
                //check if it's defined in super class
                TypePtr type = ctx.currentType;
                bool found = false;
                for(; type; type = type->getParentType())
                {
                    if(type->getDeclaredMember(name))
                    {
                        found = true;
                        break;
                    }
                }
                if(!found)//it's not a member, do not wrap it
                    return expr;
                //it's a member or defined in super class, wrap it as member access
                NodeFactory* factory = expr->getNodeFactory();
                IdentifierPtr self = factory->createIdentifier(*expr->getSourceInfo());
                self->setIdentifier(L"self");
                MemberAccessPtr ma = factory->createMemberAccess(*expr->getSourceInfo());
                ma->setSelf(self);
                ma->setField(id);
                return ma;
            }
            return expr;
        }
        default:
            //TODO other expressions is not supported yet.
            return expr;
    }
}


/*!
 * This will make implicit type conversion like expanding optional
 */
ExpressionPtr SemanticAnalyzer::transformExpression(const TypePtr& contextualType, ExpressionPtr expr)
{
    SCOPED_SET(ctx.contextualType, contextualType);
    expr = expandSelfAccess(expr);
    expr->accept(this);
    if(!contextualType)
        return expr;
    GlobalScope* global = symbolRegistry->getGlobalScope();
    if(global->isOptional(contextualType) || global->isImplicitlyUnwrappedOptional(contextualType))
    {
        ExpressionPtr transformed = expr;
        bool ret = expandOptional(contextualType, transformed);
        if(ret)
            return transformed;
        return expr;
    }
    //if(!expr->getType()->canAssignTo(contextualType))
    //{
    //   error(expr, Errors::E_CANNOT_CONVERT_EXPRESSION_TYPE_2, toString(expr), contextualType->toString());
    //}
    return expr;
}
/*!
 * Gets all functions from current scope to top scope with given name, if flagMasks is specified, only functions
 * with given mask will be returned
 */
std::vector<SymbolPtr> SemanticAnalyzer::allFunctions(const std::wstring& name, int flagMasks, bool allScopes)
{
    SymbolScope* scope = symbolRegistry->getCurrentScope();
    std::vector<SymbolPtr> ret;
    for(; scope; scope = scope->getParentScope())
    {
        SymbolPtr sym = scope->lookup(name);
        if(!sym)
            continue;
        if(FunctionOverloadedSymbolPtr funcs = dynamic_pointer_cast<FunctionOverloadedSymbol>(sym))
        {
            for(const FunctionSymbolPtr& func : *funcs)
            {
                if((func->getFlags() & flagMasks) == flagMasks)
                    ret.push_back(func);
            }
        }
        else if(FunctionSymbolPtr func = dynamic_pointer_cast<FunctionSymbol>(sym))
        {
            if((func->getFlags() & flagMasks) == flagMasks)
                ret.push_back(func);
        }
        else if(sym->getType() && sym->getType()->getCategory() == Type::Function)
        {
            if((sym->getFlags() & flagMasks) == flagMasks)
                ret.push_back(sym);
            else
                continue;
        }
        else if(TypePtr type = dynamic_pointer_cast<Type>(sym))
        {
            ret.push_back(type);
        }
        else
            continue;
        if(!allScopes)
            break;
    }
    return std::move(ret);
}

/*!
 * Declaration finished, added it as a member to current type or current type extension.
 */
void SemanticAnalyzer::declarationFinished(const std::wstring& name, const SymbolPtr& decl, const NodePtr& node)
{
    //verify if this has been declared in the type or its extension
    if(ctx.currentType)
    {
        int filter = FilterRecursive | FilterLookupInExtension;
        if(decl->hasFlags(SymbolFlagStatic))
            filter |= FilterStaticMember;

        SymbolPtr m = getMemberFromType(ctx.currentType, name, (MemberFilter)filter);

        if(FunctionSymbolPtr func = dynamic_pointer_cast<FunctionSymbol>(decl))
        {
            bool isFunc = dynamic_pointer_cast<FunctionSymbol>(m) || dynamic_pointer_cast<FunctionOverloadedSymbol>(m);
            if(!isFunc && m)
            {
                error(node, Errors::E_INVALID_REDECLARATION_1, name);
                return;
            }
        }
        else if(dynamic_pointer_cast<ComputedProperty>(node))
        {

        }
        else if (m)
        {
            error(node, Errors::E_INVALID_REDECLARATION_1, name);
            return;
        }
    }



    TypePtr target = ctx.currentExtension ? ctx.currentExtension : ctx.currentType;
    if(target)
    {
        TypeBuilderPtr builder = static_pointer_cast<TypeBuilder>(target);
        if(ctx.currentExtension)
        {
            decl->setFlags(SymbolFlagExtension, true);
        }
        if(name == L"init")
        {
            FunctionSymbolPtr init = dynamic_pointer_cast<FunctionSymbol>(decl);
            assert(init != nullptr);
            FunctionOverloadedSymbolPtr inits = builder->getDeclaredInitializer();
            if(!inits)
            {
                inits = FunctionOverloadedSymbolPtr(new FunctionOverloadedSymbol(name));
                builder->setInitializer(inits);
            }
            //inits->add(init);
        }
        //else
        {
            builder->addMember(name, decl);
        }
    }
}

static SymbolPtr getMember(const TypePtr& type, const std::wstring& fieldName, MemberFilter filter)
{
    if (filter & FilterStaticMember)
        return type->getDeclaredStaticMember(fieldName);
    else
        return type->getDeclaredMember(fieldName);
}
/*!
 * This implementation will try to find the member from the type, and look up from extension as a fallback.
 */
SymbolPtr SemanticAnalyzer::getMemberFromType(const TypePtr& type, const std::wstring& fieldName, MemberFilter filter, TypePtr* declaringType)
{
    SymbolPtr ret = getMember(type, fieldName, filter);
    SymbolScope* scope = this->symbolRegistry->getFileScope();
    if(!ret && scope)
    {
        if(filter & FilterLookupInExtension)
        {
            TypePtr extension = scope->getExtension(type->getName());
            if (extension)
            {
                assert(extension->getCategory() == Type::Extension);
                ret = getMember(extension, fieldName, filter);
            }
        }
    }
    if(ret && declaringType)
        *declaringType = type;
    if(!ret && (filter & FilterRecursive) && type->getParentType())
    {
        ret = getMemberFromType(type->getParentType(), fieldName, filter, declaringType);
    }
    return ret;
}

struct FunctionSymbolWrapper
{
    SymbolPtr symbol;
    FunctionSymbolWrapper(const SymbolPtr& symbol)
            :symbol(symbol)
    {}
    bool operator<(const FunctionSymbolWrapper& rhs) const
    {
        return Type::compare(symbol->getType(), rhs.symbol->getType()) < 0;
    }
};
static void addCandidateMethod(std::set<FunctionSymbolWrapper>& result, const FunctionSymbolPtr& func)
{
    auto iter = result.find(FunctionSymbolWrapper(func));
    if(iter == result.end())
        result.insert(FunctionSymbolWrapper(func));
}

/*!
 * This will extract all methods that has the same name in the given type(including all base types)
 */
static void loadMethods(const TypePtr& type, const std::wstring& fieldName, MemberFilter filter, std::set<FunctionSymbolWrapper>& result)
{
    SymbolPtr sym;
    if (filter & FilterStaticMember)
        sym = type->getDeclaredStaticMember(fieldName);
    else
        sym = type->getDeclaredMember(fieldName);
    if(!sym)
        return;
    if(FunctionOverloadedSymbolPtr funcs = dynamic_pointer_cast<FunctionOverloadedSymbol>(sym))
    {
        for(auto func : *funcs)
            addCandidateMethod(result, func);
    }
    else if(FunctionSymbolPtr func = dynamic_pointer_cast<FunctionSymbol>(sym))
    {
        addCandidateMethod(result, func);
    }
    TypePtr parent = type->getParentType();
    if(parent && (filter & FilterRecursive))
    {
       loadMethods(parent, fieldName, filter, result);
    }
}
/*!
 * This implementation will try to all methods from the type, including defined in parent class or extension
 */
void SemanticAnalyzer::getMethodsFromType(const TypePtr& type, const std::wstring& fieldName, MemberFilter filter, std::vector<SymbolPtr>& result)
{
    std::set<FunctionSymbolWrapper> functions;
    loadMethods(type, fieldName, filter, functions);
    SymbolScope* scope = this->symbolRegistry->getFileScope();
    if(scope)
    {
        TypePtr extension = scope->getExtension(type->getName());
        if(extension && (filter & FilterLookupInExtension))
        {
            assert(extension->getCategory() == Type::Extension);
            loadMethods(extension, fieldName, filter, functions);
        }
    }
    for(const FunctionSymbolWrapper& wrapper : functions)
    {
        result.push_back(wrapper.symbol);
    }
}

/*!
 * Mark specified symbol initialized.
 */
void SemanticAnalyzer::markInitialized(const SymbolPtr& sym)
{
    if(sym->hasFlags(SymbolFlagInitialized))
        return;
    sym->setFlags(SymbolFlagInitialized, true);
    assert(ctx.currentInitializationTracer != nullptr);
    ctx.currentInitializationTracer->add(sym);
}
/*!
 * Make a chain of member access on a tuple variable(specified by given tempName) by a set of indices
 */
static MemberAccessPtr makeAccess(SourceInfo* info, NodeFactory* nodeFactory, const std::wstring& tempName, const std::vector<int>& indices)
{
    assert(!indices.empty());
    IdentifierPtr self = nodeFactory->createIdentifier(*info);
    self->setIdentifier(tempName);
    ExpressionPtr ret = self;
    for(int i : indices)
    {
        MemberAccessPtr next = nodeFactory->createMemberAccess(*info);
        next->setSelf(ret);
        next->setIndex(i);
        ret = next;
    }
    return static_pointer_cast<MemberAccess>(ret);
}

void SemanticAnalyzer::expandTuple(vector<TupleExtractionResult>& results, vector<int>& indices, const PatternPtr& name, const std::wstring& tempName, const TypePtr& type, PatternAccessibility accessibility)
{
    TypeNodePtr declaredType;
    switch (name->getNodeType())
    {
        case NodeType::Identifier:
        {
            IdentifierPtr id = static_pointer_cast<Identifier>(name);
            if(accessibility == AccessibilityUndefined)
            {
                //undefined variable
                error(name, Errors::E_USE_OF_UNRESOLVED_IDENTIFIER_1, id->getIdentifier());
                abort();
                return;
            }
            name->setType(type);
            bool readonly = accessibility == AccessibilityConstant;
            results.push_back(TupleExtractionResult(id, type, makeAccess(id->getSourceInfo(), id->getNodeFactory(), tempName, indices), readonly));
            //check if the identifier already has a type definition
            break;
        }
        case NodeType::TypedPattern:
        {
            TypedPatternPtr pat = static_pointer_cast<TypedPattern>(name);
            assert(pat->getDeclaredType());
            TypePtr declaredType = lookupType(pat->getDeclaredType());
            pat->setType(declaredType);
            if(!Type::equals(declaredType, type))
            {
                error(name, Errors::E_TYPE_ANNOTATION_DOES_NOT_MATCH_CONTEXTUAL_TYPE_A_1, type->toString());
                abort();
                return;
            }
            expandTuple(results, indices, pat->getPattern(), tempName, declaredType, accessibility);
            break;
        }
        case NodeType::Tuple:
        {
            if(type->getCategory() != Type::Tuple)
            {
                error(name, Errors::E_TUPLE_PATTERN_CANNOT_MATCH_VALUES_OF_THE_NON_TUPLE_TYPE_A_1, type->toString());
                return;
            }
            TuplePtr tuple = static_pointer_cast<Tuple>(name);
            declaredType = tuple->getDeclaredType();
            if((type->getCategory() != Type::Tuple) || (tuple->numElements() != type->numElementTypes()))
            {
                error(name, Errors::E_TYPE_ANNOTATION_DOES_NOT_MATCH_CONTEXTUAL_TYPE_A_1, type->toString());
                abort();
                return;
            }
            //check each elements
            int elements = tuple->numElements();
            for(int i = 0; i < elements; i++)
            {
                PatternPtr element = tuple->getElement(i);
                TypePtr elementType = type->getElementType(i);
                indices.push_back(i);
                //validateTupleType(isReadonly, element, elementType);
                expandTuple(results, indices, element, tempName, elementType, accessibility);
                indices.pop_back();
            }
            break;
        }
        case NodeType::ValueBindingPattern:
        {
            if(accessibility != AccessibilityUndefined)
            {
                error(name, Errors::E_VARLET_CANNOT_APPEAR_INSIDE_ANOTHER_VAR_OR_LET_PATTERN_1, accessibility == AccessibilityConstant ? L"let" : L"var");
                abort();
            }
            else
            {
                ValueBindingPatternPtr p = static_pointer_cast<ValueBindingPattern>(name);
                expandTuple(results, indices, p->getBinding(), tempName, type, p->isReadOnly() ? AccessibilityConstant : AccessibilityVariable);
            }
            break;
        }
        case NodeType::EnumCasePattern:
        {
            EnumCasePatternPtr p(static_pointer_cast<EnumCasePattern>(name));
            if(p->getAssociatedBinding())
            {
                expandTuple(results, indices, p->getAssociatedBinding(), tempName, type, accessibility);
            }
            break;
        }
        case NodeType::TypeCase:
        case NodeType::TypeCheck:
        default:
            error(name, Errors::E_EXPECT_TUPLE_OR_IDENTIFIER);
            break;
    }
}
