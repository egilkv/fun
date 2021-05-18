
// simple ELM stuff
greet = (name){ "Hello " & name & "!" };
greet("Alice");
greet("Bob");

names = [ "Alice", "Bob", "Chuck" ];

// TODO how to get length of lists, i.e names.length
//list = #use("list");
//list.length(names);
//list.reverse(names);

numbers = [4,3,2,1];

//list.sort(numbers);

increment = (n){ n + 1 };

//list.map(numbers,increment); --> [5,4,3,2];

//In ELM tuples are short lists of length 2 or 3 (True, "name accepted!")

//In ELM, records are what we call assocs

john = {
    first : "John",
    last  : "Hobson",
    age   : 81
} ;

john.last;

// In ELM,   .last becomes a function to extract a value

//list.map(.last, [ john, john, john ])

// in ELM, the operator | is used to replace values
//{ john | last = "Adams" }
{
    john |
    last = "Adams"
};

birthday = (person) {
    { person | age : person.age + 1 }
};

//
// type annotations:
// half : Float -> Float
// half = (n) { n/2 };
//
// type alias same as typedef
// a type alias also constructs a function that builds a record
//
// review custom types: https://guide.elm-lang.org/types/custom_types.html

