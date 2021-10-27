//// Test 97
//
// For in depth


@out = '';

listen(::{
    for([0, 1], 1);
}, ::{
    out = out + 'nofn';
});


listen(::{
    for([10, 4, 1], ::{
        out = out + 'a';
    });
}, ::{
    out = out + 'norun';
});


listen(::{
    for();
}, ::{
    out = out + 'nofn';
});



listen(::{
    for([5, 0, -1], ::{
        out = out + 'b';
    });
}, ::{
    out = out + 'norun';
});

listen(::{
    for(0, ::{
    
    });
}, ::{
    out = out + 'noind';
});



listen(::{
    for([0, 10], ::(i){
        out = out + 'c';        
        when (i == 4) 10; 
    });
}, ::{
    out = out + 'norun';
});


for([0, 3, 1], ::(i){
    out = out + 'd';        
});


return out;
