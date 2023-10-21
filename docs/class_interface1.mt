@:class = import(module:'Matte.Core.Class');


@:Shape = class(
    define ::(this) {
        @privateSize = 1;
        
        this.interface = {
            setSize ::(newSize) {
                privateSize = newSize;
            }
        }
    }
);


@:smallShape = Shape.new();
smallShape.setSize(newSize: 0.001);




