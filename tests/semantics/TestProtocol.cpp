#include "../utils.h"
#include "semantics/Type.h"
#include "semantics/SymbolRegistry.h"
#include "semantics/Symbol.h"
#include "semantics/ScopedNodes.h"
#include "semantics/FunctionSymbol.h"
#include "swift_errors.h"


using namespace Swift;

TEST(TestProtocol, ProtocolInheritance)
{
    SEMANTIC_ANALYZE(L"protocol Base{}\n"
            "protocol Child : Base{}")
    dumpCompilerResults(compilerResults);
    ASSERT_EQ(0, compilerResults.numResults());
    TypePtr Base, Child;
    ASSERT_NOT_NULL(Base = std::dynamic_pointer_cast<Type>(scope->lookup(L"Base")));
    ASSERT_NOT_NULL(Child = std::dynamic_pointer_cast<Type>(scope->lookup(L"Child")));

}

TEST(TestProtocol, ProtocolHasNoVariable)
{
    SEMANTIC_ANALYZE(L"protocol Test\n"
            "var a : Int = 3\n"
            "}");
    dumpCompilerResults(compilerResults);
    ASSERT_EQ(1, compilerResults.numResults());
}


TEST(TestProtocol, MethodRequirement_Decl)
{
    SEMANTIC_ANALYZE(L"protocol RandomNumberGenerator {\n"
            L" func random() -> Double\n"
            L"  } ");

    dumpCompilerResults(compilerResults);
    ASSERT_EQ(0, compilerResults.numResults());
    TypePtr RandomNumberGenerator;
    ASSERT_NOT_NULL(RandomNumberGenerator = std::dynamic_pointer_cast<Type>(scope->lookup(L"RandomNumberGenerator")));
    SymbolPtr sym = RandomNumberGenerator->getSymbols()[L"random"];
    FunctionSymbolPtr random;

    ASSERT_NOT_NULL(random = std::dynamic_pointer_cast<FunctionSymbol>(sym));

}

TEST(TestProtocol, MethodRequirements_DefaultParameterInitializer)

{
    SEMANTIC_ANALYZE(L"protocol RandomNumberGenerator {\n"
            L" func random(a : Int = 3) -> Double\n"
            L"  } ");

    ASSERT_EQ(1, compilerResults.numResults());
    auto res = compilerResults.getResult(0);
    ASSERT_EQ(Errors::E_DEFAULT_ARGUMENT_NOT_PERMITTED_IN_A_PROTOCOL_METHOD, res.code);


}
TEST(TestProtocol, MethodRequirements_Impl)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol {"
            "func test()\n"
            "}\n"
            "class Test : MyProtocol { \n"
            "func test(){}\n"
            "}");
    ASSERT_EQ(0, compilerResults.numResults());
}


TEST(TestProtocol, MethodRequirements_NotImpl)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol {"
            "func test()\n"
            "}\n"
            "class Test : MyProtocol { \n"
            "}");
    ASSERT_EQ(1, compilerResults.numResults());
    auto res = compilerResults.getResult(0);
    ASSERT_EQ(Errors::E_TYPE_DOES_NOT_CONFORM_TO_PROTOCOL_UNIMPLEMENTED_FUNCTION, res.code);
}

TEST(TestProtocol, TypeRequirements)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol {\n"
            "typealias Element\n"
            "}\n"
            "class Test : MyProtocol\n"
            "{\n"
            "  class Element {}\n"
            "}\n");
    ASSERT_EQ(0, compilerResults.numResults());
}

TEST(TestProtocol, TypeRequirements_Impl)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol {\n"
            "typealias Element\n"
            "}\n"
            "class Test : MyProtocol\n"
            "{\n"
            "}\n");
    ASSERT_EQ(1, compilerResults.numResults());
    auto res = compilerResults.getResult(0);
    ASSERT_EQ(Errors::E_TYPE_DOES_NOT_CONFORM_TO_PROTOCOL_UNIMPLEMENTED_TYPE, res.code);
}

TEST(TestProtocol, TypeInheritance)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol { typealias Element = Int }\n"
            "class Test : MyProtocol {}");
    //TODO: type inference for let a : Test.Element
    ASSERT_EQ(0, compilerResults.numResults());

}

TEST(TestProtocol, Let)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol {\n"
            "let a : Int = 3\n"
            "}");
    ASSERT_EQ(1, compilerResults.numResults());
    auto res = compilerResults.getResult(0);
    ASSERT_EQ(Errors::E_PROTOCOL_CANNOT_DEFINE_LET_CONSTANT, res.code);
}

TEST(TestProtocol, Var)
{
    SEMANTIC_ANALYZE(L"protocol MyProtocol {\n"
            "var a : Int = 3\n"
            "}");
    ASSERT_EQ(1, compilerResults.numResults());
    auto res = compilerResults.getResult(0);
    ASSERT_EQ(Errors::E_PROTOCOL_VAR_MUST_BE_COMPUTED_PROPERTY, res.code);
}