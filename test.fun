
a = 1;
b = 2;
c = a > b;
a + b;
(a + b);
#plus(a, b);

factorial = (n) { n<2 ? 1 : n*factorial(n-1) };
factorial(10);

// assign f to a lambda
f = (){5};
f();

v = []{7, 8, "hello"};
v[0];
v[2];
a = []{
   one : 1,
   two : 2,
   three : 3
};

#(1, 2, 3);
module.symbol;
list[0];
"abc" & "def";
array1 = [6]{ 0 };
array2 = [0..5]{ 0 };
array1[3];
array1[3..4];
// list[1 ..];
a[0..j-1] & []{#f} & a[j+1..N];
c ? a : b;
// (a) { a<=1 ? a : a*.(a-1) } (5);

