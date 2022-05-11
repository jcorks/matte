@:class = import(module:'Matte.Core.Class');

@:_sharedlibrary_open = getExternalFunction(name:"_sharedlibrary_open");
@:_sharedlibrary_getCallable = getExternalFunction(name:"_sharedlibrary_getSymbol");
@:_sharedlibrary_callCallable = getExternalFunction(name:"_sharedlibrary_getSymbol");

@SharedLibrary = class(
    name: 'Matte.System.MemoryBuffer',
    statics : {
        TYPES : {
            VOID : 0
            INT : 1,
            DOUBLE : 2,
            FLOAT : 3,
            CHAR : 4,
            CSTR : 5,
            POINTER : 6,
            UINT : 7,
            INT8_T : 8,
            INT16_T : 9
            INT32_T : 10,
            INT64_T : 11
            
        }
    
    }
    define:::(this) {
        @handle;
        @:callables = {};
        this.constructor = ::(path) {
            handle = _sharedlibrary_open(a:path);
            return handle;
        };
        
        this.interface = {
            bind ::(
                name => String, 
                argTypes => Object, 
                returnType => Number
            ) {
                // should throw error if invalid
                @symbol = _sharedlibrary_getCallable(a:handle, b:name);
                callables[name] = {
                    symbol : symbol,
                    args : argTypes,
                    returnType : returnType
                };
            },
            
            
            call ::(
                args => Object
            ) {
                
            }
        };
    }
});

/*

@:lib = SharedLibrary.new(path:'libtest.so');
@:lib_print = lib.bind(
    symbol:'print', 
    argTypes: [SharedLibrary.TYPES.CSTR, SharedLibrary.TYPES.CSTR], 
    returnType:SharedLibrary.TYPES.VOID
);

@:name = 'Audrey'
lib_print.call(args:['Hello, %s', name]);
*/


return SharedLibrary;
