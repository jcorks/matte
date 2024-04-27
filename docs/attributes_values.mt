@:a = {};

a->setAttributes(:{
    'values' :: {
        @output = {};
        for(0, a->size) ::(i) {
            @str = a[i];
            for(0, str->length) ::(n) {
                output[str->charAt(:n)] = true;
            }
        }
        return output->keys;
    }
});


a->push(:'Hello');
a->push(:'World');

@outStr = '';
foreach(a->values) ::(index, value) {
    outStr = outStr + value + ' ';
}

// Should print a series of the characters within 
// Hello and World with no repeats in some order.
print(:outStr);
 
