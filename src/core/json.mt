@MString = import('Matte.String');
@Array = import('Matte.Array');
@JSON = {
    name : 'Matte.JSON',
    encode :: (obj) {        
        @encodeValue ::(obj){
            @encodeSub = context;
            return match(introspect.type(obj)) {
                (Number): ''+obj,
                (String): '\"'+obj+'\"',
                (Boolean): ''+obj,
                (Object): ::{
                    @ostr = '{';
                    foreach(obj, ::(k, v){
                        if (ostr != '{')::{
                            ostr = ostr+',';
                        }();
                        ostr = ostr + '\"'+String(k)+'\":'+encodeSub(v);
                    });
                    @ostr = ostr+'}';
                    return ostr;
                }()
            };
        };
        
        return encodeValue(obj);
    },
    
    
    decode ::(str) {
        str = MString.new(str);
        
        <@>trimSpace::(substr) {
            loop(::{
                return match(substr.charAt(0)) {
                    // found whitespace. remove it and look again
                    (' ', '\r', '\n', '\t'): ::{
                        substr.removeChar(0);
                        return true;
                    }(),

                    // loop is over
                    default : false
                };                
            });
        };

        @decodeValue::(iter){
            <@>decodeV = context;
            trimSpace(iter);
            return match(iter.charAt(0)){
                
                // parse and consume string
                ('\"'): ::{
                    // skip '"'
                    iter.removeChar(0);
                    @rawstr = '';
                    loop(::{
                        return match(iter.charAt(0)) {
                            // escape sequence
                            ('\\'): ::{
                                iter.removeChar(0);
                                match(iter.charAt(0)) {
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

                                    default: error('Unknown escape sequence.')
                                };
                                iter.removeChar(0);
                                return true;
                            }(),

                            // end of string 
                            ('"'): ::{
                                iter.removeChar(0);
                                return false;
                            }(),

                            default: ::{
                                rawstr = rawstr + iter.charAt(0);
                                iter.removeChar(0);
                                return true;
                            }()
                        };
                    });
                    return rawstr;
                }(),
                
                
                
                // true or false
                ('t'): ::{
                    return (if (iter.charAt(1) == 'r' &&
                                iter.charAt(2) == 'u' &&
                                iter.charAt(3) == 'e') ::{
                        iter.removeChar(0);
                        iter.removeChar(0);
                        iter.removeChar(0);
                        iter.removeChar(0);
                        return true;
                    }() else ::{
                        error('Unrecognized token (expected true)');
                    }());
                }(),
                
                ('f'): ::{
                    return (if (iter.charAt(1) == 'a' &&
                                iter.charAt(2) == 'l' &&
                                iter.charAt(3) == 's' &&
                                iter.charAt(4) == 'e') ::{
                        iter.removeChar(0);
                        iter.removeChar(0);
                        iter.removeChar(0);
                        iter.removeChar(0);
                        iter.removeChar(0);
                        return false;
                    }() else ::{
                        error('Unrecognized token (expected true)');
                    }());
                }(),
                
                
                // number
                ('.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::{
                    @rawnumstr = iter.charAt(0);
                    iter.removeChar(0);

                    loop(::{
                        match(iter.charAt(0)) {
                            ('.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E'): ::{
                                rawnumstr = rawnumstr + iter.charAt(0);
                                iter.removeChar(0);
                                return true;
                            }(),

                            default: false
                        };       
                    });

                    return Number(rawnumstr);
                }(),
                    
                   
                // object
                ('{'): ::{
                    // skip '{'
                    iter.removeChar(0);
                    trimSpace(iter);

                    // empty object!
                    when(iter.charAt(0) == '}'){};

                    @out = {};

                    // loop over "key": value,
                    loop(::{

                        // get string
                        @key = decodeV(iter);
                        trimSpace(iter);

                        // skip ':'
                        iter.removeChar(0);
                        trimSpace(iter);

                        // get value
                        @val = decodeV(iter);
                        trimSpace(iter);

                        out[key] = val;

                        return match(iter.charAt(0)) {
                            (','): ::{
                                iter.removeChar(0);
                                return true;
                            }(),

                            // object over
                            ('}'): ::{
                                iter.removeChar(0);
                                return false;
                            }(),

                            default: ::{
                                error("Unknown character");
                            }()
                        };
                    });

                    return out;
                }(),



                // array
                ('['): ::{
                    // skip '['
                    iter.removeChar(0);
                    trimSpace(iter);

                    // empty object!
                    when(iter.charAt(0) == ']')[];

                    @arr = Array.new();

                    // loop over value,
                    loop(::{
                        // get value
                        @val = decodeV(iter);
                        trimSpace(iter);

                        arr.push(val);

                        return match(iter.charAt(0)) {
                            (','): ::{
                                iter.removeChar(0);
                                return true;
                            }(),

                            // object over
                            (']'): ::{
                                iter.removeChar(0);
                                return false;
                            }(),

                            default: ::{
                                error("Unknown character");
                            }()
                        };
                    });

                    return arr.data;
                }(),


                default: error("Could not parse object \""+iter+'\"')
            };        
        };

        return decodeValue(str);
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
