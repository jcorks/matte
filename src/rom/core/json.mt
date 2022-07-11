@:class = import(module:'Matte.Core.Class');
@:hexToNumber = ::<= {
    @:table = {
        '0' : 0,
        '1' : 1,
        '2' : 2,
        '3' : 3,
        '4' : 4,
        '5' : 5,
        '6' : 6,
        '7' : 7,
        '8' : 8,
        '9' : 9,
        
        'a' : 10,
        'b' : 11,
        'c' : 12,
        'd' : 13,
        'e' : 14,
        'f' : 15
    };
    
    return ::(string){
        @out = 0;
        @:len = string->length;
        [0, len]->for(do:::(i) {
            out += table[string->charAt(index:i)] * (2**(len - (i+1)));
        });
        return out;
    };
};
@:cleanstring ::(in) {
    return in->replace(key:'"', with:'\\"');
}; 


@:IterString = class(
    define:::(this) {
        @iter = 0;
        @str;
        this.constructor = ::(src) {
            str = src;
            return this;        
        };
        this.interface = {
            peek ::(index){
                return str->charAt(index:iter+index);
            },
            
            skip :: {
                iter += 1;
            },
            
            substr ::(from, to) {
                str->substr(from, to);
            },
            
            next : {
                get :: {
                    @:out = str->charAt(index:iter);
                    iter+=1;
                    return out;
                }
            },
            
            length : {
                get :: {
                    return str->length - iter;
                }
            }
        };  
    }
);

