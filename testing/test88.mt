//// Test 88.mt
//
// Custom typing 

@:MyType = {}
MyType.Shape = Object.newType(name:'Shape');
MyType.Square = Object.newType(name:'Square', inherits:[MyType.Shape]);
MyType.Triangle = Object.newType(name:'Triangle', inherits:[MyType.Shape]);


@:makeSquare ::(length){
    @:out = Object.instantiate(type:MyType.Square);
    out.length = length;
    out.area = ::{
        return out.length**2;
    }
    return out;
}

@:makeTriangle ::(base, height){
    @:out = Object.instantiate(type:MyType.Triangle);
    out.base = base;
    out.height = height;
    out.area = ::{
        return out.base*out.height*0.5;
    }
    return out;
}


@tri0 = makeTriangle(base:1, height:3);
@tri1 = makeTriangle(base:2, height:4);
@sq0 = makeSquare(length:3);
@sq1 = makeSquare(length:4);


@out = '';
@:combinedArea ::(a => MyType.Shape, b => MyType.Shape) {
    out = out + a.area() + b.area(); 
}


combinedArea(a:tri0, b:sq0);
{:::}{
    @:combinedAreaRestricted ::(shapeA => MyType.Shape, squareB => MyType.Square) {
        out = out + shapeA.area() + squareB.area(); 
    }

    combinedAreaRestricted(shapeA:sq1, squareB:sq0);
    combinedAreaRestricted(shapeA:sq0, squareB:tri0);
    combinedAreaRestricted(shapeA:sq1, squareB:sq0);
} : {
    onError:::(message) {}   
}

return out;


