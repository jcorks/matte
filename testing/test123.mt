//// Test 123 
// 
// varargs
@out = "";

@:lastFn ::(first, second, third, fourth) {
    out = out + 'last' + third + fourth + first + second;   
}



@:intermediateFn::(
    first,
    second
) {
    out = out + 'inter' + second + first;
}



@:fn ::(*args) {
    out = out + 'fn' + args->keys->size + args.third + args.fourth;
    intermediateFn(
        first:args.first,
        second:args.second
    )
    args.runAfter(*args)
}





fn(
    first:1, 
    second:2, 
    third:3, 
    fourth:4, 
    runAfter:lastFn
);
return out;
