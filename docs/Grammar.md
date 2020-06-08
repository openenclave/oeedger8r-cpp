# EDL Grammar

This document describes the grammar for the EDL language in [Backus-Naur Form](https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form).

Though it is based on the Intel SDK's Ocaml Yacc input file
[Parser.mly](https://github.com/intel/linux-sgx/blob/master/sdk/edger8r/linux/Parser.mly),
it more accurrately mirrors oedger8r's hand-written EDL Parser
([parser.cpp](https://github.com/openenclave/oeedger8r-cpp/blob/master/parser.cpp)).

# Conventions

Keywords in EDL are enclosed in double quotes.
E.g: `"enclave" "import"`.

Operators in EDL are are represented in single quotes.
E.g: `'{' ',' '*'`.

Comments and white-spaces are assumed to be removed by the lexer.

In addition to keywords and operators, the lexer recognizes the following tokens
- `identifier` A sequence of alphanumeric characters or '_' that starts with a character or '_'.
   Valid identifiers in EDL are valid C language identifiers.
- `integer` A sequence of digits. EDL does not support octal and hexadecimal integers.
- `string` A C-style string literal.

Each `non-termnial` is defined via `production` of the form
```c
     non-termnial = expression
```
where `expression` consists of one or more `non-terminal`s or tokens.


`+` Indicates one or more occurence of the preceding symbol.

`*` Indicates zero or more occurence of the preceding symbol.

`[]` Indicates that the enclosed item is optional.

`()` Paranthesis is used to group items.

`|` Indicates a choice between different productions.

## Grammar

```c

enclave_definition :  "enclave" '{' enclave_item* '}' EOF

enclave_item :
    include_statement
    | import_statement
    | from_import_statement
    | enum_declaration
    | struct_declaration
    | union_declaration
    | trusted_section
    | untrusted_section

include_statement: "include" string

import_statement: "import" string

from_import_statement: "from" string "import" imported_items ';'

imported_items =
    '*'
    | identifier_list

identifier_list = identifier ( ',' identifier )*

enum_declaration =
    "enum" [identifier] '{' enumeration_item ( ',' enumeration_item )* '}' ';'

enumeration_item =
    | identifier = integer | identifier
    | identifier

struct_declaration = "struct" '{' ( declaration ';' ) * '}' ';'

union_declaration = "union" '{' ( declaration ';' ) * '}' ';'

declaration = [attributes] type_expr identifier [array_dimension+]

attributes = '[' attribute ( ',' attribute )* ']'

attribute =
    "in" | "out" | "string" | "wstring" | "isptr" | "isary" | "user_check" | count_or_size

count_or_size =
      "count" '=' attribute_value
    | "size" '=' attribute_value

attribute_value = integer | identifier

type_expr = atype [pointer]

atype = ["const"] (atype1 | atype2)

atype1 = "unsigned" integer_type
    | integer_type
    | "long" "double"

integer_type =
    | "long" "long" ["int"]
    | "long" ["int"]
    | "short" ["int"]
    | "char" ["int"]
    | "int"

atype2 =
    | "enum" identifier
    | "struct" identifier
    | "union" identifier
    | "bool"
    | "void"
    | "wchar_t"
    | "size_t"
    | "int8_t"
    | "int16_t"
    | "int32_t"
    | "int64_t"
    | "uint8_t"
    | "uint16_t"
    | "uint32_t"
    | "uint64_t"
    | "float"
    | "double"

pointer =
    '*'
    | '*' pointer

array_dimension = '[' (integer | identifier) ']'

trusted_section = "trusted" '{' trusted_function* '}' ';'

trusted_function =
    "public" function_declaration [allow_list] [trusted_suffixes] ';'

allow_list = "allow" '(' identifier_list ')'

untrusted_section =
    "untrusted" '{' untrusted_function* '}' [untrusted_suffixes] ';'

function_declaration = atype identifier parameter_list

parameter_list =
    '(' "void" ')'
    | '(' ')'
    | '(' declaration  ( ',' declaration )* ')'

trusted_suffixes = "transition_using_threads"

untrusted_suffixes = "transition_using_threads" | "propagate_errno"


```
