//// Test 120
// class base constructor test
//
@:class = import(module:'Matte.Core.Class');


@Shape = class(
    define ::(this) {
        @size_ = 0;
        
        this.interface = {
            init ::(size) {
                size_ = size;
                return this;
            },

            size : {
                get ::<- size_
            }
        }
    }
);

@Point = class(
    inherits : [Shape],
    statics : {
        a :::<= {
            @val = 10;
            return {
                get ::<- val,
                set ::(value) <- val = value
            }
        },
        all : ::<= {
            @data = [];
            return  {get::<- data}
        }
    },
    define ::(this) {
        @x_ = 1;
        @y_ = 1;

        this.init(size:10);
        
        this.interface = {
            init ::(x, y) {
                x_ = x;
                y_ = y;
                Point.all->push(value:this);
                return this;
            },


            x : {
                get ::<- x_
            },
            
            y : {
                get ::<- y_
            },
            
            compose::{
                return x_ * 10 + y_;
            }
        }
    }
);


@:p1 = Point.new().init(x: 10, y:20);
Point.a = 'aaaa';
return '' + p1.compose() + Point.a + Point.all->keycount + p1.size;


