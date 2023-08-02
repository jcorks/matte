//// Test 120
// class base constructor test
//
@:class = import(module:'Matte.Core.Class');


@Shape = class(
    define ::(this) {
        @size_ = 0;
        
        this.constructor = ::(size) {
            size_ = size;
            return this.instance;
        }
        
        this.interface = {
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
        
        this.constructor = ::(x, y) {
            x_ = x;
            y_ = y;
            this.baseConstructor[Shape](size:10)
            Point.all->push(value:this.instance);
            return this.instance;
        }

        
        this.interface = {
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


@:p1 = Point.new(x: 10, y:20);
Point.a = 'aaaa';
return '' + p1.compose() + Point.a + Point.all->keycount + p1.size;


