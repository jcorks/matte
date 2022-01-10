////Test 83
//
// String: substr 

@a = 'This is a string';

@output = 'errorDidntHappen';
listen(to:::{
    
    // should error out, not allowed.
    output = String.substr(string:a, from:'a', to:[]);    
}, onMessage:::(message) {
    output = 'a';
});


return '' + output + 
        String.substr(string:a, from:0, to:3) + 
        String.substr(string:a, from:5, to:6) +
        String.substr(string:a, from:8, to:8) +
        String.substr(string:a, from:10, to:String.length(of:a)-1) +
        String.substr(string:'test', from:0, to:3);
