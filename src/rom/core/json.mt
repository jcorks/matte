@MString = import(module:'Matte.Core.String');
@Array = import(module:'Matte.Core.Array');
@JSON = {
    encode :: (obj) {        
        @encodeValue ::(obj){
            @encodeSub = context;
            return match(introspect.type(of:obj)) {
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
        
        return encodeValue(obj:obj);
    },
    
    
    decode ::(str) {
        str = MString.new(from:str);
        
        @:trimSpace::(substr) {
            loop(function:::{
                return match(substr.charAt(index:0)) {
                    // found whitespace. remove it and look again
                    (' ', '\r', '\n', '\t'): ::{
                        substr.removeChar(index:0);
                        return true;
                    }(),

                    // loop is over
                    default : false
                };                
            });
        };

        @decodeValue::(iter){
            @:decodeV = context;
            trimSpace(substr:iter);
            return match(iter.charAt(index:0)){
                
                // parse and consume string
                ('\"'): ::{
                    // skip '"'
                    iter.removeChar(index:0);
                    @rawstr = '';
                    loop(function:::{
                        return match(iter.charAt(index:0)) {
                            // escape sequence
                            ('\\'): ::{
                                iter.removeChar(index:0);
                                match(iter.charAt(index:0)) {
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
                                iter.removeChar(index:0);
                                return true;
                            }(),

                            // end of string 
                            ('"'): ::{
                                iter.removeChar(index:0);
                                return false;
                            }(),

                            default: ::{
                                rawstr = rawstr + iter.charAt(index:0);
                                iter.removeChar(index:0);
                                return true;
                            }()
                        };
                    });
                    return rawstr;
                }(),
                
                
                
                // true or false
                ('t'): ::{
                    return (if (iter.charAt(index:1) == 'r' &&
                                iter.charAt(index:2) == 'u' &&
                                iter.charAt(index:3) == 'e') ::{
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        return true;
                    }() else ::{
                        error(data:'Unrecognized token (expected true)');
                    }());
                }(),
                
                ('f'): ::{
                    return (if (iter.charAt(index:1) == 'a' &&
                                iter.charAt(index:2) == 'l' &&
                                iter.charAt(index:3) == 's' &&
                                iter.charAt(index:4) == 'e') ::{
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        iter.removeChar(index:0);
                        return false;
                    }() else ::{
                        error(data:'Unrecognized token (expected true)');
                    }());
                }(),
                
                
                // number
                ('.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::{
                    @rawnumstr = iter.charAt(index:0);
                    iter.removeChar(index:0);

                    loop(function:::{
                        return match(iter.charAt(index:0)) {
                            ('0', '.', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::<={
                                rawnumstr = rawnumstr + iter.charAt(index:0);
                                iter.removeChar(index:0);
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
                    iter.removeChar(index:0);
                    trimSpace(substr:iter);

                    // empty object!
                    when(iter.charAt(index:0) == '}'){};

                    @out = {};

                    // loop over "key": value,
                    loop(function:::{

                        // get string
                        @key = decodeV(iter:iter);
                        trimSpace(substr:iter);

                        // skip ':'
                        iter.removeChar(index:0);
                        trimSpace(substr:iter);

                        // get value
                        @val = decodeV(iter:iter);
                        trimSpace(substr:iter);

                        out[key] = val;

                        return match(iter.charAt(index:0)) {
                            (','): ::{
                                iter.removeChar(index:0);
                                return true;
                            }(),

                            // object over
                            ('}'): ::{
                                iter.removeChar(index:0);
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
                    iter.removeChar(index:0);
                    trimSpace(substr:iter);

                    // empty object!
                    when(iter.charAt(index:0) == ']')[];

                    @arr = Array.new();

                    // loop over value,
                    loop(functio:::{
                        // get value
                        @val = decodeV(iter:iter);
                        trimSpace(substr:iter);

                        arr.push(value:val);

                        return match(iter.charAt(index:0)) {
                            (','): ::{
                                iter.removeChar(index:0);
                                return true;
                            }(),

                            // object over
                            (']'): ::{
                                iter.removeChar(index:0);
                                return false;
                            }(),

                            default: ::{
                                error(message:"Unknown character");
                            }()
                        };
                    });

                    return arr.data;
                }(),


                default: error(message:"Could not parse object \""+iter+'\"')
            };        
        };

        return decodeValue(iter:str);
    }
};



/*
print(JSON.encode({
    testing : 'ohhh',
    othertesting : {
        ithink : 'i see',
        theanswer: 42,
        owwow:true
    }
}));



@pObject ::(o) {
    @already = {};
    @pspace ::(level) {
        @str = '';
        for([0, level], ::{
            str = str + ' ';
        });
        return str;
    };
    @helper ::(obj, level) {
        @poself = context;

        return match(introspect(obj).type()) {
            (String) :    obj,
            (Number) : ''+obj,
            (Boolean): ''+obj,
            (Empty)  : 'empty',
            (Object) : ::{
                when(already[obj] == true) '[already printed]';
                 
                @output = '{\n';
                foreach(obj, ::(key, val) {
                    output = output + pspace(level)+(String(key))+' : '+poself(val, level+1) + ',\n';
                });
                output = output + pspace(level) + '}\n';
                already[obj] = true;
                return output;                
            }()
        };
    };
    print(helper(o, 0));
};


@obj = '[
  {
    "ID": 1,
    "Name": "Edward the Elder",
    "Country": "United Kingdom",
    "House": "House of Wessex",
    "Reign": "899-925"
  },
  {
    "ID": 2,
    "Name": "Athelstan",
    "Country": "United Kingdom",
    "House": "House of Wessex",
    "Reign": "925-940"
  },
  {
    "ID": 3,
    "Name": "Edmund",
    "Country": "United Kingdom",
    "House": "House of Wessex",
    "Reign": "940-946"
  },
  {
    "ID": 4,
    "Name": "Edred",
    "Country": "United Kingdom",
    "House": "House of Wessex",
    "Reign": "946-955"
  },
  {
    "ID": 5,
    "Name": "Edwy",
    "Country": "United Kingdom",
    "House": "House of Wessex",
    "Reign": "955-959"
  }
]';
*/
//JSON.decode(obj));

return JSON;
