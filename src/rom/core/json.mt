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
        for(in:[0, len], do:::(i) {
            out += table[string->charAt(index:i)] * (2**(len - (i+1)));
        });
        return out;
    };
};

@JSON = {
    encode :: (object) {        
        @encodeValue ::(obj){
            @encodeSub = context;
            return match(obj->type) {
                (Number): ''+obj,
                (String): '\"'+obj+'\"',
                (Boolean): ''+obj,
                (Object): ::{
                    @ostr = '{';
                    foreach(in:obj, do:::(k, v){
                        if (ostr != '{')::{
                            ostr = ostr+',';
                        }();
                        ostr = ostr + '\"'+String(from:k)+'\":'+encodeSub(obj:v);
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
                    match(substr->charAt(index:0)) {
                        // found whitespace. remove it and look again
                        (' ', '\r', '\n', '\t'): ::<={
                            substr = substr->removeChar(index:0);
                        },
                        default:send() // end loop
                    };   
                });
            });
            return substr;
        };

        @decodeValue::(iter){
            @:decodeV = context;
            iter = trimSpace(substr:iter);
            @out = {};
            out.key = match(iter->charAt(index:0)){
                
                // parse and consume string
                ('\"'): ::{
                    // skip '"'
                    iter = iter->removeChar(index:0);
                    @rawstr = '';
                    listen(to:::{
                        forever(do:::{
                            match(iter->charAt(index:0)) {
                                // escape sequence
                                ('\\'): ::{
                                    iter = iter->removeChar(index:0);
                                    match(iter->charAt(index:0)) {
                                        ('n'): ::{
                                            rawstr = rawstr + '\n';
                                        }(),
                                        ('r'): ::{
                                            rawstr = rawstr + '\r';
                                        }(),
                                        ('t'): ::{
                                            rawstr = rawstr + '\t';
                                        }(),
                                        ('b'): ::{
                                            rawstr = rawstr + '\b';
                                        }(),
                                        ('u'): ::{
                                            @token = ' ';
                                            token = token->setCharCodeAt(
                                                value:hexToNumber(
                                                    string: iter->substr(
                                                        from:1,
                                                        to:4,
                                                    )
                                                ),
                                                index: 0
                                            );
                                            rawstr = rawstr + token;
                                            iter = iter->removeChar(index:1);
                                            iter = iter->removeChar(index:1);
                                            iter = iter->removeChar(index:1);
                                            iter = iter->removeChar(index:1);
                                        }(),
                                        ('\\'): ::{
                                            rawstr = rawstr + '\\';
                                        }(),

                                        default: error(data:'Unknown escape sequence.')
                                    };
                                    iter = iter->removeChar(index:0);
                                }(),

                                // end of string 
                                ('"'): ::{
                                    iter = iter->removeChar(index:0);
                                    send();
                                }(),

                                default: ::{
                                    rawstr = rawstr + iter->charAt(index:0);
                                    iter = iter->removeChar(index:0);
                                    send();
                                }()
                            };
                        });
                    });
                    return rawstr;
                }(),
                
                
                
                // true or false
                ('t'): ::{
                    return (if (iter->charAt(index:1) == 'r' &&
                                iter->charAt(index:2) == 'u' &&
                                iter->charAt(index:3) == 'e') ::{
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        return true;
                    }() else ::{
                        error(data:'Unrecognized token (expected true)');
                    }());
                }(),
                
                ('f'): ::{
                    return (if (iter->charAt(index:1) == 'a' &&
                                iter->charAt(index:2) == 'l' &&
                                iter->charAt(index:3) == 's' &&
                                iter->charAt(index:4) == 'e') ::{
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        iter = iter->removeChar(index:0);
                        return false;
                    }() else ::{
                        error(data:'Unrecognized token (expected true)');
                    }());
                }(),
                
                
                // number
                ('.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::{
                    @rawnumstr = iter->charAt(index:0);
                    iter = iter->removeChar(index:0);
                    listen(to:::{
                        forever(do:::{
                            return match(iter->charAt(index:0)) {
                                ('0', '.', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::<={
                                    rawnumstr = rawnumstr + iter->charAt(index:0);
                                    iter = iter->removeChar(index:0);
                                },

                                default: send()
                            };       
                        });
                    });

                    return Number.parse(string:rawnumstr);
                }(),
                    
                   
                // object
                ('{'): ::{
                    // skip '{'
                    iter = iter->removeChar(index:0);
                    iter = trimSpace(substr:iter);

                    // empty object!
                    when(iter->charAt(index:0) == '}')::<={
                        iter = iter->removeChar(index:0);                        
                        return {};
                    };
                    @out = {};

                    // loop over "key": value,
                    listen(to:::{
                        forever(do:::{

                            // get string
                            @res = decodeV(iter:iter);
                            @key = res.key;
                            iter = res.iter;
                            iter = trimSpace(substr:iter);

                            // skip ':'
                            iter = iter->removeChar(index:0);
                            iter = trimSpace(substr:iter);

                            // get value
                            res = decodeV(iter:iter);
                            @val = res.key;
                            iter = res.iter;
                            iter = trimSpace(substr:iter);

                            out[key] = val;

                            match(iter->charAt(index:0)) {
                                (','): ::{
                                    iter = iter->removeChar(index:0);
                                }(),

                                // object over
                                ('}'): ::{
                                    iter = iter->removeChar(index:0);
                                    send();
                                }(),

                                default: ::{
                                    error(message:"Unknown character");
                                }()
                            };
                        });
                    });

                    return out;
                }(),



                // array
                ('['): ::{
                    // skip '['
                    iter = iter->removeChar(index:0);
                    iter = trimSpace(substr:iter);

                    // empty object!
                    when(iter->charAt(index:0) == ']')::<= {
                        iter = iter->removeChar(index:0);
                        return [];
                    };
                    @arr = [];

                    // loop over value,
                    listen(to:::{
                        forever(do:::{
                            // get value
                            @res =  decodeV(iter:iter); 
                            @val = res.key;
                            iter = res.iter;
                            iter = trimSpace(substr:iter);

                            Object.push(object:arr, value:val);

                            match(iter->charAt(index:0)) {
                                (','): ::<={
                                    iter = iter->removeChar(index:0);
                                },

                                // object over
                                (']'): ::<={
                                    iter = iter->removeChar(index:0);
                                    send();
                                },

                                default: ::<={
                                    error(message:"Unknown character");
                                }
                            };
                        });
                    });

                    return arr;
                }(),


                default: error(detail:"Could not parse object \""+iter+'\"')
            };        
            out.iter = iter;
            return out;
        };

        return decodeValue(iter:string).key;
    }
};



return JSON;
