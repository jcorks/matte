//// Test 88.mt
//
// Custom typing 

<@>MyType = {};
MyType.Shape = newtype({name:'Shape'});
MyType.Square = newtype({name:'Square', inherits:[MyType.Shape]});
MyType.Triangle = newtype({name:'Triangle', inherits:[MyType.Shape]});


<@>makeSquare ::(l){
    <@>out = instantiate(MyType.Square);
    out.length = l;
    out.area = ::{
        return out.length**2;
    };
    return out;
};

<@>makeTriangle ::(b, h){
    <@>out = instantiate(MyType.Triangle);
    out.base = b;
    out.height = h;
    out.area = ::{
        return out.base*out.height*0.5;
    };
    return out;
};


@tri0 = makeTriangle(1, 3);
@tri1 = makeTriangle(2, 4);
@sq0 = makeSquare(3);
@sq1 = makeSquare(4);


@out = '';
<@>combinedArea ::(a => MyType.Shape, b => MyType.Shape) {
    out = out + a.area() + b.area(); 
};


combinedArea(tri0, sq0);
listen(::{
    context.catch = ::{};
    <@>combinedAreaRestricted ::(a => MyType.Shape, b => MyType.Square) {
        out = out + a.area() + b.area(); 
    };

    combinedAreaRestricted(sq1, sq0);
    combinedAreaRestricted(sq0, tri0);
    combinedAreaRestricted(sq1, sq0);
});

return out;


