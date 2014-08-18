#ifndef TEST_TYPE_H
#define TEST_TYPE_H

#include "tests/utils.h"

using namespace Swift;

TEST(TestDeclaration, testImport)
{
    PARSE_STATEMENT(L"import Foundation");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation", import->getPath());
    ASSERT_EQ(Import::_, import->getKind());

}
TEST(TestDeclaration, testImportSubModule)
{
    PARSE_STATEMENT(L"import Foundation.SubModule");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.SubModule", import->getPath());
    ASSERT_EQ(Import::_, import->getKind());
}

TEST(TestDeclaration, testImportKind_Class)
{
    PARSE_STATEMENT(L"import class Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Class, import->getKind());
}

TEST(TestDeclaration, testImportKind_TypeAlias)
{
    PARSE_STATEMENT(L"import typealias Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Typealias, import->getKind());
}

TEST(TestDeclaration, testImportKind_Struct)
{
    PARSE_STATEMENT(L"import struct Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Struct, import->getKind());
}

TEST(TestDeclaration, testImportKind_Enum)
{
    PARSE_STATEMENT(L"import enum Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Enum, import->getKind());
}

TEST(TestDeclaration, testImportKind_Protocol)
{
    PARSE_STATEMENT(L"import protocol Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Protocol, import->getKind());
}

TEST(TestDeclaration, testImportKind_Var)
{
    PARSE_STATEMENT(L"import var Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Var, import->getKind());
}

TEST(TestDeclaration, testImportKind_Func)
{
    PARSE_STATEMENT(L"import func Foundation.NSFileManager");
    ImportPtr import;
    ASSERT_NOT_NULL(import = std::dynamic_pointer_cast<Import>(root));
    ASSERT_EQ(L"Foundation.NSFileManager", import->getPath());
    ASSERT_EQ(Import::Func, import->getKind());
}

TEST(TestDeclaration, testLet)
{
    PARSE_STATEMENT(L"let a : Int[] = [1, 2, 3]");
    ConstantsPtr c;
    IdentifierPtr id;
    ArrayLiteralPtr value;
    ArrayTypePtr type;
    TypeIdentifierPtr Int;
    ASSERT_NOT_NULL(c = std::dynamic_pointer_cast<Constants>(root));
    ASSERT_EQ(1, c->numConstants());
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(c->getConstant(0)->name));
    ASSERT_EQ(L"a", id->getIdentifier());
    ASSERT_NOT_NULL(type = std::dynamic_pointer_cast<ArrayType>(id->getDeclaredType()));
    ASSERT_NOT_NULL(Int = std::dynamic_pointer_cast<TypeIdentifier>(type->getInnerType()));
    ASSERT_EQ(L"Int", Int->getName());

    ASSERT_NOT_NULL(value = std::dynamic_pointer_cast<ArrayLiteral>(c->getConstant(0)->initializer));
    ASSERT_EQ(3, value->numElements());
    ASSERT_EQ(L"1", std::dynamic_pointer_cast<IntegerLiteral>(value->getElement(0))->valueAsString);
    ASSERT_EQ(L"2", std::dynamic_pointer_cast<IntegerLiteral>(value->getElement(1))->valueAsString);
    ASSERT_EQ(L"3", std::dynamic_pointer_cast<IntegerLiteral>(value->getElement(2))->valueAsString);

}

TEST(TestDeclaration, testLet_Multiple)
{
    PARSE_STATEMENT(L"let a=[k1 : 1, k2 : 2], b : Int = 2");
    ConstantsPtr c;
    IdentifierPtr id;
    TypeIdentifierPtr Int;
    DictionaryLiteralPtr dict;
    ASSERT_NOT_NULL(c = std::dynamic_pointer_cast<Constants>(root));
    ASSERT_EQ(2, c->numConstants());
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(c->getConstant(0)->name));
    ASSERT_EQ(L"a", id->getIdentifier());
    ASSERT_NOT_NULL(dict = std::dynamic_pointer_cast<DictionaryLiteral>(c->getConstant(0)->initializer));
    ASSERT_EQ(2, dict->numElements());

    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(c->getConstant(1)->name));
    ASSERT_EQ(L"b", id->getIdentifier());
    ASSERT_NOT_NULL(Int = std::dynamic_pointer_cast<TypeIdentifier>(id->getDeclaredType()));
    ASSERT_EQ(L"Int", Int->getName());

}

TEST(TestDeclaration, testLet_Tuple)
{
    PARSE_STATEMENT(L"let (a, b) : Int = (1, 2)");
    ConstantsPtr c;
    TuplePtr tuple;
    TypeIdentifierPtr type;
    ParenthesizedExpressionPtr p;
    ASSERT_NOT_NULL(c = std::dynamic_pointer_cast<Constants>(root));
    ASSERT_NOT_NULL(tuple = std::dynamic_pointer_cast<Tuple>(c->getConstant(0)->name));
    ASSERT_EQ(2, tuple->numElements());
    ASSERT_NOT_NULL(type = std::dynamic_pointer_cast<TypeIdentifier>(tuple->getDeclaredType()));
    ASSERT_NOT_NULL(p = std::dynamic_pointer_cast<ParenthesizedExpression>(c->getConstant(0)->initializer));
    ASSERT_EQ(2, p->numExpressions());

}

TEST(TestDeclaration, testVar)
{
    PARSE_STATEMENT(L"var currentLoginAttempt = 0");
    VariablesPtr vars;
    VariablePtr var;
    IdentifierPtr id;
    IntegerLiteralPtr i;

    ASSERT_NOT_NULL(vars = std::dynamic_pointer_cast<Variables>(root));
    ASSERT_EQ(1, vars->numVariables());
    ASSERT_NOT_NULL(var = std::dynamic_pointer_cast<Variable>(vars->getVariable(0)));
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(var->getName()));
    ASSERT_EQ(L"currentLoginAttempt", id->getIdentifier());

    ASSERT_NOT_NULL(i = std::dynamic_pointer_cast<IntegerLiteral>(var->getInitializer()));
    ASSERT_EQ(L"0", i->valueAsString);


}
TEST(TestDeclaration, testVar_Multiple)
{
    PARSE_STATEMENT(L"var x = 0.0, y = 0.0, z = 0.0");
    VariablesPtr vars;
    VariablePtr var;
    IdentifierPtr id;
    FloatLiteralPtr f;

    ASSERT_NOT_NULL(vars = std::dynamic_pointer_cast<Variables>(root));
    ASSERT_EQ(3, vars->numVariables());

    ASSERT_NOT_NULL(var = std::dynamic_pointer_cast<Variable>(vars->getVariable(0)));
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(var->getName()));
    ASSERT_EQ(L"x", id->getIdentifier());
    ASSERT_NOT_NULL(f = std::dynamic_pointer_cast<FloatLiteral>(var->getInitializer()));
    ASSERT_EQ(L"0.0", f->valueAsString);


    ASSERT_NOT_NULL(var = std::dynamic_pointer_cast<Variable>(vars->getVariable(1)));
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(var->getName()));
    ASSERT_EQ(L"y", id->getIdentifier());
    ASSERT_NOT_NULL(f = std::dynamic_pointer_cast<FloatLiteral>(var->getInitializer()));
    ASSERT_EQ(L"0.0", f->valueAsString);


    ASSERT_NOT_NULL(var = std::dynamic_pointer_cast<Variable>(vars->getVariable(2)));
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(var->getName()));
    ASSERT_EQ(L"z", id->getIdentifier());
    ASSERT_NOT_NULL(f = std::dynamic_pointer_cast<FloatLiteral>(var->getInitializer()));
    ASSERT_EQ(L"0.0", f->valueAsString);


}

