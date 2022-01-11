@JSON = {
    encode :: (object) {        
        @encodeValue ::(obj){
            @encodeSub = context;
            return match(getType(of:obj)) {
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
            loop(func:::{
                return match(String.charAt(string:substr, index:0)) {
                    // found whitespace. remove it and look again
                    (' ', '\r', '\n', '\t'): ::{
                        substr = String.removeChar(string:substr, index:0);
                        return true;
                    }(),

                    // loop is over
                    default : false
                };                
            });
            return substr;
        };

        @decodeValue::(iter){
            @:decodeV = context;
            iter = trimSpace(substr:iter);
            @out = {};
            out.key = match(String.charAt(string:iter, index:0)){
                
                // parse and consume string
                ('\"'): ::{
                    // skip '"'
                    iter = String.removeChar(string:iter, index:0);
                    @rawstr = '';
                    loop(func:::{
                        return match(String.charAt(string:iter, index:0)) {
                            // escape sequence
                            ('\\'): ::{
                                iter = String.removeChar(string:iter, index:0);
                                match(String.charAt(string:iter, index:0)) {
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
                                    ('\\'): ::{
                                        rawstr = rawstr + '\\';
                                    }(),

                                    default: error(data:'Unknown escape sequence.')
                                };
                                iter = String.removeChar(string:iter, index:0);
                                return true;
                            }(),

                            // end of string 
                            ('"'): ::{
                                iter = String.removeChar(string:iter, index:0);
                                return false;
                            }(),

                            default: ::{
                                rawstr = rawstr + String.charAt(string:iter, index:0);
                                iter = String.removeChar(string:iter, index:0);
                                return true;
                            }()
                        };
                    });
                    return rawstr;
                }(),
                
                
                
                // true or false
                ('t'): ::{
                    return (if (String.charAt(string:iter, index:1) == 'r' &&
                                String.charAt(string:iter, index:2) == 'u' &&
                                String.charAt(string:iter, index:3) == 'e') ::{
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        return true;
                    }() else ::{
                        error(data:'Unrecognized token (expected true)');
                    }());
                }(),
                
                ('f'): ::{
                    return (if (String.charAt(string:iter, index:1) == 'a' &&
                                String.charAt(string:iter, index:2) == 'l' &&
                                String.charAt(string:iter, index:3) == 's' &&
                                String.charAt(string:iter, index:4) == 'e') ::{
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        iter = String.removeChar(string:iter, index:0);
                        return false;
                    }() else ::{
                        error(data:'Unrecognized token (expected true)');
                    }());
                }(),
                
                
                // number
                ('.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::{
                    @rawnumstr = String.charAt(string:iter, index:0);
                    iter = String.removeChar(string:iter, index:0);

                    loop(func:::{
                        return match(String.charAt(string:iter, index:0)) {
                            ('0', '.', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::<={
                                rawnumstr = rawnumstr + String.charAt(string:iter, index:0);
                                iter = String.removeChar(string:iter, index:0);
                                return true;
                            },

                            default: false
                        };       
                    });

                    return Number(from:rawnumstr);
                }(),
                    
                   
                // object
                ('{'): ::{
                    // skip '{'
                    iter = String.removeChar(string:iter, index:0);
                    iter = trimSpace(substr:iter);

                    // empty object!
                    when(String.charAt(string:iter, index:0) == '}')::<={
                        iter = String.removeChar(string:iter, index:0);                        
                        return {};
                    };
                    @out = {};

                    // loop over "key": value,
                    loop(func:::{

                        // get string
                        @res = decodeV(iter:iter);
                        @key = res.key;
                        iter = res.iter;
                        iter = trimSpace(substr:iter);

                        // skip ':'
                        iter = String.removeChar(string:iter, index:0);
                        iter = trimSpace(substr:iter);

                        // get value
                        res = decodeV(iter:iter);
                        @val = res.key;
                        iter = res.iter;
                        iter = trimSpace(substr:iter);

                        out[key] = val;

                        return match(String.charAt(string:iter, index:0)) {
                            (','): ::{
                                iter = String.removeChar(string:iter, index:0);
                                return true;
                            }(),

                            // object over
                            ('}'): ::{
                                iter = String.removeChar(string:iter, index:0);
                                return false;
                            }(),

                            default: ::{
                                error(message:"Unknown character");
                            }()
                        };
                    });

                    return out;
                }(),



                // array
                ('['): ::{
                    // skip '['
                    iter = String.removeChar(string:iter, index:0);
                    iter = trimSpace(substr:iter);

                    // empty object!
                    when(String.charAt(string:iter, index:0) == ']')::<= {
                        iter = String.removeChar(string:iter, index:0);
                        return [];
                    };
                    @arr = [];

                    // loop over value,
                    loop(func:::{
                        // get value
                        @res =  decodeV(iter:iter); 
                        @val = res.key;
                        iter = res.iter;
                        iter = trimSpace(substr:iter);

                        Object.push(object:arr, value:val);

                        return match(String.charAt(string:iter, index:0)) {
                            (','): ::<={
                                iter = String.removeChar(string:iter, index:0);
                                return true;
                            },

                            // object over
                            (']'): ::<={
                                iter = String.removeChar(string:iter, index:0);
                                return false;
                            },

                            default: ::<={
                                error(message:"Unknown character");
                            }
                        };
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
