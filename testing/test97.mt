//// Test 97
//
// For in depth


@out = '';

[::]{
    [0, 1]->for(do:1);
}: { onError:::(message){
    out = out + 'nofn';
}};


[::]{
    [10, 4, 1]->for(do:::{
        out = out + 'a';
    });
}: { onError:::(message){
    out = out + 'norun';
}};


[::]{
    2->for(a:0);
}: { onError:::(message){
    out = out + 'nofn';
}};



[::]{
    [5, 0, -1]->for(do:::{
        out = out + 'b';
    });
}: { onError:::(message){
    out = out + 'norun';
}};

[::]{
    0->for(do:::{
    
    });
}: { onError:::(message){
    out = out + 'noind';
}};



[::]{
    [0, 10]->for(do:::(i){
        out = out + 'c';        
        when (i == 4) 10; 
    });
}: { onError:::(message){
    out = out + 'norun';
}};


[0, 3, 1]->for(do:::(i){
    out = out + 'd';        
});


return out;
