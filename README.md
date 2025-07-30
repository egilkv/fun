**FUN** is a functional programming language.

#Basics#

**Whitespace** can generally be used freely to format the code for best readability.

**Comments** work like in C, i.e. either enclosed in `/* */` or from ´//´ to the end of line.

**Numbers** are either integers (`42`), fractions (`22/7`) or floating point numbers (`3.14`).
*To convert an integer or fraction to floating point, simply multiply by 1.0.*

**Symbols** consist of a letter followed by zero or more letters, digits or underscores. There are no reserved symbols.

Built in symbols start with a `#`. The symbols `#t` and `#f` represents Boolean true and false, while `#void` represents no value.

**Operators** work much like in C. Basic arithmetic operators are '+', '-', '*' and '/'.
Dividing two integers or quotients will yield a quotient or an integer.
Operators for comparing numbers are `<`, `>`, `<=`, `>=` and `==`, and they yield a Boolean value.
Parenthesises can be used for grouping in the normal manner.
The conditional operator is also like in C, e.g.
    a >= 0 ? 1 : -1
