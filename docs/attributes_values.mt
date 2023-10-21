@:a = {};

a->setAttributes(
    attributes : {
        'values' :: {
            @output = {};
            for(0, a->size) ::(i) {
                @str = a[i];
                for(0, str->length) ::(n) {
                    output[str->charAt(index:n)] = true;
                }
            }
            return output->keys;
        }
    }
);


a->push(value:'Hello');
a->push(value:'World');

@outStr = '';
foreach(a->values) ::(index, value) {
    outStr = outStr + value + ' ';
}

// Should print a series of the characters within 
// Hello and World with no repeats in some order.
print(message: outStr);
 
