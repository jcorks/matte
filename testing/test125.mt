//// Test 125
//
//  LightClass (synamic bind custom + interface) test 

@:class = ::<= {

    @:applyClass ::(class, priv, obj, args) {
        if (class.inherits != empty) ::<= {
            foreach(class.inherits) ::(i, v) {
                applyClass(class:v, priv, obj, args);
            }
        }




        if (class.constructor != empty) ::<= {
            priv.constructor = class.constructor;
            priv.constructor(*args);
            priv.constructor = empty;
        }
        
        foreach(class.interface) ::(name, fn) {
            obj[name] = fn;
        }
    }


    return ::(name => String, statics, inherits, constructor, interface) {

        @:type = if (inherits == empty)
            Object.newType(name)
        else 
            Object.newType(name, inherits:[...inherits]->map(to:::(value) <- value.type));

        @:class = {};
        if (statics != empty) ::<= {
            foreach(statics) ::(name => String, thing) {
                class[name] = thing;
            }
        }
        
        class.type = type;
        class.interface = interface;
        class.constructor = constructor;
        class.inherits = inherits;
        class.new = ::(*args) {
            @:obj = Object.instantiate(type);
            @:priv = {this:obj};
            applyClass(class, priv, obj, args);        
            obj->setIsInterface(enabled:true, dynamicBinding:priv);
            return obj;
        }
        return class;
    }
}



@:Shape = class(
    name : 'Shape',
    constructor ::($, size) {
        $.size = size;
        $.sides = 0;
    },
    interface : {
        numSides : {
            get ::($) <- $.sides
        }
    }
)


@:Square = class(
    name : 'Shape.Square',
    inherits : [
        Shape
    ],
    constructor ::($) {
        $.sides = 4;
    },

    interface : {
        area::($) <- $.size**2,
        length : {
            get ::($) <-  $.size,
            set ::($, value) <- $.size = value
        }
    }
);

@out = '';
@:sq = Square.new(size:2);
out = out + sq.length;

sq.length = 3;
out = out + sq.area();
out = out + sq.numSides;

{:::} {
    out = out + sq.sides;
} : {
    onError::(message) {
        out = out + 'nosides';
    }
}

return out;
