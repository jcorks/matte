////Test 104 
//
// Introspection for numbers!

@out = '';

out = out + 4.99->floor;
out = out + 2.01->floor;
listen(to:::{
    out = out + '20.02'->floor;
}, onError:::(message){
    out = out + 'a';
});

out = out + 0.02->ceil;
out = out + 2.7988->ceil;
listen(to:::{
    out = out + '20.02'->ceil;
}, onError:::(message){
    out = out + 'b';
});


out = out + 4.51->round;
out = out + 7.39->round;
listen(to:::{
    out = out + '20.02'->round;
}, onError:::(message){
    out = out + 'c';
});


out = out + 28.64788975654116->asRadians;
out = out + 1.5707963267948966->asDegrees;
listen(to:::{
    out = out + {}->asRadians;
}, onError:::(message){
    out = out + 'd';
});
listen(to:::{
    out = out + Number->asDegrees;
}, onError:::(message){
    out = out + 'e';
});


out = out + 1.5707963267948966->sin->round;
out = out + 0->cos->round;
listen(to:::{
    out = out + '2'->sin;
}, onError:::(message){
    out = out + 'f';
});
listen(to:::{
    out = out + Number->cos;
}, onError:::(message){
    out = out + 'g';
});


out = out + 0.7853981633974483->tan->round;
listen(to:::{
    out = out + []->tan;
}, onError:::(message){
    out = out + 'h';
});



out = out + 256**0.5;
out = out + 'i';

out = out + (0 / 0)->isNaN;
out = out + (1/1)->isNaN;
listen(to:::{
    out = out + '2'->isNaN;
}, onError:::(message){
    out = out + 'j';
});



return out;
