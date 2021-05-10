
a = 1;
b = 2;
c = a < b;
a + b;
(a + b);
#plus(a, b);
c ? a : b;

factorial = (n) { n<2 ? 1 : n*factorial(n-1) };
factorial(10);

ft = (a, b) {
    c = a;
    d = b;
    c + d
};
// NOTE: final semicolon required, not after c+d
ft(50,5);

v = []{7, 8, "hello"};
v[0];
v[2];
aa = []{
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
aa.ix;

list = #(1, 2, 3);
list[0];
list.0;
// TODO module.symbol;
"abc" & "def";
"abcdef"[2];
array1 = [6]{ 0 };
array1[0];
array1[3];
array1[3..4];

a = [0..20]{ #t };
// list[1 ..];
j = 15;
a[..j-1] & []{#f} & a[j+1..];

// TODO what happens to '.' here:
// ((a) { a<=1 ? a : a*.(a-1) }) (5);

