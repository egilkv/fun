**FUN** is a functional programming language.

# Basics #

**Whitespace** can generally be used freely to format the code for best readability.

**Comments** work like in C, i.e. either enclosed in `/* */` or from ´//´ to the end of line.

**Numbers** are either integers (`42`), fractions (`22/7`) or floating point numbers (`3.14`).
*To convert an integer or fraction to floating point, simply multiply by 1.0.*

**Symbols** consist of a letter followed by zero or more letters, digits or underscores. 
There are no reserved symbols.
Symbols are assigned a value with the `=` operator, e.g. `a = 5`. Once assigned, the value can not be changed.
Outside assignments, symbols are evaluated, yielding their value. 
To avoid evaluation, precede the symbol with a single quote, like `'this'.

Built in symbols start with a `#`. 
The symbols `#t` and `#f` represents Boolean true and false, while `#void` represents no value. 
These symbols evaluate to themselves, so they need not be quoted.

Basic arithmetic **operators**, `+`, `-`, `*` and `/`, work much like in C.
Dividing two integers or quotients will yield a quotient or an integer.
Operators for comparing numbers are `<`, `>`, `<=`, `>=` and `==`, and yield a Boolean value.
Parenthesises can be used for grouping in the normal manner.
The conditional operator is also like in C, e.g. `a >= 0 ? 1 : -1`.

**String** are arrays of characters, and are written using within double quotation marks, like `"this"`.

...

Arrays as well as lists can be concatenated using the `++` operator.

