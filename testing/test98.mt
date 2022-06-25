//// Test 98
//
// foreach in depth 

@out = '';

listen(to:::{
    'aaa'->foreach(do:::{
    
    });
}, onError:::(message){
    out = out + 'noobj';
});



listen(to:::{
    {}->foreach(do:'aaa');
}, onError:::(message){
    out = out + 'nofn';
});




listen(to:::{
    {}->foreach(do:::{
        out = 'c';
    });
}, onError:::(message){
    out = out + 'norun';
});





listen(to:::{
    ['a', 'b', 'c']->foreach(do:::(key => Number, val => String) {
        out = out + val;
    });
}, onError:::(message){
    out = out + 'norun';
});

return out;