TEST(TestDeclaration, testVar_Typed)
{
    PARSE_STATEMENT(L"var welcomeMessage: String");
    VariablesPtr vars;
    VariablePtr var;
    IdentifierPtr id;
    TypeIdentifierPtr t;

    ASSERT_NOT_NULL(vars = std::dynamic_pointer_cast<Variables>(root));
    ASSERT_EQ(1, vars->numVariables());
    ASSERT_NOT_NULL(var = std::dynamic_pointer_cast<Variable>(vars->getVariable(0)));
    ASSERT_NOT_NULL(id = std::dynamic_pointer_cast<Identifier>(var->getName()));
    ASSERT_EQ(L"welcomeMessage", id->getIdentifier());

    ASSERT_NOT_NULL(t = std::dynamic_pointer_cast<TypeIdentifier>(var->getDeclaredType()));
    ASSERT_EQ(L"String", t->getName());


}

TEST(TestDeclaration, testOperator)
{
    PARSE_STATEMENT(L"operator infix +- { associativity left precedence 140 }");
    OperatorDefPtr o;
    ASSERT_NOT_NULL(o = std::dynamic_pointer_cast<OperatorDef>(root));
    ASSERT_EQ(L"+-", o->getName());
    ASSERT_EQ(Associativity::Left, o->getAssociativity());
    ASSERT_EQ(140, o->getPrecedence());


}

    


#endif