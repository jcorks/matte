@: createStruct::(
    members => Object
) {
    @:names = {};
    @:storage = {};
    @:wrapped = {};
    
    foreach(members) ::(key, value) {
        names[value] = true;
    }
    
    @:check::(key) {
        if (names[key] == empty)
            error(: 'No such member ' + key + ' within struct.');    
    }
    
    wrapped->setAttributes(:{
        '.' : {
            get ::(key) {
                check(key);
                return storage[key];
            },
            
            set ::(key, value) {
                check(key);
                return storage[key] = value;
            }
        }
    });  
    
    return wrapped;
};


@:a = createStruct(
    members : [
        "x",
        "y"
    ]
);

// Ok!
a.x = 10;
a.y = 10;

// Would generate an error.
a.length = 100;

// Should return 20
print(:a.x + a.y);

// Should return empty, as it invokes 
// bracket access.
print(:a['x']);






