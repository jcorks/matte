////Test 83
//
// String: substr 

@Matte = import('Matte.Core');
@a = Matte.String.new('This is a string');

@output = 'errorDidntHappen';
::{
    context.catch = ::{
        output = 'a';
    };
    
    // should error out, not allowed.
    output = String(a.substr('a', []));    
}


return '' + output + 
        a.substr(0, 3) + 
        a.substr(5, 6) +
        a.substr(8, 8) +
        a.substr(10, a.length-1) +
        (Matte.String.new('test').substr(0, 3));
