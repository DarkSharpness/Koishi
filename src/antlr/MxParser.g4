parser grammar MxParser;

options {
    tokenVocab = MxLexer;
}


file_Input: (function_Definition | class_Definition | variable_Definition | ';')* EOF  ;


/* Function part. */
function_Definition : function_Argument '(' function_Param_List? ')' block_Stmt;
function_Param_List : function_Argument (',' function_Argument)*;
function_Argument   : typename Identifier;

/* Class part.  */
class_Definition    : 'class' Identifier '{' class_Content* '}' ';';
class_Ctor_Function : Identifier '(' ')' block_Stmt;
class_Content       :
    variable_Definition |
    function_Definition |
    class_Ctor_Function ;

/* Basic statement.  */
stmt:
    simple_Stmt         |
    branch_Stmt         |
    loop_Stmt           |
    flow_Stmt           |
    variable_Definition |
    block_Stmt          ;


/* Simple? */
block_Stmt  : '{' stmt* '}' ;
simple_Stmt : expr_List? ';';


/* Branch part. */
branch_Stmt: if_Stmt else_if_Stmt* else_Stmt?   ;
if_Stmt     : 'if'      '(' expression ')' stmt ;
else_if_Stmt: 'else if' '(' expression ')' stmt ;
else_Stmt   : 'else'                       stmt ;


/* Loop part */
loop_Stmt   : for_Stmt | while_Stmt ;
for_Stmt    :
    'for' '(' 
        (simple_Stmt | variable_Definition)
        condition   = expression? ';'
        step        = expression? 
    ')' stmt;
while_Stmt  : 'while' '(' expression ')' stmt  ;


/* Flow control. */
flow_Stmt: ('continue' | 'break' | ('return' expression?)) ';';


/* Variable part. */
variable_Definition :
    typename init_Stmt (',' init_Stmt)* ';';
init_Stmt: Identifier ('=' expression)?;


/* Expression part */
expr_List   : expression (',' expression)*   ;
expression  :
  '(' l = expression    op = ')'                                        # bracket
    | l = expression   (op = '['    expression  ']')+                   # subscript
    | l = expression    op = '('    expr_List?  ')'                     # function
    | l = expression    op = '.'    Identifier                          # member
    | l = expression    op = ('++' |'--')                               # unary
    |                   op = 'new'  new_Type                            # construct
    | <assoc = right>   op = ('++' | '--' )             r = expression  # unary
    | <assoc = right>   op = ('+'  | '-'  | '~' | '!' ) r = expression  # unary
    | l = expression    op = ('*'  | '/'  | '%')        r = expression  # binary
    | l = expression    op = ('+'  | '-'  )             r = expression  # binary
    | l = expression    op = ('<<' | '>>' )             r = expression  # binary
    | l = expression    op = ('<=' | '>=' | '<' | '>')  r = expression  # binary
    | l = expression    op = ('==' | '!=' )             r = expression  # binary
    | l = expression    op = '&'    r = expression                      # binary
    | l = expression    op = '^'    r = expression                      # binary
    | l = expression    op = '|'    r = expression                      # binary
    | l = expression    op = '&&'   r = expression                      # binary
    | l = expression    op = '||'   r = expression                      # binary
    | <assoc = right>   v = expression  op = '?'    l = expression ':'  r = expression # condition
    | <assoc = right>   l = expression  op = '='    r = expression      # binary
    | literal_Constant                                                  # literal
    | Identifier                                                        # atom
    | This                                                              # this;


/* Basic part.  */
typename            : (BasicTypes | Identifier) ('[' ']')* ;
new_Type            : ((BasicTypes new_Index) | (Identifier new_Index?)) ('(' ')')?; 
new_Index           : ('[' good+=expression ']')+ ('[' ']')* ('[' bad+=expression ']')*;
literal_Constant    : Number | Cstring | Null | True | False;

