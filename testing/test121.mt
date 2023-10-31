@:memberFn = ::($, a, b) {
    return $.value + a + b;
}

@:obj0 = {
    value : 20,
    doSomething: memberFn
};

@:obj1 = {
    value : "Hey!",
    doSomething: memberFn
};

@out = '';

// 27
out = out + obj0.doSomething(a:3, b:4)

// Hey! How are you?
out = out + obj1.doSomething(a:"How", b:"AreYou?")


return out;
