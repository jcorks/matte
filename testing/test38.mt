//// Test 38
//
// basic matching
<@>m <-(exp){
    return match(exp) {
        (0, 1, 2, 3) : 'below4',
        default      : 'above3'
    };
};
return m(0) + m(10) + m(4);