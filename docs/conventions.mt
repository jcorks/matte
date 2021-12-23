// This example illustrates 
// using reference capture to 
// efficiently create new objects.

@ApproximateNumber ::<={
    // bound with every call.
    <@>type    = newtype(name:'ApproximateNumber');
    <@>epsilon = 0.00001;

    // the internals of the function 
    // access the constants above.
    return ::(from => Number) {
        @out = instantiate(type:type);
        out.data = from;

        setAttributes(
            of         : out, 
            attributes : {
            '==' ::(value => type) {
                return (
                    introspect.abs(of:value.data)-
                    introspect.abs(of:from)
                ) < epsilon;
            }
        });
        return out;
    };
};


@a = ApproximateNumber(from:0.044);
@b = ApproximateNumber(from:0.0440001);
@c = ApproximateNumber(from:0.045);

// should print true
print(message:a == b);

// should print false
print(message:a == c);