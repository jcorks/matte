//// Test 38
//
// basic matching
@:m :: (value){
    return match(value) {
        (0, 1, 2, 3) : 'below4',
        default      : 'above3'
    }
}
return m(value:0) + m(value:10) + m(value:4);
