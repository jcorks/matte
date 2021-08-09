
// Returns whether the input object 
// has a member named "testValue" 
// that is set to true.
<@>Point <- {
    @x = 0;
    @y = 0;

    return {
        getX :<- {return x;},
        getY :<- {return y;},

        move :<- (newX, newY) {
            x = newX;
            y = newY;
            print("moved!: "+x+','+y);
        },

        otherVar : 102
    };
};


@p = Point();
p.move(10, 10);


foreach(p, <-(key, val) {
    print(key + " : " + introspect(val).type());
});
