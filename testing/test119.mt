//// Test 119
// instantiate -> interface
@out = ''

@:createPoint = ::<= {
    @:type = Object.newType();

    return ::{
        @x_ = 0;
        @y_ = 0;
        @iter_ = 2;
        @setOnly_ = 10;
        @obj = Object.instantiate(type);
        
        @interface = {
            move ::(x, y) {
                x_ = x;
                y_ = y;
            },
            
            x : {
                get ::<- x_
            },

            y : {
                get ::<- y_
            },
            
            setOnly : {
                set ::(value) {
                    setOnly_ = value;
                }
            },
            
            iter : {
                set ::(value) <- iter_ = value,
                get ::<- iter_
            },
            
            'print' ::{
                out = out + setOnly_ + '' + x_ + ',' + y_; 
            }
        }
        
        foreach(interface)::(k, v) {
            obj[k] = v;
        }
        
        obj->setIsInterface(enabled:true);
        return obj;
    }
}



@:p = createPoint();
breakpoint();

p.move(x: 5, y: 6);
p.setOnly = '-';
p.iter += 10;
out = out + p.iter;
p.print();
out = out + (p.x + p.y);

{:::} {
    p.hi = 10;
} : {
    onError ::(message) {
        out = out + 'noMember'
    }
}


{:::} {
    p.x = 10;
} : {
    onError ::(message) {
        out = out + 'noWrite'
    }
}

{:::} {
    p.move = 10;
} : {
    onError ::(message) {
        out = out + 'noWriteFn'
    }
}

{:::} {
    p.x = p.setOnly;
} : {
    onError ::(message) {
        out = out + 'noRead'
    }
}




return out;

