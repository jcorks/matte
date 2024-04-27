@:createStrictObject :: {
    @storage = {};
    @wrapped = {};
    wrapped->setAttributes(:{
        '[]' : {
            get ::(key) {
                if (storage[key] == empty)
                    error(:'No key available');
                return storage[key];
            },
            
            set ::(key, value) {
                if (key->type == Number) 
                    error(:'No number keys are allowed');
                if (value->type != Number) 
                    error(:'Only number values are allowed');
                storage[key] = value + 10;
                return value + 100;
            }
        }
    });
    return wrapped;
}

@:a = createStrictObject();


// This would throw an error, as it would 
// invoke the bracket access getter behavior
print(:a["hello"]);


// This would throw an error, as it would 
// invoke the bracket access setter behavior
a[10] = "hi!";


// should print 300
print(:a["test"] = 200);

// Should print 210
print(:a["test"]);

// Should print empty, as 
// the dot accessor is not overridden, and will 
// point to default object storage.
print(:a.test);

