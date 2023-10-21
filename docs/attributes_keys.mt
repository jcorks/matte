@:a = {};


a->setAttributes(
    attributes : {
        'keys' ::{
            @:keys = [];
            for(0, a->size) ::(i){
                keys->push(value:match(i) {
                    (0): 'zero',
                    (1): 'one',
                    (2): 'two',
                    (3): 'three',
                    (4): 'four',
                    default: 'greater-than-four'
                });
            }
            return keys;
        }
    }
);



a->push(value:'a');
a->push(value:'b');
a->push(value:'c');

// Should print
//  zero
//  one 
//  two
foreach(a->keys) ::(index, key) {
    print(message:key);
}

