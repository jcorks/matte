@:a = {};


a->setAttributes(:{
    'keys' ::{
        @:keys = [];
        for(0, a->size) ::(i){
            keys->push(:match(i) {
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
});



a->push(:'a');
a->push(:'b');
a->push(:'c');

// Should print
//  zero
//  one 
//  two
foreach(a->keys) ::(index, key) {
    print(:key);
}

