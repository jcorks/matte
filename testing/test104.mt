////Test 104 
//
// Introspection for numbers!

@out = '';

out = out + introspect.floor(of:4.99);
out = out + introspect.floor(of:2.01);
listen(to:::{
    out = out + introspect.floor(of:'20.02');
}, ::{
    out = out + 'a';
});

out = out + introspect.ceil(of:0.02);
out = out + introspect.ceil(of:2.7988);
listen(to:::{
    out = out + introspect.ceil(of:'20.02');
}, ::{
    out = out + 'b';
});


out = out + introspect.round(of:4.51);
out = out + introspect.round(of:7.39);
listen(to:::{
    out = out + introspect.round(of:'20.02');
}, ::{
    out = out + 'c';
});


out = out + introspect.toRadians(of:28.64788975654116);
out = out + introspect.toDegrees(of:1.5707963267948966);
listen(to:::{
    out = out + introspect.toRadians({});
}, ::{
    out = out + 'd';
});
listen(to:::{
    out = out + introspect.toDegrees(Number);
}, onMessage:::{
    out = out + 'e';
});


out = out + introspect.round(of:introspect.sin(of:1.5707963267948966));
out = out + introspect.round(of:introspect.cos(of:0));
listen(to:::{
    out = out + introspect.sin('2');
}, onMessage:::{
    out = out + 'f';
});
listen(to:::{
    out = out + introspect.cos();
}, onMessage:::{
    out = out + 'g';
});


out = out + introspect.round(of:introspect.tan(of:0.7853981633974483));
listen(to:::{
    out = out + introspect.tan(of:[]);
}, onMessage:::{
    out = out + 'h';
});



out = out + introspect.sqrt(of:256);
listen(to:::{
    out = out + introspect.sqrt(of:'2');
}, onMessage:::{
    out = out + 'i';
});

out = out + introspect.isNaN(of:0 / 0);
out = out + introspect.isNaN(of:1/1);
listen(to:::{
    out = out + introspect.isNaN(of:'2');
}, onMessage:::{
    out = out + 'j';
});



return out;
