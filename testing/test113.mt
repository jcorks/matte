//// Test 113
// "From the wild"
return ::<= {
    @:struct = ::<= {
        @:class = import(module:'Matte.Core.Class');

        return ::(
            name => String,
            items => Object,
            readOnly
        ) {
            @:type = Object.newType(name);
            @:itemNames = items->keys;
            @:itemTypes = {...items}

            return class(
                name: 'Testing.Struct',
                define:::(this){
                    this.interface = {
                        new ::(state => Object) {
                            @:initialized = {}
                            foreach(itemNames) ::(index, value) {
                                initialized[value] = false;
                            }
                            
                            @:data = {}
                            foreach(state)::(name, value) {
                                when (initialized[name] == empty)
                                    error(detail:'"' + name + '" is not a member of the structure ' + String(from:type));

                                when (itemTypes[name] != value->type)
                                    error(
                                        detail:'"' + name + '" should be of type ' 
                                        + String(from:itemTypes[name]) +  
                                        ', but given value was of type ' +
                                        String(from:value->type)
                                    );
                                data[name] = value;
                                initialized[name] = true;
                            }
                               
                            /*
                            foreach(in:initialized, do:::(name, initialized) {
                                if (initialized != true)
                                    error(detail:'"' + name + '" was never initialized in constructor.');
                            });  
                            */
                            
                            @:out = Object.instantiate(type);
                            
                            
                            @:reactor = {
                                get ::(key) {
                                    if (initialized[key] == empty)
                                        error(detail:'"' + key + '" is not a member of the structure ' + String(from:type));
                                    return data[key];
                                },

                                set: if (readOnly) 
                                            ::(key, value) {
                                                error(detail:'Structure ' + String(from:type) + ' is read-only.'); 
                                            }
                                        else
                                            ::(key, value) {
                                                when (initialized[key] == empty)
                                                    error(detail:'"' + name + '" is not a member of the structure ' + String(from:type));
                                            
                                                data[key] = value;
                                            }
                            }
                            
                            out->setAttributes(
                                attributes : {
                                    '[]' : reactor,
                                    '.'  : reactor,
                                    foreach ::<- data,
                                    values  ::<- data->values,
                                    keys    ::<- data->keys                                                    
                                }
                            );  
                            
                            
                            return out;
                        }
                    }
                }
            ).new();
            

        }
    }

    @:CANVAS_WIDTH  = 80;
    @:CANVAS_HEIGHT = 22;

    @:hints = {
        NEUTRAL : 0,
        GOOD : 1,
        BAD : 2,
        SUBDUED : 3,
        ALERT : 4,
        QUOTE : 5,
        SPEAKER : 6,
        PROMPT: 7,
        NEWLINE: 8,    
        CLEAR: 9
    }



    // converts a string into an array of characters.
    @:splay ::(string => String) {
        @:out = [];
        for(0, string->length) 
            ::(i) <- out->push(
                value:string->charAt(
                    index:i
                )
            )
        
        
        return out;
    }


    @:min ::(a => Number, b => Number) {
        when(a < b) a;
        return b;
    }

    @:TextIter = struct(
        name: 'Test.TextIter',
        items : {
            text : String,
            color : Number
        }
    );  
    
    @:inst = TextIter.new(state:{
        text : 'Hello!',
        color : 30
    });
    return inst.text + inst.color;
}

return '';
