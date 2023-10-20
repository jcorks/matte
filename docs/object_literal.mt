// Empty literals are okay!
@:a = {};

// Literal keys may be identifiers
@:b = {
    one : 1,
    two : 2,
    three : 3
};

// Should print true
print(message: b.one == b['one'] == 1)

@:c = {
    "one"+"two" : 3,
    b->type     : "hello"
}

// Should print 3
print(message: c.onetwo);

// Should print hello
print(message: c[Object]);

