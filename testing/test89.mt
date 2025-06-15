@:class = import(module:'Matte.Core.Class');


@:Shape = class(
    name:   'Shape',
    define: ::(this, thisType) {
        this.interface = {
            // a shape on its own is abstract
            area :: {
                return 0;
            }
        }
    }
);



@:Square = class(
    name     : 'Square',
    inherits : [Shape],
    define   : ::(this, thisType) {
        @l;

        this.interface = {
            length : {
                set::(value) <- l = value
            },
        
            area ::{
                return l**2;
            }
        }
    }
);


@:Triangle = class(
    name     : 'Triangle',
    inherits : [Shape],
    define   :::(this, thisType) {

        @b;
        @h;

        this.interface = {
            init::(base => Number, height => Number) {
                b = base;
                h = height;
                return this;
            },

            area ::{
                return b*h*0.5;
            }
        }
    }
);


@tri0 = Triangle.new().init(base:1, height:3);
@tri1 = Triangle.new().init(base:2, height:4);
@sq0 = Square.new();
@sq1 = Square.new();

sq0.length = 3;
sq1.length = 4;

@out = '';
@:combinedArea ::(a => Shape.type, b => Shape.type) {
    out = out + 
        a.area() + 
        b.area()
    ; 
}


combinedArea(a:tri0, b:sq0);
::?{
    @:combinedAreaRestricted ::(a => Shape.type, b => Square.type) {
        out = out + a.area() + b.area(); 
    }

    combinedAreaRestricted(a:sq1, b:sq0);
    // will throw type error
    combinedAreaRestricted(a:sq0, b:tri0);
    combinedAreaRestricted(a:sq1, b:sq0);
} => {
    onError:::(message){}
}

return out;
