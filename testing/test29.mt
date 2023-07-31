//// Test 28
//
// object test: literal
@:Point = ::{
    @x = 0;
    @y = 0;

    return {
        getX : ::{return x;},
        getY : ::{return y;},

        move : ::(newX, newY) {
            x = newX;
            y = newY;
        },

        otherVar : 102
    }
}


@p = Point();
p.move(newX:14, newY:23);

return '' + p.getX() + p.getY();
