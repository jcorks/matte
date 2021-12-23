//// Test 98
//
// foreach in depth 

@out = '';

listen(to:::{
    foreach(in:'aaa', do:::{
    
    });
}, onMessage:::(message){
    out = out + 'noobj';
});



listen(to:::{
    foreach(in:{}, do:'aaa');
}, onMessage:::(message){
    out = out + 'nofn';
});




listen(to:::{
    foreach(in:{}, do:::{
        out = 'c';
    });
}, onMessage:::(message){
    out = out + 'norun';
});





listen(to:::{
    foreach(in:['a', 'b', 'c'], do:::(key => Number, val => String) {
        out = out + val;
    });
}, onMessage:::(message){
    out = out + 'norun';
});

return out;
