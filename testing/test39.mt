//// Test 39
//
// basic matching
@:m :: (value){
    return match(value) {
        (0)           : 'none',
        (1)           : 'one',
        (2, 3, 4)     : 'few',
        (5, 6)        : 'some',
        (7, 8, 9, 10) : 'lots',
        default       : 'toomany'
    }
}
return m(value:8)+m(value:0)+m(value:2)+m(value:399);
