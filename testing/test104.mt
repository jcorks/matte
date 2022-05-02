////Test 104 
//
// Introspection for numbers!

@out = '';

out = out + Number.floor(of:4.99);
out = out + Number.floor(of:2.01);
listen(to:::{
    out = out + Number.floor(of:'20.02');
}, onError:::(message){
    out = out + 'a';
});

out = out + Number.ceil(of:0.02);
out = out + Number.ceil(of:2.7988);
listen(to:::{
    out = out + Number.ceil(of:'20.02');
}, onError:::(message){
    out = out + 'b';
});


out = out + Number.round(value:4.51);
out = out + Number.round(value:7.39);
listen(to:::{
    out = out + Number.round(value:'20.02');
}, onError:::(message){
    out = out + 'c';
});


out = out + Number.toRadians(value:28.64788975654116);
out = out + Number.toDegrees(value:1.5707963267948966);
listen(to:::{
    out = out + Number.toRadians(value:{});
}, onError:::(message){
    out = out + 'd';
});
listen(to:::{
    out = out + Number.toDegrees(value:Number);
}, onError:::(message){
    out = out + 'e';
});


out = out + Number.round(value:Number.sin(of:1.5707963267948966));
out = out + Number.round(value:Number.cos(of:0));
listen(to:::{
    out = out + Number.sin(of:'2');
}, onError:::(message){
    out = out + 'f';
});
listen(to:::{
    out = out + Number.cos();
}, onError:::(message){
    out = out + 'g';
});


out = out + Number.round(value:Number.tan(of:0.7853981633974483));
listen(to:::{
    out = out + Number.tan(of:[]);
}, onError:::(message){
    out = out + 'h';
});



out = out + 256**0.5;
out = out + 'i';

out = out + Number.isNaN(value:0 / 0);
out = out + Number.isNaN(value:1/1);
listen(to:::{
    out = out + Number.isNaN(value:'2');
}, onError:::(message){
    out = out + 'j';
});



return out;
