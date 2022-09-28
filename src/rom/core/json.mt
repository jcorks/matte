@:class = import(module:'Matte.Core.Class');

@:json_encode = getExternalFunction(name:"__matte_::json_encode");
@:json_decode = getExternalFunction(name:"__matte_::json_decode");

@JSON = {
    encode :: (object => Object) { 
        return json_encode(a:object);
    },
    
    
    decode ::(string => String) {
        return json_decode(a:string);
    }
};



return JSON;
