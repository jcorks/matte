/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the Matte project (https://github.com/jcorks/matte)
matte was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/
@:class = import(module:'Matte.Core.Class');

@:_sharedlibrary_open = getExternalFunction(:"_sharedlibrary_open");
@:_sharedlibrary_getCallable = getExternalFunction(:"_sharedlibrary_getSymbol");
@:_sharedlibrary_callCallable = getExternalFunction(:"_sharedlibrary_getSymbol");


@TYPES = {
  VOID : 0
  INT : 1,
  DOUBLE : 2,
  FLOAT : 3,
  CHAR : 4,
  POINTER : 5,
  
  INT8_T : 6,
  INT16_T : 7
  INT32_T : 8,
  INT64_T : 9

  UINT8_T : 10,
  UINT16_T : 11
  UINT32_T : 12,
  UINT64_T : 13,
  
  STRUCT : 16,
    
}


// For now assume 64bit system, standard attribs
// NOTE: implicit fallback is that any of these can accept a memory buffer
// as input, which will be checked and copied first
@:PACK_INFO = [
  //VOID
  {
    size:0, 
    encode::(value, mem) <- error(:'Cannot convert to void!'),
    decode::(value) <- error(:'Cannot convert from void!'),
  },

  // INT
  {
    size:4, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeI32(offset:0, value),
        default: error(:'Cannot convert to INT!')
      }
    ,
    
    decode::(value) <-
      value.readI32(offset:0)
  },
  // Double
  {
    size:8, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeFloat64(offset:0, value),
        default: error(:'Cannot convert to DOUBLE!')
      }
    ,
    
    decode::(value) <-
      value.readFloat64(offset:0)
  },
  // float
  {
    size:4, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeFloat32(offset:0, value),
        default: error(:'Cannot convert to FLOAT!')
      }
    ,
    
    decode::(value) <-
      value.readFloat32(offset:0)
  },

  // char
  {
    size:1, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeI8(offset:0, value),
        default: error(:'Cannot convert to CHAR!')
      }
    ,
    
    decode::(value) <-
      value.readI8(offset:0)
  },

  // pointer,
  {
    size:8, 
    encode::(value, mem) <-
      match(value->type) {
        (MemoryBuffer.type):::{
          mem.size = value.size;
          mem.copy(0, value, 0, mem.size);
        }
      }
    ,
    
    decode::(value) {
      @:mem = MemoryBuffer.new();
      mem.size = value.size;
      mem.copy(0, value, 0, mem.size);
    }
  },


  // INT8_t
  {
    size:1, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeI8(offset:0, value),
        default: error(:'Cannot convert to INT8!')
      }
    ,
    
    decode::(value) <-
      value.readI8(offset:0)
  },
  //INT16_T 
  {
    size:2, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeI16(offset:0, value),
        default: error(:'Cannot convert to INT16!')
      }
    ,
    
    decode::(value) <-
      value.readI16(offset:0)
  },

  //INT32_T 
  {
    size:4, 
    encode::(value, mem) <-
      match(value->type) {
        (Number): mem.writeI32(offset:0, value),
        default: error(:'Cannot convert to INT32!')
      }
    ,
    
    decode::(value) <-
      value.readI16(offset:0)
  },



  INT64_T : 11

  UINT8_T : 12,
  UINT16_T : 13
  UINT32_T : 14,
  UINT64_T : 15,
  
  STRUCT : 16,


]



@SharedLibrary = class(
  name: 'Matte.System.MemoryBuffer',
  statics : {
      TYPES : {get::<-TYPES},
  },
  define:::(this) {
    @handle;
    @:callables = {}

    this.constructor = ::(path) {
      when(handle) empty;
      handle = _sharedlibrary_open(a:path);
    };



    @:packArg::(iter, value, type, mem) {
      
      mem.
    }

    
    this.interface = {

      // Returns a function that is a wrapper for the binded call.
      // 
      bind ::(
        name => String, 
        argTypes => Object, 
        returnType => Number
      ) {
          // should throw error if invalid
        @symbol = _sharedlibrary_getCallable(a:handle, b:name);
        @callInfo = {
          symbol : symbol,
          args : argTypes,
          returnType : returnType
        }
        
        // for packing args.
        @memIn = MemoryBuffer.new();
        
        // for packing output
        @memOut = MemoryBuffer.new();
        
        return ::(args) {
          
          _sharedlibrary_callCallable(
            
          );
        }
      }
    }
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
