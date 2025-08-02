// factorial, no tail call

factorial(n) { n < 2 ? 1 : n*factorial(n-1) };
