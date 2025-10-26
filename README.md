# alder
c++ transpiler for alder language

Grammer:

module      := { item } Eof
item        := func | stmt

func        := KwDef Identifier
               LParen [param { Comma param }] RParen
               Arrow type
               block

param       := type Colon Identifier
type        := Kwi64 | Kwi32 | Kwf64 | KwBool | KwChar | KwVoid

stmt        := vardecl
             | forin
             | while
             | assign
             | exprstmt
             | block

vardecl     := type Colon Identifier Assign expr [Newline]
forin       := KwFor type Colon Identifier KwIn expr block 
while       := KwWhile expr block
assign      := Identifier Assign expr [Newline]
exprstmt    := expr [Newline]
block       := LBrace { stmt } RBrace

expr        := (prefix then postfix calls then infix as in §4)
prefix      := IntLit | StringLit | BoolLit | Identifier
             | Minus prefix | KwNot prefix
             | LParen expr RParen
postfix     := ( LParen [expr { Comma expr }] RParen )*
infix       := { (Star|Slash|Plus|Minus|Equals) expr }


AST Nodes:

Core containers:
Module