@String = import('Matte.String');
@JSON = {
    encode :: (obj) {        
        @encodeValue ::(obj){
            @encodeSub = context;
            return match(introspect(obj).type()) {
                ('number'): ''+obj,
                ('string'): '\"'+obj+'\"',
                ('boolean'): ''+obj,
                ('object'): ::{
                    @ostr = '{';
                    foreach(obj, ::(k, v){
                        if (ostr != '{')::{
                            ostr = ostr+',';
                        }();
                        ostr = ostr + '\"'+AsString(k)+'\":'+encodeSub(v);
                    });
                    @ostr = ostr+'}';
                    return ostr;
                }()
            };
        };
        
        return encodeValue(obj);
    },
    
    
    decode ::(str) {
        @str = String(str);
        
        @trimSpace::(substr) {
            loop(::{
                when(match(substr.charAt(0)) {
                    ' ', '\r', '\n', '\t' : false,
                    default: true
                }): false;
                
                substr.removeChar(0);
                return true;
            });
        }

        @decodeValue::(substr){
            trimSpace(substr);
            return match(substr.charAt(0)){
                
                // string 
                ('\"'): ::{
                    return substr.substr(1, substr.length-2).data;
                },
                
                
                
                // true or error
                ('t'): ::{
                    when(substr.data == 'true'): true;
                }(),
                
                ('f'): ::{
                    when(substr.data == 'false'): true;
                }(),
                
                
                // number
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E':
                    AsNumber(substr.data),
                    
                    
                   
                // object
                ('{'): ::{
                    // loop over "key": value,
                    trimSpace(
                
                }(),
                
                default: error("Could not parse object \""+substr+'\"')
            }        
        }
    }
}




print(JSON.encode({
    testing : 'ohhh',
    othertesting : {
        ithink : 'i see',
        theanswer: 42,
        owwow:true
    }
}));
