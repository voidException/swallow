#ifndef AST_H
#define AST_H
#include "unary-operator.h"
#include "binary-operator.h"
#include "in-out-parameter.h"


#include "literal-nodes.h"
#include "integer-literal.h"
#include "float-literal.h"
#include "string-literal.h"
#include "compile-constant.h"

#include "array-literal.h"
#include "dictionary-literal.h"

#include "identifier.h"
#include "comment.h"
#include "member-access.h"
#include "subscript-access.h"


#include "type-check.h"
#include "type-cast.h"
#include "assignment.h"
#include "conditional-operator.h"
#include "parenthesized-expression.h"
#include "function-call.h"
#include "initializer-reference.h"
#include "self-expression.h"
#include "dynamic-type.h"
#include "forced-value.h"
#include "optional-chaining.h"
#include "closure-expression.h"

#include "ast/break-statement.h"
#include "ast/code-block.h"
#include "ast/continue-statement.h"
#include "ast/do-loop.h"
#include "ast/fallthrough-statement.h"
#include "ast/for-loop.h"
#include "ast/if-statement.h"
#include "ast/labeled-statement.h"
#include "ast/return-statement.h"
#include "ast/statement.h"
#include "ast/switch-case.h"
#include "ast/case-statement.h"
#include "ast/while-loop.h"

#include "ast/value-binding.h"
#include "ast/tuple.h"

#include "ast/type-node.h"
#include "ast/tuple-type.h"
#include "ast/type-identifier.h"
#include "ast/function-type.h"
#include "ast/array-type.h"
#include "ast/optional-type.h"
#include "ast/implicitly-unwrapped-optional.h"
#include "ast/protocol-composition.h"
#include "ast/attribute.h"

#include "ast/declaration.h"

#include "ast/import.h"
#include "ast/constant.h"
#include "ast/variable.h"
#include "ast/variables.h"
#include "ast/type-alias.h"
#include "ast/function-def.h"
#include "ast/parameters.h"
#include "ast/parameter.h"
#include "ast/enum-def.h"
#include "ast/struct-def.h"
#include "ast/class-def.h"
#include "ast/protocol-def.h"
#include "ast/initializer-def.h"
#include "ast/deinitializer-def.h"
#include "ast/extension-def.h"
#include "ast/subscript-def.h"
#include "ast/operator-def.h"
#include "ast/generic-parameters.h"
#include "ast/generic-constraint.h"

#endif//AST_H
