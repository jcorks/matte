////Test 104 
//
// Introspection for numbers!

@out = '';

out = out + introspect.floor(4.99);
out = out + introspect.floor(2.01);
listen(::{
    out = out + introspect.floor('20.02');
}, ::{
    out = out + 'a';
});

out = out + introspect.ceil(0.02);
out = out + introspect.ceil(2.7988);
listen(::{
    out = out + introspect.ceil('20.02');
}, ::{
    out = out + 'b';
});


out = out + introspect.round(4.51);
out = out + introspect.round(7.39);
listen(::{
    out = out + introspect.round('20.02');
}, ::{
    out = out + 'c';
});


out = out + introspect.toRadians(28.64788975654116);
out = out + introspect.toDegrees(1.5707963267948966);
listen(::{
    out = out + introspect.toRadians({});
}, ::{
    out = out + 'd';
});
listen(::{
    out = out + introspect.toDegrees(Number);
}, ::{
    out = out + 'e';
});


out = out + introspect.round(introspect.sin(1.5707963267948966));
out = out + introspect.round(introspect.cos(0));
listen(::{
    out = out + introspect.sin('2');
}, ::{
    out = out + 'f';
});
listen(::{
    out = out + introspect.cos();
}, ::{
    out = out + 'g';
});


out = out + introspect.round(introspect.tan(0.7853981633974483));
listen(::{
    out = out + introspect.tan([]);
}, ::{
    out = out + 'h';
});



out = out + introspect.sqrt(256);
listen(::{
    out = out + introspect.sqrt('2');
}, ::{
    out = out + 'i';
});

out = out + introspect.isNaN(0 / 0);
out = out + introspect.isNaN(1/1);
listen(::{
    out = out + introspect.isNaN('2');
}, ::{
    out = out + 'j';
});



return out;
