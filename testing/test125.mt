//// Test 125
//
//  LightClass (dynamic bind custom + interface) test 

@:class = ::<= {

    @:applyClass ::(class, priv, obj) {
        if (class.inherits != empty) ::<= {
            foreach(class.inherits) ::(i, v) {
                applyClass(class:v, priv, obj);
            }
        }
        
        foreach(class.interface) ::(name, fn) {
            obj[name] = fn;
        }
    }
    
    @:construct ::(class, priv, obj, args) {
        if (class.inherits != empty) ::<= {
            foreach(class.inherits) ::(i, v) {
                construct(class:v, priv, obj, args);
            }
        }
        if (class.constructor != empty) ::<= {
            obj->setIsInterface(enabled:false);
            obj.constructor = class.constructor;
            obj->setIsInterface(enabled:true, private:priv);
            breakpoint();
            obj.constructor(*args);
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
            applyClass(class, priv, obj);        
            obj.constructor = constructor;
            obj->setIsInterface(enabled:true, private:priv);
            construct(class, obj, priv, args);        
            return obj;
        }
        return class;
    }
}



@:Shape = class(
    name : 'Shape',
    constructor ::(size) {
        breakpoint();
        _.size = size;
        _.sides = 0;
    },
    interface : {
        numSides : {
            get :: <- _.sides
        }
    }
)


@:Square = class(
    name : 'Shape.Square',
    inherits : [
        Shape
    ],
    constructor :: {
        _.sides = 4;
    },

    interface : {
        area:: <- _.size**2,
        length : {
            get :: <- _.size,
            set ::(value) <- _.size = value
        }
    }
);

@out = '';
@:sq = Square.new(size:2);
out = out + sq.length;

sq.length = 3;
out = out + sq.area();
out = out + sq.numSides;

::? {
    out = out + sq.sides;
} => {
    onError::(message) {
        out = out + 'nosides';
    }
}

return out;
