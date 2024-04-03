//// Test 126 
//
// layouts
@:Shape = Object.newType(
    name : 'Shape',
    layout : {
        label : String,
        numSides : Number,
        length : Number,
        area : Function
    }
);


@:SuperShape = Object.newType(
    name : 'SuperShape',
    inherits : [
        Shape
    ],
    
    layout : {
        depth : Number,
        waySet : Object,
    }
);


@:instance = Object.instantiate(type:SuperShape);

instance.label = 'Square';
instance.numSides = 4;
instance.length = 5;
instance.area = ::<- instance.length**2;
instance.depth = 10;




@out = '';

out = out + instance.label;
out = out + instance.waySet;
out = out + instance.numSides;
out = out + instance.area();
out = out + instance.depth;

{:::} {
    instance.grub = 10;
} : {
    onError::(message) {
        return out = out + 'nomem1'
    }
}


{:::} {
    instance.label = 100;
} : {
    onError::(message) {
        return out = out + 'notype'
    }
}



{:::} {
    Object.newType(
        inherits : [
            SuperShape
        ],
        layout : out
    );
} : {
    onError::(message) {
        return out = out + 'nolyt1'
    }
}

return out;
