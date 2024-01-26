lexer grammar MxLexer;

channels {
    COMMENTS
}


/* Comments */
Comment_Multi   : '/*'.*?'*/'             -> channel(COMMENTS)  ;
Comment_Single  : '//'.*? (NewLine | EOF) -> channel(COMMENTS)  ;


/* Basics...... */
Cstring : '"' (EscapeChar | .)*? '"' ;
fragment EscapeChar: '\\\\' | '\\n' | '\\t' | '\\"';
NewLine : ('\r' | '\n' | '\u2028' | '\u2029')           -> skip ;
Blank   : (' ' | '\t' | '\u000B' | '\u000C' | '\u00A0') -> skip ;

/* Basic types. */
BasicTypes  : 'int' | 'bool' | 'void' | 'string' ;


/* Built-in Variables */
This    : 'this'  ;
Null    : 'null'  ;
True    : 'true'  ;
False   : 'false' ;


/* Classes. */
New     : 'new'   ;
Class   : 'class' ;


/* Flow. */
Else_if     : 'else if'    ; // This is inspired by Wankupi qwq~
If          : 'if'         ;
Else        : 'else'       ;
For         : 'for'        ;
While       : 'while'      ;
Break       : 'break'      ;
Continue    : 'continue'   ;
Return      : 'return'     ;


/* Operators of 1 object. */
Increment   : '++'  ;
Decrement   : '--'  ;
Logic_Not   : '!'   ;
Bit_Inv     : '~'   ;


/* Operators of 1~2 objects. */
Add : '+'   ;
Sub : '-'   ;


/* Operators of 2 objects. */
Dot         : '.'   ;
Mul         : '*'   ;
Div         : '/'   ;
Mod         : '%'   ;
Shift_Right : '>>'  ;
Shift_Left_ : '<<'  ;
Cmp_le      : '<='  ;
Cmp_ge      : '>='  ;
Cmp_lt      : '<'   ;
Cmp_gt      : '>'   ;
Cmp_eq      : '=='  ;
Cmp_ne      : '!='  ;
Bit_And     : '&'   ;
Bit_Xor     : '^'   ;
Bit_Or_     : '|'   ;
Logic_And   : '&&'  ;
Logic_Or_   : '||'  ;
Assign      : '='   ;


/* Operators of 3 objects. */
Quest       : '?'   ;
Colon       : ':'   ;


/* Brackets...... */
Paren_Left_ : '('   ;
Paren_Right : ')'   ;
Brack_Left_ : '['   ;
Brack_Right : ']'   ;
Brace_Left_ : '{'   ;
Brace_Right : '}'   ;
Comma       : ','   ;
Semi_Colon  : ';'   ;


/* Others...... */
Number          : [1-9] Digit* | '0';
fragment Digit  : [0-9] ;
Identifier      : [A-Za-z][A-Za-z_0-9]*;

