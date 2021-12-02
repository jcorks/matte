//// Test 98
//
// foreach in depth 

@out = '';

listen(to:::{
    foreach(in:'aaa', do:::{
    
    });
}, ::{
    out = out + 'noobj';
});



listen(to:::{
    foreach(in:{}, do:'aaa');
}, ::{
    out = out + 'nofn';
});




listen(to:::{
    foreach(in:{}, do:::{
        out = 'c';
    });
}, ::{
    out = out + 'norun';
});





listen(to:::{
    foreach(in:['a', 'b', 'c'], do:::(key => Number, val => String) {
        out = out + val;
    });
}, ::{
    out = out + 'norun';
});

return out;
