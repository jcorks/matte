////Test 48
//
// Core: Class (2)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(definition:{
    instantiate ::(this, thisType) {
        
    }
});
return ''+introspect.type(of=:TestClass);
