@:class = import(module:'Matte.Core.Class');


@:Shape = class(
    define ::(this) {
        @privateSize;
        
        this.constructor = ::{
            this.reset();
        }
        
        this.interface = {
            size : {
                get ::<- privateSize,
                set ::(value => Number) <- privateSize = value
            },
            
            reset ::{
                privateSize = 1;
            }
        }
    }
);


@:Square = class(
    inherits : [Shape],
    define::(this) {
        this.interface = {
            area : {
                get ::<- this.size ** 2
            }
        }    
    }
)


@:square = Square.new();
square.size = 2;

// Should print 4
print(message:square.area);

@:shape = Shape.new();

@:printSize::(s => Shape.type) {
    print(message:s.size);
}   

// Should print 1
printSize(s:shape);

// Should print 2, as Square is a Shape.
printSize(s:square);








