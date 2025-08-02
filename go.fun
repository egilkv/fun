
f1(a){
    b = 99;
    f3( (){ cc = b+2; #bp(); a })
};

f3(thunk){
    b = 9;
    thunk();
    12
};

f1(999);

---------------------------------------------------------
io = #use("io");
#go( (){ 
	#go( (){ io.println("hello2") });
	io.println("hello1")
});
---------------------------------------------------------
io = #use("io");
c1 = #channel();
c2 = #channel();

cr() {
	io.println("read");
	v = <- c1;
	io.println("write");
	c2 <- 2*v;
	io.println("loop");
	cr()
};
#go(cr);
c1 <- 99;
<- c2;
---------------------------------------------------------
io = #use("io");
c1 = #channel();

cr() {
	io.println("value", <- c1);
	cr()
};
#go(cr);
c1 <- 99;

