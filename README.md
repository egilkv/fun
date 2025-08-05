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
Operators for comparing numbers are `<`, `>`, `<=`, `>=`, `==` and `!=`, and yield a Boolean value.
Parenthesises can be used for grouping in the normal manner.
The conditional operator is also like in C, e.g. `a >= 0 ? 1 : -1`.

**Strings** are written using within double quotation marks, like `"this"`.
The backslash introduces special characters, as in C.
Strings are arrays of characters, and can be indexed like arrays. TODO described where
Operators for comparing numbers also work for alphabetic comparison of strings.

**Functions** are defined as in this example: `add(a,b) { a+b }` The function body, within curly brackets, may contain
multiple expressions, separated by semicolons. The last expression defines the value. Ideally, functions should have no
side effects, which means that as long as they are invoked with the same arguments, they always yield the same value.
Unnamed functions can be defined by replacing the function name by '\\', for *lambda*.

**Lists** ...

...

Arrays, strings as well as lists can be concatenated using the `&` operator.

