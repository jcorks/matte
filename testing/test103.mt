//// Test 103
//
// Erroneous introspection 

@out = '';

listen(::{
    out = out + introspect.keys(2423);
}, ::{
    out = out + 1;
});


listen(::{
    out = out + introspect.values(String);
}, ::{
    out = out + 2;
});

listen(::{
    out = out + introspect.keycount(String);
}, ::{
    out = out + 3;
});


listen(::{
    out = out + introspect.length([0, 1, 2]);
}, ::{
    out = out + 4;
});


listen(::{
    out = out + introspect.arrayToString('aaaaaaa');
}, ::{
    out = out + 5;
});


listen(::{
    out = out + introspect.charAt('123456', 5);
    out = out + introspect.charAt(1321, 2);
}, ::{
    out = out + 7;
});


listen(::{
    out = out + introspect.charAt('222222', '2');
}, ::{
    out = out + 8;
});

listen(::{
    out = out + introspect.charCodeAt(1321, 1);
}, ::{
    out = out + 9;
});

listen(::{
    out = out + introspect.charCodeAt('222222', '2');
}, ::{
    out = out + 'A';
});




listen(::{
    out = out + introspect.subset(20);
}, ::{
    out = out + 'B';
});

listen(::{
    out = out + introspect.subset([0, 1, 2], 1);
}, ::{
    out = out + 'C';
});


listen(::{
    out = out + introspect.subset([0, 1, 2], '1', 2);
}, ::{
    out = out + 'C';
});

return out;