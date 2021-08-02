@Point <= (x_, y_) {
    @x = gate(x_ == empty, 0, x_);
    @y = gate(y_ == empty, 0, y_);
    // new declarations are always "empty"
    @onMove;

    // object or something
    @instance = {
        getX : <= {return x;},
        getY : <= {return y;},

        moveTo : <= (newX, newY) {
            x = newX;
            y = newY;

            when(onMove != empty) : onMove();
        },

        setOnMove : <= (func) {
            onMove = func;
        }
    };
    
    instance.operator = {
        '+' : <= (a, b) {
            // refers back to function context
            return context(a.x + b.x, a.y + b.y);
        }
    };

    return instance;
};

@p1 = Point(100, 200);
@p2 = p1 + Point(300, 400);