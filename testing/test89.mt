@:class = import(module:'Matte.Core.Class');


@:Shape = class(
    name:   'Shape',
    define: ::(this, thisType) {
        this.interface = {
            // a shape on its own is abstract
            area :: {
                return 0;
            }
        };
    }
);



@:Square = class(
    name     : 'Square',
    inherits : [Shape],
    define   : ::(this, thisType) {
        @l;

        this.constructor = ::(length) {
            l = length;
            return this;
        };


        this.interface = {
            area ::{
                return l**2;
            }
        };
    }
);


@:Triangle = class(
    name     : 'Triangle',
    inherits : [Shape],
    define   :::(this, thisType) {

        @b;
        @h;

        this.constructor = ::(base => Number, height => Number) {
            b = base;
            h = height;
            return this;
        };

        this.interface = {
            area ::{
                return b*h*0.5;
            }
        };
    }
);


@tri0 = Triangle.new(base:1, height:3);
@tri1 = Triangle.new(base:2, height:4);
@sq0 = Square.new(length:3);
@sq1 = Square.new(length:4);


@out = '';
@:combinedArea ::(a => Shape.type, b => Shape.type) {
    out = out + a.area() + b.area(); 
};


combinedArea(a:tri0, b:sq0);
listen(to:::{
    @:combinedAreaRestricted ::(a => Shape.type, b => Square.type) {
        out = out + a.area() + b.area(); 
    };

    combinedAreaRestricted(a:sq1, b:sq0);
    // will throw type error
    combinedAreaRestricted(a:sq0, b:tri0);
    combinedAreaRestricted(a:sq1, b:sq0);
}, onError:::(message){});

return out;
