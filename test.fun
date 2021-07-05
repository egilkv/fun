
a = 1;
b = /* comment */ 2;
c = a < b;
a + b;
(a + b);
#plus(a, b);
c ? a : b;

factorial = (n){ n<2 ? 1 : n*factorial(n-1) };
factorial(10);

ft = (a, b) {
    c = a;
    d = b;
    c + d
};
// NOTE: final semicolon required, not after c+d
ft(50,5);

v = [ 7, 8, "hello" ];
v[0];
v[2];
aa = {
   one : 1,
   two : 2,
   three : 3
};
ix = 'two;
aa['one];
aa[ix];
aa['three];
aa.one;
aa.two;
aa.three;
// aa.ix;
aa ++ { four : "FOUR" }; // modified assoc, add key
aa ++ { two : "TWO" }; // modified assoc, modify key
[ 3 : 99, 0 : "test" ]; // 1 and 2 are undefined

list = [ 1, 2, 3 ];
list[0];
list.0;
// TODO module.symbol;
"abc" ++ "def";
"abcdef"[2];
array1 = [ 1,2,3,4,5,6 ];       // TODO array1 = [ 0..5 : 0];
array1[0];
array1[3];
array1[3..4];

a = [0..10-1: #t ];
j = 5;
a[..4] ++ [#f] ++ a[6..];
a[..j-1] ++ [#f] ++ a[j+1..];

// TODO what happens to '.' here:
// ((a) { a<=1 ? a : a*.(a-1) }) (5);

// '=' binds right-to-left
f = g = 9;
g + f + 12;
g + f + 12 - 30;

// TODO parsing does not stop at semicolon
2^;

// '/' associates left-to-right
100 / 2 / 2;

undefined;

// continuation
io = #use("io");

text1 = "ECHO";
text2 = "DELTA";

invoke = (fn){
    text1 = "BOB";
    text2 = "CAROLINE";
    fn();
    text2 // value is local text
};

invoke( (){ io.println("case 1 is ", text1, " and ", text2) } );

func = (){
    text1 = "ALICE";
    invoke( (){ io.println("case 2 is ", text1, " and ", text2) } )
};

func();

((text2){ invoke( (){ io.println("case 3 is ", text1, " and ", text2) } ) })("ZZZ");

12 / - 4;
12345678.901234 * 100000;

1/2 + 1/3;
1.0/2 + 1/3;
1/2 - 0.5;

m = #use("math");
m.sqrt(2);
m.sqrt(-2);
m.e;
m.sin(m.pi/2);
m.tan(m.pi/4);
m.tan(m.pi/2);
m.atan(9999999999999);
m.pow(10,9);
m.pow(10,90);
m.pow(10,900);
m.pow(-10,9);
m.pow(10,-9);
m.abs(-12.34);
m.abs(-12);
//  m.abs(-12/34);
m.abs(-(12/34));
m.int(12.1);
m.int(12.6);
m.int(-12.1);
m.int(-12.6);
m.int(10/3);
m.int(11/3);

"a" < "b" < "c";
"aa" < "a";
"b" < "a" < "x";

as = "abc";
al = [ 1, 2, 3 ];
av = [ 0: 1, 2, 3 ];
as[2..];
al[2..];
av[2..];
as[3..];
al[3..];
av[3..];
as[4..];
al[4..];
av[4..];
as[2..2];
al[2..2];
av[2..2];
as[2..3];
al[2..3];
av[2..3];
as[3..3];
al[3..3];
av[3..3];

[0: 1, 2, 3] ++ [0: 4, 5];
[ 1, 2, 3 ] ++ [ 4, 5 ];

#include("qsort.fun");
qsort([5, 2, 7, 6, 3, 9, 12, 99, 1]);
qsort(["the", "quick", "brown", "fox", "jumps", "over", "a", "lazy", "dog"]);

max = 9223372036854775807; // max positive integer
max+1;
-max;
(-max)-1;
(-max)-2;
m.abs(-max);
m.abs((-max)-1);
m.int(9223372036854770000.0);
m.int(9223372036854775000.0);
m.int(9223372036854775807.0);
m.int(9223372036854775808.0);
m.int(9223372036855000000.0);
m.int(-9223372036854770000.0);
m.int(-9223372036854775807.0);
m.int(3/2);
m.int(-3/2);
m.int(1/3);
m.int(2/3);
m.int(4/3);
m.int(5/3);
-4-1;

[0: 1, 2, 3] ++ [ 4, 5];
[ 1, 2, 3 ] ++ [ 0: 4, 5 ];
qsort([0:5, 2, 7, 6, 3, 9, 12, 99, 1]);

[ 0..3: 99, 100 ];
[ 0: 99, 3: 99, 100 ];
[ 0: 98, ..3 : 99, 100 ];
[ 0: 98, ..3 : 99, 100 ];

[ 0: 1.23, 2.34, 3:3.14, 0: 2];

// closures https://clojure.org/guides/learn/functions
messenger_builder = (greeting) {
  (who) { io.println(greeting, " ", who) } }; // closes over greeting

// greeting provided here, then goes out of scope
hello_er = messenger_builder("Hello");

// greeting value still available because hello-er is a closure
hello_er("world!");

#type("abc");
#type(hello_er);
#type(list);
#type(1);
#type(1.0);
#type(1/2);
#type('sym);
#type(#type);
#type(#t);
#type(#void);
#type(akka);

time = #use("time");
time.utctime(1622447459.9024);
lt = time.localtime(1622447459.9024);
time.mktime(lt);

abc = (a, b, c) {
	a+b*(c+10)
};

// should detect this
aba = (a, b, a) {
	a+b*(a+5)
};

abc(1,2,3);
abc(c:3, b:2, a:1);

// initial unlabelled is OK
abc(1, c:3, b:2);
// cannot have unlabeled args after labelled
abc(c:3, a:1, 2);
// argument must match
abc(1, x:3, b:2);

abc0 = (a:10, b:20, c:30) {
	a+b*(c+10)
};

aba0 = (a:10, b:20, a:30) {
	a+b*(a+10)
};

ab0 = (a, b, 30) {
	a+b*(a+10)
};

abc0();
abc0(b:200);
abc0(1000);
abc0(1000, c:300);

// variadic functions:
average = ( ... ) {
    #apply(#plus, ...) / #count(...)
};

average(11, 12, 12);
average(11, 12, 12.0);

add = ( ... ) {
    ... == [] ? 0 : ...[0] + #apply(add, ...[1..])
};

add();
add(1);
add(1,2,3);

#apply(#plus);
#apply(#plus, 1, 2, [3, 4, 5]);

35/3 * 1.0;

g = (a, ...) { [a] ++ [99] ++ ... };
g(2, a:3, 4);

A = 1;
B = 1;
C = 2;
D = 2;

A == B && C == D;
1 == 1 && C == D;
1 == 2 && C == D;
2 == 2 && 1 == 1 && C == D;
1 == A && 2 == A && C == D;

// unknows:
A2 == B2;
A == A2;
A2 > B2;
A2 < 1;

// tail-call two levels
fc = (a) {
    b = 2*a;
    g = (){ b };
    g()
};

h = () {
    10*fc(12)
};

h();

(1..10)[0];
(1..10)[1..];
(1..2)[1..];
(1..2)[2..];
