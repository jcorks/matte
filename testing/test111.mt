//// Test 111
//
// Auto-named parameters

@:MyFunc ::(value, otherValue) <- value*otherValue;


return ::<= {
    @:value = 20;
    @:otherValue = 3;
    
    return MyFunc(value, otherValue);
};

