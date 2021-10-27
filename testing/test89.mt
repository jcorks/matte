<@>class = import('Matte.Class');


<@>Shape = class({
    name : 'Shape',
    define ::(this, args, classinst) {
        this.interface({
            // a shape on its own is abstract
            area :: {
                return 0;
            }
        });
    }
});



<@>Square = class({
    name : 'Square',
    inherits : [Shape],
    define ::(this, args, classinst) {
        @length = Number(args.length);
        this.interface({
            area ::{
                return length**2;
            }
        });
    }
});


<@>Triangle = class({
    name : 'Triangle',
    inherits : [Shape],
    define ::(this, args, classinst) {
        @base = Number(args.base);
        @height = Number(args.height);
        this.interface({
            area ::{
                return base*height*0.5;
            }
        });
    }
});


@tri0 = Triangle.new({base:1, height:3});
@tri1 = Triangle.new({base:2, height:4});
@sq0 = Square.new({length:3});
@sq1 = Square.new({length:4});


@out = '';
<@>combinedArea ::(a => Shape.type, b => Shape.type) {
    out = out + a.area() + b.area(); 
};


combinedArea(tri0, sq0);
listen(::{
    <@>combinedAreaRestricted ::(a => Shape.type, b => Square.type) {
        out = out + a.area() + b.area(); 
    };

    combinedAreaRestricted(sq1, sq0);
    // will throw type error
    combinedAreaRestricted(sq0, tri0);
    combinedAreaRestricted(sq1, sq0);
});

return out;
