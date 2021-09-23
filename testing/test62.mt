///// Test 62
//
// I <3 match

@decider = ::(a){
    return match(true) {
        (a < 50): 
            match(true) {
                (a < 15): 'Small',
                (a < 35): 'KindaSmall',                
                default:  'Sizeable'
            },
        
        (a < 100): 
            match(true) {
                (a < 65): 'Big',
                (a < 85): 'VeryBig',
                default:  'Huge'
            },
            
        default: 'TooBig'              
    };
};

@otherDecider = ::(i) {
    return match(true) {
        (i < 2): 2+2,
        (i > 2): 4+3+4,
        default: 'h' + 'ello'
    };
};


return '' + decider(30) + decider(2) + decider(99) + otherDecider(3) + decider(300);
