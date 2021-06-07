
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


