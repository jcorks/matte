//// Test 97
//
// For in depth


@out = '';

listen(to:::{
    for(in:[0, 1], do:1);
}, ::{
    out = out + 'nofn';
});


listen(to:::{
    for(in:[10, 4, 1], do:::{
        out = out + 'a';
    });
}, ::{
    out = out + 'norun';
});


listen(to:::{
    for();
}, ::{
    out = out + 'nofn';
});



listen(to:::{
    for(in:[5, 0, -1], do:::{
        out = out + 'b';
    });
}, ::{
    out = out + 'norun';
});

listen(to:::{
    for(in:0, do:::{
    
    });
}, ::{
    out = out + 'noind';
});



listen(to:::{
    for(in:[0, 10], do:::(i){
        out = out + 'c';        
        when (i == 4) 10; 
    });
}, ::{
    out = out + 'norun';
});


for(in:[0, 3, 1], do:::(i){
    out = out + 'd';        
});


return out;
