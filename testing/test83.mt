////Test 83
//
// String: substr 

@a = 'This is a string';

@output = 'errorDidntHappen';
listen(to:::{
    
    // should error out, not allowed.
    output = a->substr(from:'a', to:[]);    
}, onError:::(message) {
    output = 'a';
});


return '' + output + 
        a->substr(from:0, to:3) + 
        a->substr(from:5, to:6) +
        a->substr(from:8, to:8) +
        a->substr(from:10, to:a->length-1) +
        'test'->substr(from:0, to:3);
