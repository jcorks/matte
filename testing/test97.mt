//// Test 97
//
// For in depth


@out = '';

listen(to:::{
    for(in:[0, 1], do:1);
}, onError:::(message){
    out = out + 'nofn';
});


listen(to:::{
    for(in:[10, 4, 1], do:::{
        out = out + 'a';
    });
}, onError:::(message){
    out = out + 'norun';
});


listen(to:::{
    for();
}, onError:::(message){
    out = out + 'nofn';
});



listen(to:::{
    for(in:[5, 0, -1], do:::{
        out = out + 'b';
    });
}, onError:::(message){
    out = out + 'norun';
});

listen(to:::{
    for(in:0, do:::{
    
    });
}, onError:::(message){
    out = out + 'noind';
});



listen(to:::{
    for(in:[0, 10], do:::(i){
        out = out + 'c';        
        when (i == 4) 10; 
    });
}, onError:::(message){
    out = out + 'norun';
});


for(in:[0, 3, 1], do:::(i){
    out = out + 'd';        
});


return out;
