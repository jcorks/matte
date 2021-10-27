//// Test 98
//
// foreach in depth 

@out = '';

listen(::{
    foreach('aaa', ::{
    
    });
}, ::{
    out = out + 'noobj';
});



listen(::{
    foreach({}, 'aaa');
}, ::{
    out = out + 'nofn';
});




listen(::{
    foreach({}, ::{
        out = 'c';
    });
}, ::{
    out = out + 'norun';
});





listen(::{
    foreach(['a', 'b', 'c'], ::(key => Number, val => String) {
        out = out + val;
    });
}, ::{
    out = out + 'norun';
});

return out;
