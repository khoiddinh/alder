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

expr            := equality ;

equality        := comparison { (Equals | NotEq) comparison } ;
comparison      := additive   { (LCompare | LEqCompare | GCompare | GEqCompare) additive } ;
additive        := multiplicative { (Plus | Minus) multiplicative } ;
multiplicative  := unary { (Star | Slash) unary } ;
unary           := (Minus | KwNot) unary | postfix ;
postfix         := primary ( LParen [expr { Comma expr }] RParen )* ;
primary         := IntLit | StringLit | BoolLit | Identifier | LParen expr RParen ;


AST Nodes:

Core containers:
Module