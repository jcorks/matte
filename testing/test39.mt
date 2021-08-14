//// Test 39
//
// basic matching
<@>m <-(exp){
    return match(exp) {
        (0)           : 'none',
        (1)           : 'one',
        (2, 3, 4)     : 'few',
        (5, 6)        : 'some',
        (7, 8, 9, 10) : 'lots',
        default       : 'toomany'
    };
};
return m(8)+m(0)+m(2)+m(399);