// This example illustrates 
// using reference capture to 
// efficiently create new objects.

@ApproximateNumber ::<={
    // bound with every call.
    @:type    = Object.newType(name:'ApproximateNumber');
    @:epsilon = 0.00001;

    // the internals of the function 
    // access the constants above.
    return ::(val => Number) {
        @out = Object.instantiate(type);
        out.data = val;

        out->setAttributes(:{
            '==' ::(value => type) {
                return (
                    value.data-
                           val
                )->abs < epsilon;
            }
        });
        return out;
    }
}


@a = ApproximateNumber(:0.044);
@b = ApproximateNumber(:0.0440001);
@c = ApproximateNumber(:0.045);

// should print true
print(:a == b);

// should print false
print(:a == c);
