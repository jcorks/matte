//// Test 97
//
// For in depth


@out = '';

listen(to:::{
    for(in:[0, 1], do:1);
}, onMessage:::(message){
    out = out + 'nofn';
});


listen(to:::{
    for(in:[10, 4, 1], do:::{
        out = out + 'a';
    });
}, onMessage:::(message){
    out = out + 'norun';
});


listen(to:::{
    for();
}, onMessage:::(message){
    out = out + 'nofn';
});



listen(to:::{
    for(in:[5, 0, -1], do:::{
        out = out + 'b';
    });
}, onMessage:::(message){
    out = out + 'norun';
});

listen(to:::{
    for(in:0, do:::{
    
    });
}, onMessage:::(message){
    out = out + 'noind';
});



listen(to:::{
    for(in:[0, 10], do:::(i){
        out = out + 'c';        
        when (i == 4) 10; 
    });
}, onMessage:::(message){
    out = out + 'norun';
});


for(in:[0, 3, 1], do:::(i){
    out = out + 'd';        
});


return out;
