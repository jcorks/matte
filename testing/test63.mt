////Test 48
//
// Core: Class (2)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(info:{
    define ::(this) {
        
    }
});
return ''+introspect.type(of:TestClass);
