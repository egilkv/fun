
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
aa & { two : "TWO" }; // modified assoc

[ 3 : 99, 0 : "test" ]; // 1 and 2 are undefined

list = #(1, 2, 3);
list[0];
list.0;
// TODO module.symbol;
"abc" & "def";
"abcdef"[2];
// TODO array1 = [ 0..5 : 0];
array1[0];
array1[3];     // TODO generates error?
// TODO array1[3..4];

// TODO a = [0..20]{ #t };
// TODO list[1 ..];
j = 15;
// TODO a[..j-1] & []{#f} & a[j+1..];

// TODO what happens to '.' here:
// ((a) { a<=1 ? a : a*.(a-1) }) (5);

// '=' binds right-to-left
f = g = 9;
g + f + 12;
g + f + 12 - 30;

// error reporting
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
