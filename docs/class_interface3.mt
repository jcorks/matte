@:class = import(module:'Matte.Core.Class');


@:Shape = class(
    define ::(this) {
        @privateSize;
        
        this.constructor = ::{
            this.reset();
        }
        
        this.interface = {
            size : {
                set ::(value => Number) <- privateSize = value
            },
            
            reset ::{
                privateSize = 1;
            }   
        }
    }
);


@:smallShape = Shape.new();
smallShape.size = 0.001;




