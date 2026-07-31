; LTSL highlights for tree-sitter / Zed.
; Copyright (C) 2025  darkoned12000
; SPDX-License-Identifier: GPL-3.0-or-later

; comments
(comment) @comment

; strings
(string) @string

; numbers
(number) @number

; declaration keywords
["function" "type" "var" "ref" "static"] @keyword

; control-flow / special-form keywords
["if" "for" "while" "switch" "case" "otherwise"
 "return" "cast" "block" "desc" "call" "else"] @keyword

; literals and builtin constants
((word) @constant.builtin
  (#match? @constant.builtin "^(true|false|null|nil|Pi|2Pi|Self|This)$"))

; operators
(operator) @operator

; parens
(paren_group "(" @punctuation.bracket ")" @punctuation.bracket)

; function definitions: `function ReturnType Name (params)`
(function_definition
  return_type: (_) @type
  name: (word) @function)

; type declarations
(type_definition
  name: (word) @type)

; variable / ref / static declarations
(variable_declaration
  name: (word) @variable)

; capitalized identifiers read as types / constructors
((word) @type
  (#match? @type "^[A-Z]"))

; everything else is a plain identifier
((word) @variable
  (#not-match? @variable "^[A-Z]"))
