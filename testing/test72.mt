////Test 57
//
// Core: String (5)
@:containsAny::(in, keys) {
    return ::?{
        keys->foreach(do:::(k, v) {
            when(in->contains(key:v)) send(message:true);
        });
        return false;        
    }
}

@str = "testinBdwatest";
return ''+containsAny(in:str, keys:['z', 'x', 'y'])+
          containsAny(in:str, keys:['f', 'f', 'a'])+
          containsAny(in:str, keys:['xz', '', 'ss'])+
          containsAny(in:str, keys:['tesst', 'BBda', 'estt'])+
          containsAny(in:str, keys:['test', 'Bdw', 'est'])+
          containsAny(in:str, keys:['ttt', 'tin', 'eee']);


