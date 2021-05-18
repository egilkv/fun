/*
 *  https://spf13.com/post/is-go-object-oriented/

type rect struct {
    width int
    height int
}

func (r *rect) area() int {
    return r.width * r.height
}

func main() {
    r := rect{width: 10, height: 5}
    fmt.Println("area: ", r.area())
}

*/

io = #use("io");

r = {
    width: 10,
    height: 5
};

area = (r) {
     r.width * r.height
};

io.println("area: ", area(r));

// TODO how to connect the function to the assoc

/* one possibility
r = {
    width: 10,
    height: 5,
    area: () { .width * .height }
};
*/

///////////////////////////////////////////////////////////

/*
 *  https://play.golang.org/p/LGB-2j707c

package main

import "fmt"

type Counter int

func (c *Counter) AddOne() {
  *c++
}

func main() {
        var hits Counter
        hits.AddOne()
        fmt.Println(hits)
}

*/

///////////////////////////////////////////////////////////

/*
 *  "has-a"

type Person struct {
   Name string
   Address Address
}

type Address struct {
   Number string
   Street string
   City   string
   State  string
   Zip    string
}

func (p *Person) Talk() {
    fmt.Println("Hi, my name is", p.Name)
}

func (p *Person) Location() {
    fmt.Println("I'm at", p.Address.Number, p.Address.Street,
p.Address.City, p.Address.State, p.Address.Zip)
}

func main() {
    p := Person{
	Name: "Steve",
	Address: Address{
	    Number: "13",
	    Street: "Main",
	    City:   "Gotham",
	    State:  "NY",
	    Zip:    "01313",
	},
    }

    p.Talk()
    p.Location()
}

*/

Talk = (p){
    io.println("Hi, my name is", p.Name)
};

Location = (p) {
    io.println("I'm at", p.Address.Number, p.Address.Street,
	       p.Address.City, p.Address.State, p.Address.Zip)
};

    p = {
	Name: "Steve",
	Address: {
	    Number: "13",
	    Street: "Main",
	    City:   "Gotham",
	    State:  "NY",
	    Zip:    "01313"
	}
    };

    Talk(p);
    Location(p);
