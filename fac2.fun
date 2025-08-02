// factorial, with tail call

factorial(n) {
    // TODO use lambda, are parenthesis needed?
    (iter(product, n){
	 n < 2 ? product
	       : iter(product * n, n - 1)
    })(1, n)
};

factorial(10);
