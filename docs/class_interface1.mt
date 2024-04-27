@:class = import(:'Matte.Core.Class');


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
smallShape.setSize(: 0.001);