@JSON = {
    encode :: (object) {  
        @:isNumber ::(value) {
            return value->type == Number;
        };      

        @encodeValue ::(obj){
            @encodeSub = context;
            return match(obj->type) {
                (Number): ''+obj,
                (String): '\"'+cleanstring(in:obj)+'\"',
                (Boolean): ''+obj,
                (Empty): 'null',
                (Object): ::{
                    // array case:
                    when (obj->keys->all(condition:isNumber)) ::<= {
                    
                    
                        @ostr = '[';
                        obj->foreach(do:::(k, v){
                            if (ostr != '[')::<={
                                ostr = ostr+',';
                            };
                            ostr = ostr + encodeSub(obj:v);
                        });
                        @ostr = ostr+']';
                        return ostr;    
                    };
                    @ostr = '{';
                    obj->foreach(do:::(k, v){
                        if (ostr != '{')::{
                            ostr = ostr+',';
                        }();
                        ostr = ostr + '\"'+cleanstring(in:String(from:k))+'\":'+encodeSub(obj:v);
                    });
                    @ostr = ostr+'}';
                    return ostr;
                }()
            };
        };
        
        return encodeValue(obj:object);
    },
    
    
    decode ::(string) {
        
        @:trimSpace::(substr) {
            listen(to:::{
                forever(do:::{
                    match(substr.peek(index:0)) {
                        // found whitespace. remove it and look again
                        (' ', '\r', '\n', '\t'): ::<={
                            substr.skip();
                        },
                        default:send() // end loop
                    };   
                });
            });
        };
        @:iter = IterString.new(src:string);
        @decodeValue::{
            trimSpace(substr:iter);
            @out = {};
            out.key = match(iter.peek(index:0)){
                
                // parse and consume string
                ('\"'): ::<={
                    // skip '"'
                    iter.skip();
                    @rawstr = '';
                    listen(to:::{
                        forever(do:::{
                            match(iter.peek(index:0)) {
                                // escape sequence
                                ('\\'): ::{
                                    iter.skip();
                                    match(iter.next) {
                                        ('n'): ::<={
                                            rawstr = rawstr + '\n';
                                        },
                                        ('r'): ::<={
                                            rawstr = rawstr + '\r';
                                        },
                                        ('t'): ::<={
                                            rawstr = rawstr + '\t';
                                        },
                                        ('b'): ::<={
                                            rawstr = rawstr + '\b';
                                        },
                                        ('"'): ::<={
                                            rawstr = rawstr + '"';
                                        },
                                        ('u'): ::<={
                                            @token = ' ';
                                            token = token->setCharCodeAt(
                                                value:hexToNumber(
                                                    string: iter.substr(
                                                        from:0,
                                                        to:3
                                                    )
                                                ),
                                                index: 0
                                            );
                                            rawstr = rawstr + token;
                                            iter.skip();
                                            iter.skip();
                                            iter.skip();
                                            iter.skip();
                                        },
                                        ('\\'): ::<={
                                            rawstr = rawstr + '\\';
                                        },

                                        default: error(data:'Unknown escape sequence.')
                                    };
                                }(),

                                // end of string 
                                ('"'): ::<={
                                    iter.skip();
                                    send();
                                },

                                default: ::<={
                                    rawstr = rawstr + iter.next;
                                }
                            };
                        });
                    });
                    return rawstr;
                },
                
                ('n'): ::<={
                    return (if (iter.peek(index:1) == 'u' &&
                                iter.peek(index:2) == 'l' &&
                                iter.peek(index:3) == 'l') ::<={
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        return empty;
                    } else ::<={
                        error(data:'Unrecognized token (expected null)');
                    });
                },           
                
                // true or false
                ('t'): ::<={
                    return (if (iter.peek(index:1) == 'r' &&
                                iter.peek(index:2) == 'u' &&
                                iter.peek(index:3) == 'e') ::<={
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        return true;
                    } else ::<={
                        error(data:'Unrecognized token (expected true)');
                    });
                },
                
                ('f'): ::<={
                    return (if (iter.peek(index:1) == 'a' &&
                                iter.peek(index:2) == 'l' &&
                                iter.peek(index:3) == 's' &&
                                iter.peek(index:4) == 'e') ::{
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        iter.skip();
                        return false;
                    } else ::<={
                        error(data:'Unrecognized token (expected true)');
                    });
                }(),
                
                
                // number
                ('.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::<={
                    @rawnumstr = iter.next;
                    listen(to:::{
                        forever(do:::{
                            return match(iter.peek(index:0)) {
                                ('0', '.', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::<={
                                    rawnumstr = rawnumstr + iter.peek(index:0);
                                    iter.skip();
                                },

                                default: send()
                            };       
                        });
                    });

                    return Number.parse(string:rawnumstr);
                },
                    
                   
                // object
                ('{'): ::<={
                    // skip '{'
                    iter.skip();
                    trimSpace(substr:iter);

                    // empty object!
                    when(iter.peek(index:0) == '}')::<={
                        iter.skip();                   
                        return {};
                    };
                    @out = {};

                    // loop over "key": value,
                    listen(to:::{
                        forever(do:::{

                            // get string
                            @res = decodeValue();
                            @key = res.key;
                            trimSpace(substr:iter);

                            // skip ':'
                            iter.skip();
                            trimSpace(substr:iter);

                            // get value
                            res = decodeValue();
                            @val = res.key;
                            trimSpace(substr:iter);

                            out[key] = val;

                            match(iter.next) {
                                (','): empty, // skip

                                // object over
                                ('}'): ::<={
                                    send();
                                },

                                default: ::<={
                                    error(message:"Unknown character");
                                }
                            };
                        });
                    });

                    return out;
                },



                // array
                ('['): ::<={
                    // skip '['
                    iter.skip();
                    trimSpace(substr:iter);

                    // empty object!
                    when(iter.peek(index:0) == ']')::<= {
                        iter.skip();
                        return [];
                    };
                    @arr = [];

                    // loop over value,
                    listen(to:::{
                        forever(do:::{
                            // get value
                            @res =  decodeValue(); 
                            @val = res.key;
                            trimSpace(substr:iter);

                            arr->push(value:val);

                            match(iter.next) {
                                (','): empty,

                                // object over
                                (']'): ::<={
                                    send();
                                },

                                default: ::<={
                                    error(message:"Unknown character");
                                }
                            };
                        });
                    });

                    return arr;
                },


                default: error(detail:"Could not parse object \""+iter+'\"')
            };        
            return out;
        };

        return decodeValue().key;
    }
};



return JSON;
