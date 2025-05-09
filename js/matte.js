'use strict';

const Matte = {};
Matte.newVM = function(
    onImport, // function 
    onPrint, // function
) {
    var OBJECT_ID_POOL = 0;
    if (onImport == undefined) 
        throw new Error("onImport MUST be defined");


    const vm = {
        // the external function set.
        externalFunctions : {},
        
        // constains stackframe objects
        stackframe : [],
        
        // IDs of built-in external calls
        // matte_opcode.h: matteExtCall_t
        EXT_CALL : {
            NOOP : 0,
            IMPORT : 1,
            IMPORTMODULE : 2,
            PRINT : 3,
            SEND : 4,
            ERROR : 5,
            BREAKPOINT : 6,
            
            NUMBER_PI : 7,
            NUMBER_PARSE : 8,
            NUMBER_RANDOM : 9,
            
            STRING_COMBINE : 10,
            
            OBJECT_NEWTYPE : 11,
            OBJECT_INSTANTIATE : 12,
            OBJECT_FREEZEGC : 13,
            OBJECT_THAWGC : 14,
            OBJECT_GARBAGECOLLECT : 15,
            
            QUERY_ATAN2 : 16,
            
            QUERY_PUSH : 17,
            QUERY_INSERT: 18,
            QUERY_REMOVE : 19,
            QUERY_SETATTRIBUTES : 20,
            QUERY_SORT : 21,
            QUERY_SUBSET : 22,
            QUERY_FILTER : 23,
            QUERY_FINDINDEX : 24,
            QUERY_FINDINDEXCONDITION : 25,
            QUERY_ISA : 26,
            QUERY_MAP : 27,
            QUERY_REDUCE : 28,
            QUERY_ANY : 29,
            QUERY_ALL : 30,
            QUERY_FOREACH : 31,
            QUERY_SETSIZE : 32,
            
            QUERY_CHARAT : 33,
            QUERY_CHARCODEAT : 34,
            QUERY_SETCHARAT : 35,
            QUERY_SETCHARCODEAT : 36,
            QUERY_SCAN : 37,
            QUERY_SEARCH : 38,
            QUERY_SEARCH_ALL : 39,
            QUERY_FORMAT : 40,
            QUERY_SPLIT : 41,
            QUERY_SUBSTR : 42,
            QUERY_CONTAINS : 43,
            QUERY_COUNT : 44,
            QUERY_REPLACE : 45,
            QUERY_REMOVECHAR : 46,
            
            QUERY_SETISINTERFACE : 47,
            
            GETEXTERNALFUNCTION : 48
            
        },
        
        opcodes : {
            // no operation
            MATTE_OPCODE_NOP : 0,
            
            
            // push referrable onto value stack of current frame.
            MATTE_OPCODE_PRF : 1,
            // push empty 
            MATTE_OPCODE_NEM : 2,
            // push new number,
            MATTE_OPCODE_NNM : 3,
            // push new boolean,
            MATTE_OPCODE_NBL : 4,
            // push new string
            MATTE_OPCODE_NST : 5,
            // push new empty object
            MATTE_OPCODE_NOB : 6,
            // push new function from a compiled stub
            MATTE_OPCODE_NFN : 7,

            // costructor object assignment
            // with an object, key, and value,
            // assigns bypassing normal assignment 
            // checks. The object is left on the stack 
            // afterwards.
            MATTE_OPCODE_CAS : 8,
            
            // costructor array assignment
            // with an array and value,
            // assigns bypassing normal assignment 
            // checks. The object is left on the stack 
            // afterwards.
            MATTE_OPCODE_CAA : 9,    
            

            // push a new callstack from the given function object.
            MATTE_OPCODE_CAL : 10,
            // assign value to a referrable 
            MATTE_OPCODE_ARF : 11,
            // assign object member
            MATTE_OPCODE_OSN : 12,
            // lookup member
            MATTE_OPCODE_OLK : 13,

            // general purpose operator code.
            MATTE_OPCODE_OPR : 14,
            
            // call c / built-in function
            MATTE_OPCODE_EXT : 15,

            // pop values from the stack
            MATTE_OPCODE_POP : 16,
            // copys top value on the stack.
            MATTE_OPCODE_CPY : 17,

            // return;
            MATTE_OPCODE_RET : 18,
            
            // skips PC forward if conditional is false. used for when conditional return
            MATTE_OPCODE_SKP : 19 ,
            // skips PC forward always.
            MATTE_OPCODE_ASP : 20,
            // pushes a named referrable at runtime.
            // performance heavy; is not used except for 
            // special compilation.
            MATTE_OPCODE_PNR : 21,
            
            // Pushes the result of an listen expression.
            // listen pops 2 functions off the stack and pushes 
            // the result of the listen operation.
            MATTE_OPCODE_LST : 22,
            
            // pushes a abuilt-in type object 
            // 
            // 0 -> empty 
            // 1 -> boolean 
            // 2 -> number 
            // 3 -> string 
            // 4 -> object
            // 5 -> type (type of the type object) 
            // 6 -> any (not physically in code, just for the vm)
            MATTE_OPCODE_PTO : 23,
            
            
            // indicates to the VM that the next NFN is 
            // a strict function.
            MATTE_OPCODE_SFS : 24,

            // Indicates a query
            MATTE_OPCODE_QRY : 25,


            // Shhort circuit: &&
            // Peek the top. If false, skip by given count
            MATTE_OPCODE_SCA : 26,

            // Shhort circuit: OR
            // Peek the top. If true, skip by given count
            MATTE_OPCODE_SCO : 27,

            // spread operator: array.
            // pops the top of the stack and push all values of the object.
            MATTE_OPCODE_SPA : 28,

            // spread operator: object 
            // pops the top of the stack and pushes all keys and values of the object.
            MATTE_OPCODE_SPO : 29,
            
            // Object: Assign Set 
            // Allows for many assignments at once to an object using the 
            // dot operator. The syntax is operand.{static object};
            // The first operand is pushed to the top of the stack.
            MATTE_OPCODE_OAS  : 30,
            
            // performs a loop, taking the top 3 variables on the stack to define a 
            // loop: from value[top-3] to value[top-2], run function value[top-1]
            MATTE_OPCODE_LOP : 31,

            // performs an infinite loop, calling the value[top-1] function indefinitely.
            MATTE_OPCODE_FVR : 32,

            // performs a foreach loop on an object (value[top-1])
            MATTE_OPCODE_FCH : 33,
            
            // Performs a varargs call. Always expects one argument + the function.
            MATTE_OPCODE_CLV : 34,
            
            // Pushes the empty function.
            MATTE_OPCODE_NEF : 35,
            
            // Pushes the current private binding
            MATTE_OPCODE_PIP : 36
                     
        
        }              
    };


    // matte_bytecode_stub.c
    const bytecode = {
        // converts bytecode into stub objects.
        // bytecode should be an ArrayBuffer.
        stubsFromBytecode : function(fileID, bytecode) {
            // simple byte iterator matching what is done in C
            const bytes = {
                iter : 0,
                dataView : new DataView(bytecode),
                chompUInt8 : function() {
                    if (bytes.iter >= bytecode.byteLength) return 0;
                    const out = bytes.dataView.getUint8(bytes.iter);
                    bytes.iter += 1;
                    return out;
                },
                chompUInt16 : function() {
                    if (bytes.iter >= bytecode.byteLength) return 0;
                    const out = bytes.dataView.getUint16(bytes.iter, true);
                    bytes.iter += 2;
                    return out;
                },
                
                chompUInt32 : function() {
                    if (bytes.iter >= bytecode.byteLength) return 0;
                    const out = bytes.dataView.getUint32(bytes.iter, true);
                    bytes.iter += 4;
                    return out;
                },
                chompInt32 : function() {
                    if (bytes.iter >= bytecode.byteLength) return 0;
                    const out = bytes.dataView.getInt32(bytes.iter, true);
                    bytes.iter += 4;
                    return out;
                },
                chompDouble : function() {
                    if (bytes.iter >= bytecode.byteLength) return 0;
                    const out = bytes.dataView.getFloat64(bytes.iter, true);
                    bytes.iter += 8;
                    return out;
                },
                
                chompString : function() {
                    const size = bytes.chompUInt32();
                    var out = "";
                    var i = 0;
                    while (i < size) {
                        var val = 0;
                        const p0 = bytes.chompUInt8();
                        i++;
                        if (p0 < 128 && p0) {
                            val = (p0) & 0x7F;
                        } else {
                            const p1 = bytes.chompUInt8();
                            i++;
                            if (p0 < 224 && p0 && p1) {
                                val = ((p0 & 0x1F)<<6) + (p1 & 0x3F);
                            } else {
                                const p2 = bytes.chompUInt8();                                    
                                i++;
                                if (p0 < 240 && p0 && p1 && p2) {
                                    val = ((p0 & 0x0F)<<12) + ((p1 & 0x3F)<<6) + (p2 & 0x3F);
                                } else {
                                    const p3 = bytes.chompUInt8();
                                    i++;
                                    if (p0 && p1 && p2 && p3) {
                                        val = ((p0 & 0x7)<<18) + ((p1 & 0x3F)<<12) + ((p2 & 0x3F)<<6) + (p3 & 0x3F);
                                    } 
                                }
                            }
                        }
                        out += String.fromCodePoint(val);
                    } 
                    return out;


                },
                
                chompBytes : function(n) {
                    const out = bytecode.slice(iter, iter+n);
                    iter += n;
                    return out;
                }
            };
            
            
            const createStub = function() {
                if (bytes.chompUInt8() != ('M').charCodeAt(0) ||
                    bytes.chompUInt8() != ('A').charCodeAt(0) ||
                    bytes.chompUInt8() != ('T').charCodeAt(0) ||
                    bytes.chompUInt8() != 0x01 ||
                    bytes.chompUInt8() != 0x06 ||
                    bytes.chompUInt8() != ('B').charCodeAt(0)) {
                    throw new Error("Invalid header for stub.");   
                }

                // version
                if (bytes.chompUInt8() != 1)
                    throw new Error("Only version 1 of bytecode is supported.");
                    
                const stub = {
                    fileID : fileID
                };
                
                stub.stubID = bytes.chompUInt32();
                stub.isVarArg = bytes.chompUInt8();
                stub.argCount = bytes.chompUInt8();
                stub.argNames = [];
                stub.isDynamicBinding = false;
                for(var i = 0; i < stub.argCount; ++i) {
                    stub.argNames[i] = bytes.chompString();
                    if (stub.argNames[i] == '$')
                        stub.isDynamicBinding = true;
                }
                
                stub.localCount = bytes.chompUInt8();
                stub.localNames = [];
                for(var i = 0; i < stub.localCount; ++i) {
                    stub.localNames[i] = bytes.chompString();
                }

                stub.stringCount = bytes.chompUInt32();
                stub.strings = [];
                for(var i = 0; i < stub.stringCount; ++i) {
                    stub.strings[i] = bytes.chompString();
                }

                stub.capturedCount = bytes.chompUInt16();
                stub.captures = [];
                for(var i = 0; i < stub.capturedCount; ++i) {
                    stub.captures[i] = {
                        stubID : bytes.chompUInt32(),
                        referrable : bytes.chompUInt32()
                    };
                }
                
                stub.instructionCount = bytes.chompUInt32();
                stub.instructions = [];
                
                const baseLine = bytes.chompUInt32();
                for(var i = 0; i < stub.instructionCount; ++i) {
                    const offset = bytes.chompUInt16();
                    stub.instructions[i] = {
                        lineNumber : baseLine + offset,
                        opcode     : bytes.chompUInt8()
                    };
                    
                    if (stub.instructions[i].opcode == vm.opcodes.MATTE_OPCODE_NFN) {
                        stub.instructions[i].nfnFileID = fileID;
                        
                        // only the first 32 bits of data is the stubID.
                        // rewind and take only that
                        stub.instructions[i].data = bytes.chompUInt32();
                        bytes.chompUInt32(); // dump;
                    } else {
                        stub.instructions[i].data = bytes.chompDouble();                        
                    }
                }
                stub.endByte = bytes.iter;


                return stub;
            }
            
            
            const stubs = [];
            while(bytes.iter < bytecode.byteLength) {
                stubs.push(createStub());
            }
            
            return stubs;
        }
    };

    // matte_store.c
    
    const store = function(){
        const TYPE_EMPTY = 0;
        const TYPE_BOOLEAN = 1;
        const TYPE_NUMBER = 2;
        const TYPE_STRING = 3;
        const TYPE_OBJECT = 4;
        const TYPE_TYPE = 5;
        
        const valToType = function(val) {
            switch(typeof val) {
              case 'string': return TYPE_STRING;
              case 'boolean': return TYPE_BOOLEAN;
              case 'undefined' : return TYPE_EMPTY;
              case 'number' : return TYPE_NUMBER;
              default :
                return val.binID;                  
            }
        }
        
        const QUERY = {
            TYPE: 0,
            COS : 1,
            SIN : 2,
            TAN : 3,
            ACOS : 4,
            ASIN : 5,
            ATAN : 6,
            ATAN2 : 7,
            SQRT : 8,
            ABS : 9,
            ISNAN : 10,
            FLOOR : 11,
            CEIL : 12,
            ROUND : 13,
            RADIANS : 14,
            DEGREES : 15,
            
            REMOVECHAR : 16,
            SUBSTR : 17,
            SPLIT : 18,
            SCAN : 19,
            LENGTH : 20,
            SEARCH : 21,
            FORMAT : 22,
            SEARCH_ALL : 23,
            CONTAINS : 24,
            REPLACE : 25,
            COUNT : 26,
            CHARCODEAT : 27,
            CHARAT : 28,
            SETCHARCODEAT : 29,
            SETCHARAT : 30,
            
            KEYCOUNT : 31,
            SIZE : 32,
            SETSIZE : 33,
            KEYS : 34,
            VALUES : 35,
            PUSH : 36,
            POP : 37,
            INSERT : 38,
            REMOVE : 39,
            SETATTRIBUTES : 40,
            ATTRIBUTES : 41,
            SORT : 42,
            SUBSET : 43,
            FILTER : 44,
            FINDINDEX : 45,
            FINDINDEXCONDITION : 46,
            ISA : 47,
            MAP : 48,
            REDUCE : 49,
            ANY : 50,
            ALL : 51,
            FOREACH : 52,
            SETISINTERFACE : 53
        };
        
        var typecode_id_pool = 10;
        const TYPECODES = {
            OBJECT : 5
        };
    
        const createValue = function() {
            return undefined;
        };

        const createObject = function() {
            return {
                binID : TYPE_OBJECT,
                id : OBJECT_ID_POOL++,
                typecode : TYPECODES.OBJECT
            }
        };
        
        const objectPutProp = function(object, key, value) {
            const typ = valToType(key);
            const rec = object.hasLayout;
            if (rec) {
                if (typ != TYPE_STRING) {
                    vm.raiseErrorString("Objects with layouts can only have string keys.");
                    return;
                } 
                    
                const typeObj = store_typecode2data[object.typecode];
                const typeObj_ref = typeObj.layout[key];
                if (typeObj_ref == undefined) {
                    vm.raiseErrorString("Object has no such key '" + key+ "' in its layout.");
                    return;
                } 
                
                if (store.valueIsA(value, typeObj_ref) == false) {
                    vm.raiseErrorString(
                        "Object member '" + key + 
                        "' cannot be set from a " + 
                            store.valueTypeName(store.valueGetType(value)) + 
                        ". Any setting must be of type " + 
                            store.valueTypeName(typeObj_ref) + '.'
                    );
                    return;
                }
            }
            switch(typ) {
              case TYPE_STRING:
                if (object.kv_string == undefined)
                    object.kv_string = Object.create(null, {});
                object.kv_string[key] = value;
                break;
                
              case TYPE_NUMBER:
                if (object.kv_number == undefined)
                    object.kv_number = [];
                    
                if (key > object.kv_number.length) {
                    const index = Math.floor(key);
                    
                    for(var i = object.kv_number.length; i < index+1; ++i) {
                        object.kv_number[i] = store.empty;                        
                    }
                }
                
                object.kv_number[Math.floor(key)] = value;
                break;
              case TYPE_BOOLEAN:
                if (key == true) {
                    object.kv_true = value;
                } else {
                    object.kv_false = value;                    
                } 
                break;
              case TYPE_OBJECT:
                if (object.kv_object_values == undefined) {
                    object.kv_object_keys   = [];
                    object.kv_object_values = {};
                }                    
                object.kv_object_values[key.id.toString()] = value;
                object.kv_object_keys.push(key);
                break;
                
              case TYPE_TYPE:
                if (object.kv_types_values == undefined) {
                    object.kv_types_keys   = [];
                    object.kv_types_values = {};
                }
                object.kv_types_values[key.id.toString()] = value;
                object.kv_types_keys.push(key);
                break;
            }
        }
        
        

        const objectGetConvOperator = function(m, type) {
            if (m.table_attribSet == undefined)
                return createValue();
            return store.valueObjectAccess(m.table_attribSet, type, 0);
        };

        const objectGetAccessOperator = function(object, isBracket, read) {
            var out = createValue();
            if (object.table_attribSet == undefined)
                return out;
            
            const set = store.valueObjectAccess(object.table_attribSet, isBracket ? store_specialString_bracketAccess : store_specialString_dotAccess, 0);
            if (set) {
                if (valToType(set) != TYPE_OBJECT) {
                    vm.raiseErrorString("operator['[]'] and operator['.'] property must be an Object if it is set.");
                } else {
                    out = store.valueObjectAccess(set, read ? store_specialString_get : store_specialString_set, 0);
                }
            }
            return out;
        };
        
        const isaAdd = function(arr, v) {
            arr.push(v);
            if (v.isa) {
                const count = v.isa.length
                for(var i = 0; i < count; ++i) {
                    arr.push(v.isa[i]);
                }
            }
        };
        
        const typeArrayToIsA = function(val) {
            const count = store.valueObjectGetNumberKeyCount(val); // should be non-number?
            const array = [];
            if (count == 0) {
                vm.raiseErrorString("'inherits' attribute cannot be empty.");
                return array;
            }
            for(var i = 0; i < count; ++i) {
                const v = store.valueObjectArrayAtUnsafe(val, i);
                if (valToType(v) != TYPE_TYPE) {
                    vm.raiseErrorString("'inherits' attribute must have Type values only.");
                    return array;
                }
                isaAdd(array, v);
            }
            return array;
        };
        
        
        const typeArrayGetLayout = function(val, layout) {
            var out;
            
            if (layout) {
                out = {};
                if (valToType(layout) != TYPE_OBJECT) {
                    vm.raiseErrorString("'layout' attribute must be an Object when present");
                    return;
                }
                
                if (layout.kv_string) {
                    const keys = Object.keys(layout.kv_string);
                    for(var i = 0; i < keys.length; ++i) {
                        const typ = layout.kv_string[keys[i]];
                        if (valToType(typ) != TYPE_TYPE) {
                            vm.raiseErrorString("'layout' attribute \""+ keys[i]+"\" is not a Type.")
                            return;
                        }
                        
                        out[keys[i]] = typ;
                    }
                }
            }
            
            if (valToType(val) == TYPE_OBJECT) {
                const count = store.valueObjectGetNumberKeyCount(val); // should be non-number?
                for(var i = 0; i < count; ++i) {
                    const v = store.valueObjectArrayAtUnsafe(val, i);
                    if (valToType(v) == TYPE_TYPE) {
                        const type = store_typecode2data[v.id];
                                                    
                        if (type.layout) {
                            if (!out)
                                out = {};
                            const keys = Object.keys(type.layout);
                            for(var i = 0; i < keys.length; ++i) {
                                if (out[keys[i]] != undefined && out[keys[i]] != type.layout[keys[i]]) {
                                    vm.raiseErrorString("'layout' attribute \"" + keys[i] + "\" has multiple entries with differing types.");
                                    return;
                                }
                                out[keys[i]] = type.layout[keys[i]];
                            }
                        }
                    }
                }
            }
            return out;
        };


        // either returns lookup value or undefined if no such key
        const objectLookup = function(object, key) {
            const typ = valToType(key); 
            if (object.hasLayout) {
                if (typ != TYPE_STRING) {
                    vm.raiseErrorString("Objects with layouts can only have string keys.");
                    return undefined;                                    
                }
                
                if (store_typecode2data[object.typecode].layout[key] == undefined) {
                    vm.raiseErrorString("Record has no such key '" + key + "'");
                }
                if (object.kv_string == undefined) 
                    return undefined;
                return object.kv_string[key];
            }
        
            switch(typ) {
              case TYPE_EMPTY: return undefined;

              case TYPE_STRING:
                if (object.kv_string == undefined) 
                    return undefined;
                return object.kv_string[key];

              case TYPE_NUMBER:
                if (key < 0) return undefined;
                if (object.kv_number == undefined)
                    return undefined;
                if (key >= object.kv_number.length)
                    return undefined;
                return object.kv_number[Math.floor(key)];
               
              case TYPE_BOOLEAN:
                if (key == true) {
                    return object.kv_true;
                } else {
                    return object.kv_true;                    
                }
              case TYPE_OBJECT:
                if (object.kv_object_values == undefined)
                    return undefined;
                return object.kv_object_values[key.id.toString()];
              case TYPE_TYPE:
                if (!object.kv_types_values)
                    return undefined;
                return object.kv_types_values[key.id.toString()];
              
            }
        };

        const store_lock = {};
        var store_typepool = 0;
        const store_typecode2data = [[]];
        const store_type_number_methods = {
            PI : vm.EXT_CALL.NUMBER_PI,
            parse : vm.EXT_CALL.NUMBER_PARSE,
            random : vm.EXT_CALL.NUMBER_RANDOM
        };
        const store_type_object_methods = {
            newType : vm.EXT_CALL.OBJECT_NEWTYPE,
            instantiate : vm.EXT_CALL.OBJECT_INSTANTIATE,
            freezeGC : vm.EXT_CALL.OBJECT_FREEZEGC,
            thawGC : vm.EXT_CALL.OBJECT_THAWGC,
            garbageCollect : vm.EXT_CALL.OBJECT_GARBAGECOLLECT
        };
        const store_type_string_methods = {
            combine : vm.EXT_CALL.STRING_COMBINE
        };
        const store_queryTable = {};
        const store = {
            TYPE_BOOLEAN : TYPE_BOOLEAN,
            TYPE_EMPTY : TYPE_EMPTY,
            TYPE_NUMBER : TYPE_NUMBER,
            TYPE_OBJECT : TYPE_OBJECT,
            TYPE_TYPE : TYPE_TYPE,
            TYPE_STRING : TYPE_STRING,
            valToType : valToType,
            empty : (function() {
                return createValue();
            })(),
            
            createEmpty : function() {return store.empty;},
            
            createNumber : function(val) {
                return val;
            },
            
            createBoolean : function(val) {
                return !val ? false : true;
            },
            
            createString : function(val) {
                return val;
            },
            
            createObject : function() {
                return createObject();
            },
            
            createObjectTyped : function(type) {
                const out = createObject();
                if (valToType(type) != TYPE_TYPE) {
                    vm.raiseErrorString("Cannot instantiate object without a Type. (given value is of type " + store.valueTypeName(store.valueGetType(type)) + ')');
                    return out;
                }
                out.typecode = type.id;
                if (type.layout) {
                    out.hasLayout = true;
                }
                return out;
            },
            
            valueObjectGetIsInterfaceUnsafe : function(value) {
                return value.hasInterface;
            },
            
            valueObjectGetInterfacePrivateBindingUnsafe : function(value) {
                return value.table_privateBinding;
            },
            
            valueObjectSetIsInterface : function(value, enabled, privateBinding) {
                if (valToType(value) != TYPE_OBJECT) {
                    vm.raiseErrorString('setIsInterface query requires an Object.');
                    return;
                }
                value.hasInterface = enabled;
                value.table_privateBinding = undefined;
                if (privateBinding != undefined) {
                    if (valToType(privateBinding) != TYPE_OBJECT) {
                        vm.raiseErrorString("Cannot use a private interface binding for an interface that isn't an Object.")
                        return
                    }
                    value.table_privateBinding = privateBinding;
                }
            },

            valueObjectSetIsRecord : function(value, enabled) {
                if (valToType(value) != TYPE_OBJECT) {
                    vm.raiseErrorString('setIsRecord query requires an Object.');
                    return;
                }
                value.hasLayout = enabled;
            },
            
            createType : function(name, inherits, layout) {
                const out = {};
                out.binID = TYPE_TYPE;
                if (store_typepool == 0xffffffff) {
                    vm.raiseErrorString('Type count limit reached. No more types can be created.');
                    return createValue();
                } else {
                    out.id = ++store_typepool
                }
                
                if (name && valToType(name)) {
                    out.name = store.valueAsString(name);
                } else {
                    out.name = store.createString("<anonymous type " + out.id + ">");
                }

                if (inherits && valToType(inherits)) {
                    if (valToType(inherits) != TYPE_OBJECT) {
                        vm.raiseErrorString("'inherits' attribute must be an object.");
                        return createValue();
                    }                    
                    out.isa = typeArrayToIsA(inherits);
                }
                
                out.layout = typeArrayGetLayout(inherits, layout);
                
                store_typecode2data.push(out);
                return out;
            },
            
            
            createObjectLiteral : function(valueArray) {
                const out = createObject();
                for(var i = 0; i < valueArray.length; i += 2) {
                    objectPutProp(out, valueArray[i], valueArray[i+1]);
                }
                return out;
            },
            
            createObjectArray : function(values) {
                const out = createObject();
                out.kv_number = values.slice(0);
                return out;
            },
            
            emptyFunction : (function() {
                const out = createObject();
                out.function_stub = {
                    stubID : 0,
                    isVarArg : 0,
                    argCount : 0,
                    argNames : [],
                    isDynamicBinding : true,
                    localCount : 0,
                    localNames : [],
                    stringCount : 0,
                    strings : [],
                    capturedCount : 0,
                    captures : [],
                    instructionCount : 0,
                    instructions : []
                };
                out.function_captures = [];
                return out;
            })(),
            
            createFunction : function(stub) {
                const out = createObject();
                out.function_stub = stub;
                out.function_captures = [];


                const captures = stub.captures;
                var frame;
                // always preserve origin
                if (vm.getStackSize() > 0) {
                    frame = vm.getStackframe(0);
                    out.function_origin = frame.context;
                    out.function_origin_referrable = frame.referrable;
                }
                
                if (captures.length == 0) return out;


                                        
                for(var i = 0; i < captures.length; ++i) {
                    var context = frame.context;
                    var contextReferrable = frame.referrable;
                    
                    while(context.binID != 0) {
                        const origin = context;
                        if (origin.function_stub.stubID == captures[i].stubID) {
                            const ref = {};
                            ref.referrableSrc = contextReferrable;
                            ref.index = captures[i].referrable;
                            
                            out.function_captures.push(ref);
                            break;
                        }
                        context = origin.function_origin;
                        contextReferrable = origin.function_origin_referrable;
                    }
                    
                    if (context.binID == 0) {
                        vm.raiseErrorString("Could not find captured variable!");
                        out.function_captures.push({referrableSrc:createValue(), index:0});
                    }
                }
                return out;
            },
            
            createExternalFunction : function(stub) {
                return store.createFunction(stub);
            },
            
            getDynamicBindToken : function() {return store_specialString_dynamicBindToken;},
            
            createTypedFunction : function(stub, args) {
                for(var i = 0; i < args.length; ++i) {
                    if (valToType(args[i]) != TYPE_TYPE) {
                        if (i == len-1) {
                            vm.raiseErrorString("Function constructor with type specifiers requires those specifications to be Types. The return value specifier is not a Type.");
                        } else {
                            vm.raiseErrorString("Function constructor with type specifiers requires those specifications to be Types. The type specifier for Argument '"+stub.argNames[i]+"' is not a Type.\n");
                        }
                        return createValue();
                    }
                }
                const func = store.createFunction(stub);
                func.function_types = args;
                return func;
            },
            
            valueObjectPushLock : function(value) {
                if (store_lock[value] != undefined) {
                    store_lock[value]++;
                } else {
                    store_lock[value] = 1;                    
                }
            },
            
            valueObjectPopLock : function(value) {
                if (store_lock[value] == undefined)
                    return;
                store_lock[value]--;
                if (store_lock[value] <= 0)
                    delete store_lock[value];
            },
            
            valueObjectArrayAtUnsafe : function(value, index) {
                return value.kv_number[index];
            },
            
            valueStringGetStringUnsafe : function(value, index) {
                return value;
            },
            
            valueFrameGetNamedReferrable : function(frame, value) {
                throw new Error('TODO'); // or not. This VM does not do debugging.
            },
            
            valueSetCapturedValue : function(value, index, capt) {
                if (index >= value.function_captures.length) return;
                const ref = value.function_captures[index];
                store.valueObjectSet(
                    ref.referrableSrc, 
                    store.createNumber(ref.index),
                    capt 
                );
            },
            
            valueGetBytecodeStub : function(value) {
                return value.function_stub;
            },
            
            valueGetCapturedValue : function(value, index) {
                //if (value.binID != TYPE_OBJECT || value.function_stub == undefined) return;
                //if (index >= value.function_captures.length) return;
                return store.valueObjectArrayAtUnsafe(
                    value.function_captures[index].referrableSrc,
                    value.function_captures[index].index
                );
            },
            
            valueSubset : function(value, from, to) {
                const typ = valToType(value); 
                if (typ == TYPE_OBJECT) {
                    const curlen = value.kv_number ? value.kv_number.length : 0;
                    if (from >= curlen || to >= curlen) return createValue();
                    
                    return store.createObjectArray(value.kv_number.slice(from, to+1));
                } else if (typ == TYPE_STRING) {
                    
                    const curlen = value.length;;
                    if (from >= curlen || to >= curlen) return createValue();
                    return store.createString(
                        value.substr(from, to+1)
                    );
                }
            },
            
            // valueObjectSetUserData
            // valueObjectGetUserData
            // valueObjectSetNativeFinalizer are not implemented.
            
            valueIsEmpty : function(value) {
                return value == undefined;
            },
            
            valueAsNumber : function(value) {
                switch(valToType(value)) {
                  case TYPE_EMPTY:
                    vm.raiseErrorString('Cannot convert empty into a number');
                    return 0;

                  case TYPE_TYPE:
                    vm.raiseErrorString('Cannot convert type value into a number');
                    return 0;

                  case TYPE_NUMBER:
                    return value;
                    
                  case TYPE_BOOLEAN:
                    return value == true ? 1 : 0;
                    
                  case TYPE_STRING:
                    vm.raiseErrorString('Cannot convert string value into a number');
                    return 0;
                  case TYPE_OBJECT:
                    if (value.function_stub) {
                        vm.raiseErrorString("Cannot convert function value into a number.");
                        return 0;
                    }

                    const operator = objectGetConvOperator(value, store_type_number);
                    if (operator != undefined) {
                        return store.valueAsNumber(vm.callFunction(operator, [], []));
                    } else {
                        vm.raiseErrorString("Object has no valid conversion to number");
                    }
                    
                    
                }
            },
            
            valueAsString : function(value) {
                switch(valToType(value)) {
                  case TYPE_NUMBER:
                    return store.createString(String(value));

                  case TYPE_EMPTY:
                    return store_specialString_empty;

                  case TYPE_TYPE:
                    return store.valueTypeName(value);

                  case TYPE_BOOLEAN:
                    return value == true ? store_specialString_true : store_specialString_false;
                    
                  case TYPE_STRING:
                    return value;
                    
                  case TYPE_OBJECT:
                    if (value.function_stub != undefined) {
                        vm.raiseErrorString("Cannot convert function into a string.");
                        return store_specialString_empty;
                    }

                    const operator = objectGetConvOperator(value, store_type_string);
                    if (operator != undefined) {
                        return store.valueAsString(vm.callFunction(operator, [], []));
                    } else {
                        vm.raiseErrorString("Object has no valid conversion to string");
                    }
                        
                }
                // shouldnt get here.
                return store_specialString_empty;
            },
            
            
            valueAsBoolean : function(value) {
                switch(valToType(value)) {
                  case TYPE_EMPTY:   return false;
                  case TYPE_TYPE:    return true;
                  case TYPE_NUMBER:  return value != 0;
                  case TYPE_BOOLEAN: return value;
                  case TYPE_STRING:  return true;
                  case TYPE_OBJECT:
                    if (value.function_stub != undefined)
                        return 1;

                    const operator = objectGetConvOperator(value, store_type_boolean);                            
                    if (operator != undefined) {
                        const r = vm.callFunction(operator, [], []);
                        return store.valueAsBoolean(r);
                    } else {
                        return 1;
                    }
                    
                  break;
                }
                return false;
            },
            
            valueToType : function(v, t) {
                const typ = valToType(v);
                switch(t.id) {
                  case 1: // empty
                    if (typ != TYPE_EMPTY) {
                        vm.raiseErrorString("It is an error to convert any non-empty value to the Empty type.");
                    } else {
                        return v;
                    }
                    break;
                  case 7: // type
                    if (typ != TYPE_TYPE) {
                        vm.raiseErrorString("It is an error to convert any non-Type value to a Type.");
                    } else {
                        return v;
                    }
                    break;

                  case 2:
                    return store.createBoolean(store.valueAsBoolean(v));

                  case 3:
                    return store.createNumber(store.valueAsNumber(v));

                  case 4:
                    return store.valueAsString(v);
                    
                  case 5: 
                    if (typ == TYPE_OBJECT && !v.function_stub) {
                        return v;
                    } else {
                        vm.raiseErrorString("Cannot convert value to Object type.");
                    }
                    break;
                  case 6: 
                    if (typ == TYPE_OBJECT && v.function_stub) {
                        return v;
                    } else {
                        vm.raiseErrorString("Cannot convert value to Function type.");
                    }
                    break;
                  
                }
                return store.empty;
            },
            
            valueIsCallable : function(value) {
                const typ = valToType(value);
                if (typ == TYPE_TYPE) return 1;
                if (typ != TYPE_OBJECT) return 0;
                if (value.function_stub == undefined) return 0;
                if (value.function_types == undefined) return 1;
                return 2; // type-strict
            },
            
            valueObjectFunctionPreTypeCheckUnsafe : function(func, args) {
                const len = args.length;
                for(var i = 0; i < len; ++i) {
                    if (!store.valueIsA(args[i], func.function_types[i])) {
                        vm.raiseErrorString(
                            "Argument '" + func.function_stub.argNames[i] + 
                            "' (type: " + store.valueTypeName(store.valueGetType(args[i])) + ") is not of required type " + 
                            store.valueTypeName(func.function_types[i])
                        );
                        return 0;
                    }
                }
                return 1;
            },
            
            valueObjectFunctionPostTypeCheckUnsafe : function(func, ret) {
                const end = func.function_types.length - 1;
                if (!store.valueIsA(ret, func.function_types[end])) {
                    matte.raiseErrorString(
                        "Return value" + 
                        " (type: " + store.valueTypeName(store.valueGetType(ret)) + ") is not of required type " + 
                        store.valueTypeName(func.function_types[end])
                    );
                    return 0;
                }
                return 1;
            },
            
            valueObjectAccess : function(value, key, isBracket) {
                switch(valToType(value)) {
                  case TYPE_TYPE:
                    return store.valueObjectAccessDirect(value, key, isBracket);

                  case TYPE_OBJECT:
                    if (value.function_stub != undefined) {
                        vm.raiseErrorString("Functions do not have members.");
                        return createValue();
                    }
                    var accessor;
                    if (value.table_attribSet != undefined)
                        accessor = objectGetAccessOperator(value, isBracket, 1);
                    
                    if (value.hasInterface == true && (!(isBracket && accessor && accessor.binID))) {
                        if (valToType(key) != TYPE_STRING) {
                            vm.raiseErrorString("Objects with interfaces only have string-keyed members.");
                            return createValue();
                        }
                        
                        if (value.kv_string == undefined) {
                            vm.raiseErrorString("Object's interface was empty.");
                            return createValue();                        
                        }
                        
                        
                        const v = value.kv_string[key];
                        if (!v) {
                            vm.raiseErrorString("Object's interface has no member \"" + key + "\".");
                            return createValue();
                        }
                        
                        if (valToType(v) != TYPE_OBJECT) {
                            vm.raiseErrorString("Object's interface member \"" + key + "\" is neither a Function nor an Object. Interface is malformed.");
                            return createValue();
                        }
                        
                        // direct function, return
                        if (v.function_stub) {
                            return v;
                        }
                        
                        const setget = v;
                        const getter = setget.kv_string[store_specialString_get];
                        if (getter == undefined) {
                            vm.raiseErrorString("Object's interface disallows reading of the member \"" + key +"\".");
                            return createValue();
                        }
                        return vm.callFunction(getter, [], [], value.table_privateBinding);
                        
                    }
                    
                    
                    
                    if (accessor != undefined) {
                        return vm.callFunction(accessor, [key], [store_specialString_key]);
                    } else {
                        var output = objectLookup(value, key);
                        if (output == undefined)
                            return createValue();
                        return output;
                    }
                  default:
                    vm.raiseErrorString("Cannot access member of a non-object type. (Given value is of type: " + store.valueTypeName(store.valueGetType(value)) + ")");
                    return createValue();
                }
            },
            
            // will return undefined if no such access
            valueObjectAccessDirect : function(value, key, isBracket) {
                switch(valToType(value)) {
                  case TYPE_TYPE:
                    if (isBracket) {
                        vm.raiseErrorString("Types can only yield access to built-in functions through the dot '.' accessor.");
                        return;
                    }
                    
                    if (valToType(key) != TYPE_STRING) {
                        vm.raiseErrorString("Types can only yield access to built-in functions through the string keys.");
                        return;
                    }
                    
                    var base;
                    switch(value.id) {
                      case 3: base = store_type_number_methods; break;
                      case 4: base = store_type_string_methods; break;
                      case 5: base = store_type_object_methods; break;
                    }
                    if (base == undefined) {
                        vm.raiseErrorString("The given type has no built-in functions.");
                        return;
                    }
                    const out = base[key];
                    if (out == undefined) {
                        vm.raiseErrorString("The given type has no built-in function by the name '" + key + "'");
                        return;
                    }
                    return vm.getBuiltinFunctionAsValue(out);
                    
                    
                  case TYPE_OBJECT:
                    if (value.function_stub != undefined) {
                        vm.raiseErrorString("Cannot access member of type Function (Functions do not have members).");
                        return undefined;
                    }
                    if (value.hasInterface) return undefined;
                    
                    const accessor = objectGetAccessOperator(value, isBracket, 1);
                    if (accessor != undefined) {
                        return undefined;
                    } else {
                        return objectLookup(value, key);
                    }
                  
                  default:
                    vm.raiseErrorString("Cannot access member of a non-object type. (Given value is of type: " + store.valueTypeName(store.valueGetType(value)) + ")");
                    return undefined;                      
                }
            },
            
            
            valueObjectAccessString: function(value, plainString) {
                return store.valueObjectAccess(
                    value,
                    store.createString(plainString),
                    0
                );
            },
            
            valueObjectAccessIndex : function(value, index) {
                return store.valueObjectAccess(
                    value,
                    store.createNumber(index),
                    1
                );                
            },
            
            valueObjectKeys : function(value) {
                if (valToType(value) != TYPE_OBJECT || value.function_stub) {
                    vm.raiseErrorString("Can only get keys from something that's an Object.");
                    return createValue();
                }
            
            
                if (value.table_attribSet != undefined) {
                    const set = store.valueObjectAccess(value.table_attribSet, store_specialString_keys, 0);
                    if (set != undefined)
                        return store.valueObjectValues(vm.callFunction(set, [], []));
                }
                const out = [];
                if (value.kv_string) {
                    const keys = Object.keys(value.kv_string);
                    const len = keys.length;
                    for(var i = 0; i < len; ++i) {
                        out.push(store.createString(keys[i]));
                    }
                }


                if (!value.hasInterface && !value.hasLayout) {
                    if (value.kv_number) {
                        const len = value.kv_number.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(store.createNumber(i));
                        }
                    }
                                    


                    if (value.kv_object_keys) {
                        const keys = Object.values(value.kv_object_keys);
                        const len = keys.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(keys[i]); // OK, object 
                        }
                    }
                    
                    if (value.kv_true) {
                        out.push(store.createBoolean(true));
                    }

                    if (value.kv_false) {
                        out.push(store.createBoolean(false));
                    }


                    if (value.kv_types_keys) {
                        const keys = Object.values(value.kv_types_keys);
                        const len = keys.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(keys[i]); // OK, type
                        }
                    }
                }
                return store.createObjectArray(out);                
            },
            
            valueObjectValues : function(value) {
                if (valToType(value) != TYPE_OBJECT || value.function_stub) {
                    vm.raiseErrorString("Can only get values from something that's an Object.");
                    return createValue();
                }

                if (value.table_attribSet != undefined) {
                    const set = store.valueObjectAccess(value.table_attribSet, store_specialString_values, 0);
                    if (set)
                        return store.valueObjectValues(vm.callFunction(set, [], []));
                }
                const out = [];

                                
                if (value.kv_string) {
                    const values = Object.values(value.kv_string);
                    const len = values.length;
                    
                    if (value.hasInterface) {
                        for(var i = 0; i < len; ++i) {
                            const v = values[i];
                            if (v.function_stub) {
                                out.push(v);
                            } else {
                                const setget = v;
                                const getter = setget.kv_string[store_specialString_get];
                                if (!getter) {
                                    continue;
                                }
                                out.push(vm.callFunction(getter, [], []));                                    
                            }
                        }                        
                    } else {                        
                        for(var i = 0; i < len; ++i) {
                            out.push(values[i]);
                        }
                    }
                }
                
                if (!value.hasInterface && !value.hasLayout) {
                    if (value.kv_number) {
                        const len = value.kv_number.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(value.kv_number[i]);
                        }
                    }
                    if (value.kv_object_values) {
                        const values = Object.values(value.kv_object_values);
                        const len = values.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(values[i]); // OK, object 
                        }
                    }
                    
                    if (value.kv_true) {
                        out.push(store.createBoolean(value.kv_true));
                    }

                    if (value.kv_false) {
                        out.push(store.createBoolean(value.kv_false));
                    }
                }

                if (value.kv_types_values) {
                    const values = Object.values(value.kv_types_values);
                    const len = values.length;
                    for(var i = 0; i < len; ++i) {
                        out.push(values[i]); // OK, type
                    }
                }
                return store.createObjectArray(out);                  
            },
            
            
            valueObjectGetKeyCount : function(value) {
                if (valToType(value) != TYPE_OBJECT) return 0;
                var out = 0;
                if (value.kv_number) {
                    out += value.kv_number.length;
                }
                                
                if (value.kv_string) {
                    const keys = Object.keys(value.kv_string);
                    out += keys.length;
                }

                if (value.kv_object_keys) {
                    const keys = Object.values(value.kv_object_keys);
                    out +=  keys.length;
                }
                
                if (value.kv_true) {
                    out += 1;
                }

                if (value.kv_false) {
                    out += 1;
                }


                if (value.kv_types_keys) {
                    const keys = Object.values(value.kv_types_keys);
                    out += keys.length;
                }
                return out;                     
            },
            
            valueObjectGetNumberKeyCount : function(value) {
                if (valToType(value) != TYPE_OBJECT) return 0;
                if (value.kv_number == undefined) return 0;
                return value.kv_number.length;                    
            },
            
            valueObjectInsert : function(value, plainIndex, v) {
                if (valToType(value) != TYPE_OBJECT) return;
                if (value.kv_number == undefined) value.kv_number = [];
                value.kv_number.splice(plainIndex, 0, v);
            },

            valueObjectPush : function(value, v) {
                if (valToType(value) != TYPE_OBJECT) return;
                if (value.kv_number == undefined) value.kv_number = [];
                value.kv_number.push(v);
            },

            
            valueObjectSortUnsafe : function(value, lessFnValue) {
                if (value.kv_number == undefined) return;
                const vals = value.kv_number;
                if (vals.length == 0) return;
                
                const names = [
                    store.createString('a'),
                    store.createString('b')
                ];
                
                vals.sort(function(a, b) {
                    return store.valueAsNumber(vm.callFunction(lessFnValue, [a, b], names));
                });
            },
            
            
            valueObjectForeach : function(value, func) {
                if (valToType(value) != TYPE_OBJECT) {
                    vm.raiseErrorString("Cannot foreach on something that isn't an object.");
                    return store.empty;
                }                    
                if (value.table_attribSet) {
                    const set = store.valueObjectAccess(value.table_attribSet, store_specialString_foreach, 0);
                    if (set != undefined) {
                        const v = vm.callFunction(set, [], []);
                        if (valToType(v) != TYPE_OBJECT) {
                            vm.raiseErrorString("foreach attribute MUST return an object.");
                            return;
                        } else {
                            value = v;
                        }
                    }
                }
                
                const names = [
                    store_specialString_key,
                    store_specialString_value
                ];
                const stub = func.function_stub; 
                if (stub && stub.argCount >= 2) {
                    names[0] = store.createString(stub.argNames[0]);
                    names[1] = store.createString(stub.argNames[1]);
                }
                
                const keys   = store.valueObjectKeys(value);
                const values = store.valueObjectValues(value);
                const len = store.valueObjectGetNumberKeyCount(keys);
                for(var i = 0; i < len; ++i) {
                    const k = store.valueObjectAccessIndex(keys, i);
                    const v = store.valueObjectAccessIndex(values, i);
                    vm.callFunction(func, [k, v], names);
                }
            },
            
            valueObjectSet : function(value, key, v, isBracket) {
                if (valToType(value) != TYPE_OBJECT) {
                    vm.raiseErrorString("Cannot set property on something that isn't an object.");
                    return store.empty;
                }
                
                if (key == undefined) {
                    vm.raiseErrorString("Cannot set property with an empty key");
                    return store.empty;
                }
                
                const assigner = objectGetAccessOperator(value, isBracket, 0);
                
                if (value.hasInterface == true && (!(isBracket && assigner))) {
                    if (valToType(key) != TYPE_STRING) {
                        vm.raiseErrorString("Objects with interfaces only have string-keyed members.");
                        return createValue();
                    }
                    
                    if (value.kv_string == undefined) {
                        vm.raiseErrorString("Object's interface was empty.");
                        return createValue();                        
                    }
                    
                    const vv = value.kv_string[key];
                    if (!vv) {
                        vm.raiseErrorString("Object's interface has no member \"" + key + "\".");
                        return createValue();
                    }

                    if (valToType(vv) != TYPE_OBJECT) {
                        vm.raiseErrorString("Object's interface member \"" + key + "\" is neither a Function nor an Object. Interface is malformed.");
                        return createValue();
                    }
                    
                    // direct function, return
                    if (vv.function_stub) {
                        vm.raiseErrorString("Object's \""+key+"\" is a member function and is read-only. Writing to this member is not allowed.");
                        return createValue();
                    }
                    
                    const setget = vv;
                    const setter = setget.kv_string[store_specialString_set];
                    if (!setter) {
                        vm.raiseErrorString("Object's interface disallows writing of the member \"" + key +"\".");
                        return createValue();
                    }
                    
                    vm.callFunction(setter, [v], [store_specialString_value], value.table_privateBinding);
                    return createValue();
                    
                }




                if (assigner) {
                    const r = vm.callFunction(assigner, [key, v], [store_specialString_key, store_specialString_value]);
                    if (valToType(r) == TYPE_BOOLEAN && !r) {
                        return store.empty;
                    }
                }
                return objectPutProp(value, key, v);
            },
            
            valueObjectSetTable: function(value, srcTable) {
                if (valToType(srcTable) != TYPE_OBJECT) {
                    vm.raiseErrorString("Cannot use object set assignment syntax something that isnt an object.");
                    return;
                }
                const keys = store.valueObjectKeys(srcTable);
                const len = store.valueObjectGetNumberKeyCount(keys);
                for(var i = 0; i < len; ++i) {
                    const key = store.valueObjectArrayAtUnsafe(keys, i);
                    const val = store.valueObjectAccess(srcTable, key, 1);
                    store.valueObjectSet(value, key, val, 0);
                }
            },
            
            valueObjectSetIndexUnsafe : function(value, plainIndex, v) {
                value.kv_number[plainIndex] = v;
            },
            
            valueObjectSetAttributes : function(value, opObject) {
                if (valToType(value) != TYPE_OBJECT) {
                    vm.raiseErrorString("Cannot assign attributes set to something that isnt an object.");
                    return;
                }
                
                if (valToType(opObject) != TYPE_OBJECT) {
                    vm.raiseErrorString("Cannot assign attributes set that isn't an object.");
                    return;
                }
                
                if (opObject === value) {
                    vm.raiseErrorString("Cannot assign attributes set as its own object.");
                    return;
                }
                
                value.table_attribSet = opObject;
            },
            
            valueObjectGetAttributesUnsafe : function(value) {
                return value.table_attribSet;
            },

            valueObjectRemoveKey : function(value, key) {
                if (valToType(value) != TYPE_OBJECT) {
                    return; // no error?
                }
                
                switch(valToType(key)) {
                  case TYPE_STRING:
                    if (value.kv_string == undefined) return;
                    delete value.kv_string[key];
                    break;
                  case TYPE_TYPE:
                    if (value.kv_types_keys == undefined) return;
                    delete value.kv_types_values[key.id];
                    for(var i = 0; i < value.kv_types_keys.length; ++i) {
                        if (value.kv_types_keys[i] === key) {
                            value.kv_types_keys.splice(i, 1);
                        }
                    }
                    break;
                  case TYPE_NUMBER:
                    if (value.kv_number == undefined || value.kv_number.length == 0) return;
                    value.kv_number.splice(Math.floor(key), 1);
                    break;
                  case TYPE_BOOLEAN:
                    if (key) {
                        value.kv_true = undefined;
                    } else {
                        value.kv_false = undefined;                        
                    }
                    break;
                  case TYPE_OBJECT:
                    if (value.kv_object_keys == undefined) return;
                    delete value.kv_object_values[key.id];
                    for(var i = 0; i < value.kv_object_keys.length; ++i) {
                        if (value.kv_object_keys[i] === key) {
                            value.kv_object_keys.splice(i, 1);
                        }
                    }
                    break;
                }
            },
            
            
            valueObjectRemoveKeyString : function(value, plainString) {
                if (valToType(value) != TYPE_OBJECT) {
                    return; // no error?
                }
                
                if (value.kv_string == undefined) return;
                delete value.kv_string[plainString];
            },
                            
            valueIsA : function(value, typeobj) {
                if (valToType(typeobj) != TYPE_TYPE) {
                    vm.raiseErrorString("VM error: cannot query isa() with a non Type value.");
                    return 0;
                }
                
                if (typeobj.id == store_type_any.id) return 1;
                if (valToType(value) != TYPE_OBJECT) {
                    if (typeobj.id == store_type_nullable.id && value == undefined)
                        return 1;
                    return (store.valueGetType(value)).id == typeobj.id;
                } else {
                    if (typeobj.id == store_type_nullable.id) return 1;
                    if (typeobj.id == store_type_object.id) return 1;
                    if (typeobj.id == store_type_function.id)
                        return value.function_stub != undefined;
                    const typep = store.valueGetType(value); 
                    if (typep.id == typeobj.id) return 1;
                    
                    if (typep.isa == undefined) return 0;
                    for(var i = 0; i < typep.isa.length; ++i) {
                        if (typep.isa[i].id == typeobj.id) return 1;
                    }
                    return 0;
                }
            },
            
            valueGetType : function(value) {
                switch(valToType(value)) {
                  case TYPE_EMPTY:      return store_type_empty;
                  case TYPE_BOOLEAN:    return store_type_boolean;
                  case TYPE_NUMBER:     return store_type_number;
                  case TYPE_STRING:     return store_type_string;
                  case TYPE_TYPE:       return store_type_type;
                  case TYPE_OBJECT: 
                    if (value.function_stub != undefined)
                        return store_type_function;
                    else {
                        return store_typecode2data[value.typecode];
                    } 
                }
            },
            
            
            valueQuery : function(value, query) {
                const out = store_queryTable[query](value);
                if (out == undefined) {
                    vm.raiseErrorString("VM error: unknown query operator");
                }
                return out;
            },
            
            valueTypeName : function(value) {
                if (valToType(value) != TYPE_TYPE) {
                    vm.raiseErrorString("VM error: cannot get type name of a non Type value.");
                    return store_specialString_empty;
                }
                switch(value.id) {
                  case 1: return store_specialString_type_empty;
                  case 2: return store_specialString_type_boolean;
                  case 3: return store_specialString_type_number;
                  case 4: return store_specialString_type_string;
                  case 5: return store_specialString_type_object;
                  case 6: return store_specialString_type_function;
                  case 7: return store_specialString_type_type;
                  case 8: return store_specialString_type_nullable;
                  default:
                    return value.name;
                }
            },
            
            getEmptyType    : function() {return store_type_empty;},
            getBooleanType  : function() {return store_type_boolean;},
            getNumberType   : function() {return store_type_number;},
            getStringType   : function() {return store_type_string;},
            getObjectType   : function() {return store_type_object;},
            getFunctionType : function() {return store_type_function;},
            getTypeType     : function() {return store_type_type;},
            getAnyType      : function() {return store_type_any;},
            getNullableType      : function() {return store_type_nullable;}
            
                            
        };
        
        store_queryTable[QUERY.TYPE] = function(value) {
            return store.valueGetType(value);
        };
        
                    
        store_queryTable[QUERY.COS] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("cos requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.cos(value));
        };
        
        store_queryTable[QUERY.SIN] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("sin requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.sin(value));
        };
        
        
        store_queryTable[QUERY.TAN] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("tan requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.tan(value));
        };
        

        store_queryTable[QUERY.ACOS] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("acos requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.acos(value));
        };
        
        store_queryTable[QUERY.ASIN] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("asin requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.asin(value));
        };
        
        
        store_queryTable[QUERY.ATAN] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("atan requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.atan(value));
        };
        store_queryTable[QUERY.ATAN2] = function(value) {
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ATAN2);
        };
        store_queryTable[QUERY.SQRT] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("sqrt requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.atan(value));
        };
        store_queryTable[QUERY.ABS] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("abs requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.abs(value));
        };
        store_queryTable[QUERY.ISNAN] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("isNaN requires base value to be a number.");
                return createValue();
            }
            return store.createBoolean(isNaN(value));
        };
        store_queryTable[QUERY.FLOOR] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("floor requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.floor(value));
        };
        store_queryTable[QUERY.CEIL] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("ceil requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.ceil(value));
        };
        store_queryTable[QUERY.ROUND] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("round requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(Math.round(value));
        };
        store_queryTable[QUERY.RADIANS] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("radian conversion requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(value * (Math.PI / 180.0));
        };
        store_queryTable[QUERY.DEGREES] = function(value) {
            if (valToType(value) != TYPE_NUMBER) {
                vm.raiseErrorString("degree conversion requires base value to be a number.");
                return createValue();
            }
            return store.createNumber(value * (180.0 / Math.PI));
        };







        store_queryTable[QUERY.REMOVECHAR] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("removeChar requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REMOVECHAR);
        };

        store_queryTable[QUERY.SUBSTR] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("substr requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SUBSTR);
        };
        
        store_queryTable[QUERY.SPLIT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("split requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SPLIT);
        };

        store_queryTable[QUERY.SCAN] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("scan requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SCAN);
        };

        store_queryTable[QUERY.LENGTH] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("length requires base value to be a string.");
                return createValue();
            }
            return store.createNumber(value.length);
        };
        
        store_queryTable[QUERY.SEARCH] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("search requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SEARCH);
        };

        store_queryTable[QUERY.SEARCH_ALL] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("searchAll requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SEARCH_ALL);
        };

        store_queryTable[QUERY.CONTAINS] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("contains requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_CONTAINS);
        };

        store_queryTable[QUERY.REPLACE] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("replace requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REPLACE);
        };

        store_queryTable[QUERY.FORMAT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("format requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FORMAT);
        };

        store_queryTable[QUERY.COUNT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("count requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_COUNT);
        };

        store_queryTable[QUERY.SETCHARCODEAT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("setCharCodeAt requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETCHARCODEAT);
        };

        store_queryTable[QUERY.SETCHARAT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("setCharAt requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETCHARAT);
        };
        store_queryTable[QUERY.CHARCODEAT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("charCodeAt requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_CHARCODEAT);
        };

        store_queryTable[QUERY.CHARAT] = function(value) {
            if (valToType(value) != TYPE_STRING) {
                vm.raiseErrorString("charAt requires base value to be a string.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_CHARAT);
        };





        store_queryTable[QUERY.KEYCOUNT] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("keycount requires base value to be an object.");
                return createValue();
            }
            return store.createNumber(store.valueObjectGetKeyCount(value));
        };

        store_queryTable[QUERY.SIZE] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("size requires base value to be an object.");
                return createValue();
            }
            return store.createNumber(store.valueObjectGetNumberKeyCount(value));
        };


        store_queryTable[QUERY.KEYS] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("keys requires base value to be an object.");
                return createValue();
            }
            return store.valueObjectKeys(value);
        };

        store_queryTable[QUERY.VALUES] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("values requires base value to be an object.");
                return createValue();
            }
            return store.valueObjectValues(value);
        };

        store_queryTable[QUERY.PUSH] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("push requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_PUSH);
        };

        store_queryTable[QUERY.SETSIZE] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("setSize requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETSIZE);
        };
        store_queryTable[QUERY.POP] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("pop requires base value to be an object.");
                return createValue();
            }
            
            const indexPlain = store.valueObjectGetNumberKeyCount(value) - 1;
            const key = store.createNumber(indexPlain);
            const out = store.valueObjectAccessDirect(value, key, 1);
            if (out != undefined) {
                store.valueObjectRemoveKey(value, key);
                return out;
            }
            return createValue();
        };
        
        store_queryTable[QUERY.INSERT] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("insert requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_INSERT);
        };
        
        store_queryTable[QUERY.REMOVE] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("remove requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REMOVE);
        };

        store_queryTable[QUERY.SETATTRIBUTES] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("setAttributes requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETATTRIBUTES);
        };

        store_queryTable[QUERY.ATTRIBUTES] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("setAttributes requires base value to be an object.");
                return createValue();
            }
            const out = store.valueObjectGetAttributesUnsafe(value);
            if (out == undefined) {
                return createValue();
            }
            return out;
        };

        store_queryTable[QUERY.SORT] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("sort requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SORT);
        };

        store_queryTable[QUERY.SUBSET] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("subset requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SUBSET);
        };

        store_queryTable[QUERY.FILTER] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("filter requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FILTER);
        };

        store_queryTable[QUERY.FINDINDEX] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("findIndex requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FINDINDEX);
        };

        store_queryTable[QUERY.FINDINDEXCONDITION] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("findIndexCondition requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FINDINDEXCONDITION);
        };

        store_queryTable[QUERY.ISA] = function(value) {
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ISA);
        };

        store_queryTable[QUERY.MAP] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("map requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_MAP);
        };

        store_queryTable[QUERY.ANY] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("any requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ANY);
        };


        store_queryTable[QUERY.FOREACH] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("foreach requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FOREACH);
        };
        
        store_queryTable[QUERY.SETISINTERFACE] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("setIsInterface requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETISINTERFACE);
        };            



        store_queryTable[QUERY.ALL] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("all requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ALL);
        };

        store_queryTable[QUERY.REDUCE] = function(value) {
            if (valToType(value) != TYPE_OBJECT) {
                vm.raiseErrorString("reduce requires base value to be an object.");
                return createValue();
            }
            return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REDUCE);
        };

        




        

        
        

        
        const store_type_empty = store.createType();
        const store_type_boolean = store.createType();
        const store_type_number = store.createType();
        const store_type_string = store.createType();
        const store_type_object = store.createType();
        const store_type_function = store.createType();
        const store_type_type = store.createType();
        const store_type_any = store.createType();
        const store_type_nullable = store.createType();
        
        
        const store_specialString_false = store.createString('false');
        const store_specialString_true = store.createString('true');
        const store_specialString_empty = store.createString('empty');
        const store_specialString_nothing = store.createString('');

        const store_specialString_type_boolean = store.createString('Boolean');
        const store_specialString_type_empty = store.createString('Empty');
        const store_specialString_type_number = store.createString('Number');
        const store_specialString_type_string = store.createString('String');
        const store_specialString_type_object = store.createString('Object');
        const store_specialString_type_type = store.createString('Type');
        const store_specialString_type_function = store.createString('Function');
        const store_specialString_type_nullable = store.createString('Nullable');

        const store_specialString_bracketAccess = store.createString('[]');
        const store_specialString_dotAccess = store.createString('.');
        const store_specialString_set = store.createString('set');
        const store_specialString_get = store.createString('get');
        const store_specialString_foreach = store.createString('foreach');
        const store_specialString_keys = store.createString('keys');
        const store_specialString_values = store.createString('values');
        const store_specialString_name = store.createString('name');
        const store_specialString_inherits = store.createString('inherits');
        const store_specialString_key = store.createString('key');
        const store_specialString_value = store.createString('value');
        const store_specialString_dynamicBindToken = store.createString('$');
        
        return store; 
    }();    


    
    (function(){
        
        const valToType = store.valToType;

        const vm_operator = {
            MATTE_OPERATOR_ADD : 0, // + 2 operands
            MATTE_OPERATOR_SUB : 1, // - 2 operands
            MATTE_OPERATOR_DIV : 2, // / 2 operands
            MATTE_OPERATOR_MULT : 3, // * 2 operands
            MATTE_OPERATOR_NOT : 4, // ! 1 operand
            MATTE_OPERATOR_BITWISE_OR : 5, // | 2 operands
            MATTE_OPERATOR_OR : 6, // || 2 operands
            MATTE_OPERATOR_BITWISE_AND : 7, // & 2 operands
            MATTE_OPERATOR_AND : 8, // && 2 operands
            MATTE_OPERATOR_SHIFT_LEFT : 9, // << 2 operands
            MATTE_OPERATOR_SHIFT_RIGHT : 10, // >> 2 operands
            MATTE_OPERATOR_POW : 11, // ** 2 operands
            MATTE_OPERATOR_EQ : 12, // == 2 operands
            MATTE_OPERATOR_BITWISE_NOT : 13, // ~ 1 operand
            MATTE_OPERATOR_POINT : 14, // -> 2 operands
            MATTE_OPERATOR_POUND : 15, // # 1 operand
            MATTE_OPERATOR_TERNARY : 16, // ? 2 operands
            MATTE_OPERATOR_GREATER : 17, // > 2 operands
            MATTE_OPERATOR_LESS : 18, // < 2 operands
            MATTE_OPERATOR_GREATEREQ : 19, // >= 2 operands
            MATTE_OPERATOR_LESSEQ : 20, // <= 2 operands
            MATTE_OPERATOR_TRANSFORM : 21, // <> 2 operands
            MATTE_OPERATOR_NOTEQ : 22, // != 2 operands
            MATTE_OPERATOR_MODULO : 23, // % 2 operands
            MATTE_OPERATOR_CARET : 24, // ^ 2 operands
            MATTE_OPERATOR_NEGATE : 25, // - 1 operand
            MATTE_OPERATOR_TYPESPEC : 26, // => 2 operands

            // special operators. They arent part of the OPR opcode
            MATTE_OPERATOR_ASSIGNMENT_NONE : 100,
            MATTE_OPERATOR_ASSIGNMENT_ADD : 101,
            MATTE_OPERATOR_ASSIGNMENT_SUB : 102,
            MATTE_OPERATOR_ASSIGNMENT_MULT : 103,
            MATTE_OPERATOR_ASSIGNMENT_DIV : 104,
            MATTE_OPERATOR_ASSIGNMENT_MOD : 105,
            MATTE_OPERATOR_ASSIGNMENT_POW : 106,
            MATTE_OPERATOR_ASSIGNMENT_AND : 107,
            MATTE_OPERATOR_ASSIGNMENT_OR : 108,
            MATTE_OPERATOR_ASSIGNMENT_XOR : 109,
            MATTE_OPERATOR_ASSIGNMENT_BLEFT : 110,
            MATTE_OPERATOR_ASSIGNMENT_BRIGHT : 111,
            
            
            // special operator state flag for OSN instructions.
            // TODO: better method maybe? 
            MATTE_OPERATOR_STATE_BRACKET : 200,
        
        };
        const vm_operatorFunc = [];
        
        
        const vm_imports = [];
        const vm_stubIndex = {};
        const vm_precomputed = {}; // matte_vm.c: imported
        const vm_specialString_parameters = store.createString('parameters');
        const vm_specialString_onsend = store.createString('onSend');
        const vm_specialString_onerror = store.createString('onError');
        const vm_specialString_message = store.createString('message');
        const vm_specialString_value = store.createString('value');
        const vm_specialString_base = store.createString('base');
        const vm_specialString_previous = store.createString('previous');
        const vm_specialString_from = store.createString('from');
        const vm_externalFunctionIndex = []; // all external functions
        const vm_externalFunctions = {}; // name -> id in index
        const vm_extStubs = [];
        const vm_extFuncs = [];
        const vm_opcodeSwitch = [];
        const vm_id2importPath = {};
        const vm_importPath2ID = {};
        const vm_CONTROL_CODE_VALUE_TYPE__DYNAMIC_BINDING = 0xff;

        const vm_callstack = [];
        const vm_opcodesSwitch = [];
        vm.stackframe = vm_callstack;

        var vm_pendingCatchable = false;
        var vm_pendingCatchableIsError = false;
        var vm_pendingRestartCondition = false;
        var vm_pendingRestartConditionData = false;
        var vm_stacksize = 0;
        var vm_fileIDPool = 10;
        var vm_catchable;
        var vm_errorLastFile;
        var vm_errorLastLine;
        
        
        const vm_getScriptNameById = function(fileID) {
            return vm_id2importPath[fileID];
        };
        
        const vm_getNewFileID = function(name) {
            const id = vm_fileIDPool++;
            vm_importPath2ID[name] = id;
            vm_id2importPath[id] = name;
            return id;
        };
        
        const vm_infoNewObject = function(detail) {
            const out = store.createObject();
            const callstack = store.createObject();
            
            var key = store.createString("length");
            var val = store.createNumber(vm_stacksize);
            
            store.valueObjectSet(callstack, key, val, 0);
            
            const arr = [];
            var str;
            if (valToType(detail) == store.TYPE_STRING) {
                str = detail + '\n';
            } else {
                str = "<no string data available>";
            }
            
            for(var i = 0; i < vm_stacksize; ++i) {
                const framesrc = vm.getStackframe(i);
                const frame = store.createObject();
                const filename = vm_getScriptNameById(framesrc.stub.fileID);
                
                key = store.createString('file');
                val = store.createString(filename ? filename : '<unknown>');
                store.valueObjectSet(frame, key, val, 0);
                
                const instcount = framesrc.stub.instructionCount;
                key = store.createString("lineNumber");
                var lineNumber = 0;
                if (framesrc.pc -1 < instcount) {
                    lineNumber = framesrc.stub.instructions[framesrc.pc-1].lineNumber;
                }
                val = store.createNumber(lineNumber);
                store.valueObjectSet(frame, key, val);
                arr.push(frame);
                str += 
                    " (" + i + ") -> " + (filename ? filename : "<unknown>") + ", line " + lineNumber + "\n"
                ;
            }
            
            val = store.createObjectArray(arr);
            key = store.createString("frames");
            
            store.valueObjectSet(callstack, key, val, 0);
            key = store.createString("callstack");
            store.valueObjectSet(out, key, callstack, 0);
            
            key = store.createString("summary");
            val = store.createString(str);
            store.valueObjectSet(out, key, val);
            
            key = store.createString("detail");
            store.valueObjectSet(out, key, detail);
            
            return out;
        };
        



        
        const vm_addBuiltIn = function(index, argNames, fn) {
            if (vm_externalFunctionIndex.length <= index)
                vm_externalFunctionIndex.length = index+1;
                
            if (vm_extStubs.length <= index) {
                vm_extStubs.length = index+1;
                vm_extFuncs.length = index+1;
            }
            
            
            
            const set = {
                userFunction : fn,
                userData : null,
                nArgs : argNames.length
            };
            vm_externalFunctionIndex[index] = set;


            var charLen = 0;
            for(var i = 0; i < argNames.length; ++i) {
                const str = argNames[i];
                var nBytes = 0;
                // get utf8 length;
                for(var n = 0; n < str.length; ++n) {
                    const val = str.codePointAt(n);
                    if (val < 0x80) {
                        charLen += 1;
                    } else if (val < 0x800) {
                        charLen += 2;
                    } else if (val < 0x10000) {
                        charLen += 3;    
                    } else {
                        charLen += 4;
                    }                    
                }
            };
            
            const buffer = new Uint8Array(7 + 4 + 2 + charLen + argNames.length*4);
            
            buffer[0] = 'M'.charCodeAt(0);
            buffer[1] = 'A'.charCodeAt(0);
            buffer[2] = 'T'.charCodeAt(0);
            buffer[3] = 0x01;
            buffer[4] = 0x06;
            buffer[5] = 'B'.charCodeAt(0);
            buffer[6] = 0x1;
            
            
            const bufferView = new DataView(buffer.buffer);
            var iter = 7;
            bufferView.setUint32(iter, index, true); iter += 4;
            bufferView.setUint8(iter, 0); iter += 1;
            bufferView.setUint8(iter, argNames.length); iter += 1;
            
            for(var i = 0; i < argNames.length; ++i) {
                const str = argNames[i];
                var nBytes = 0;
                // get utf8 length;
                for(var n = 0; n < str.length; ++n) {
                    const val = str.codePointAt(n);
                    if (val < 0x80) {
                        nBytes += 1;
                    } else if (val < 0x800) {
                        nBytes += 2;
                    } else if (val < 0x10000) {
                        nBytes += 3;    
                    } else {
                        nBytes += 4;
                    }                    
                }
            
            
                bufferView.setUint32(iter, nBytes, true);
                iter += 4;
                for(var n = 0; n < str.length; ++n) {
                    const val = str.codePointAt(n);
                    if (val < 0x80) {
                        bufferView.setUint8(iter++, val & 0x7F);
                    } else if (val < 0x800) {
                        bufferView.setUint8(iter++, ((val & 0x7C0) >> 6) | 0xC0);
                        bufferView.setUint8(iter++, (val & 0x3F) | 0x80); 
                    } else if (val < 0x10000) {
                        bufferView.setUint8(iter++, ((val & 0xF000) >> 12) | 0xE0); 
                        bufferView.setUint8(iter++, ((val & 0xFC0) >> 6) | 0x80); 
                        bufferView.setUint8(iter++, (val & 0x3F) | 0x80); 
                    } else {
                        bufferView.setUint8(iter++, ((val & 0x1C0000) >> 18) | 0xF0); 
                        bufferView.setUint8(iter++, ((val & 0x3F000) >> 12) | 0x80); 
                        bufferView.setUint8(iter++, ((val & 0xFC0) >> 6) | 0x80); 
                        bufferView.setUint8(iter++, (val & 0x3F) | 0x80); 
                    }

                }
            };
            
            const stubs = bytecode.stubsFromBytecode(0, buffer.buffer);
            const stub = stubs[0];
            
            vm_extStubs[index] = stub;
            vm_extFuncs[index] = store.createFunction(stub);
        };

        const vm_stackframeGetReferrable = function(i, referrableID) {
            i = vm_stacksize - 1 - i;
            if (i >= vm_stacksize) {
                vm.raiseErrorString("Invalid stackframe requested in referrable query.");
                return undefined;
            }
            
            if (referrableID < vm_callstack[i].referrableSize) {
                return store.valueObjectArrayAtUnsafe(vm_callstack[i].referrable, referrableID);
            } else {
                return store.valueGetCapturedValue(vm_callstack[i].context, referrableID - vm_callstack[i].referrableSize);
            }
        };
        
        const vm_stackframeSetReferrable = function(i, referrableID, val) {
            i = vm_stacksize - 1 - i;
            if (i >= vm_stacksize) {
                vm.raiseErrorString("Invalid stackframe requested in referrable assignment.");
                return;
            }
            
            if (referrableID < vm_callstack[i].referrableSize) {
                const vkey = store.createNumber(referrableID);
                store.valueObjectSet(vm_callstack[i].referrable, vkey, val, 1);
            } else {
                store.valueSetCapturedValue(
                    vm_callstack[i].context,
                    referrableID - vm_callstack[i].referrableSize,
                    val
                );
            }
        };

        const vm_popFrame = function() {
            if (vm_stacksize == 0) return;
            vm_stacksize--;
        }

        const vm_listen = function(v, respObject) {
            if (!store.valueIsCallable(v)) {
                vm.raiseErrorString("Listen expressions require that the listened-to expression is a function. ");
                return store.empty;
            }
            
            const typ_respObject = valToType(respObject);
            if (typ_respObject != store.TYPE_EMPTY && typ_respObject != store.TYPE_OBJECT) {
                vm.raiseErrorString("Listen requires that the response expression is an object.");
                return store.empty;                    
            }
            
            var onSend;
            var onError;
            if (typ_respObject != store.TYPE_EMPTY) {
                onSend = store.valueObjectAccessDirect(respObject, vm_specialString_onsend, 0);
                if (onSend && !store.valueIsCallable(onSend)) {
                    vm.raiseErrorString("Listen requires that the response object's 'onSend' attribute be a Function.");
                    return store.empty;                                            
                }
                onError = store.valueObjectAccessDirect(respObject, vm_specialString_onerror, 0);
                if (onError && !store.valueIsCallable(onError)) {
                    vm.raiseErrorString("Listen requires that the response object's 'onError' attribute be a Function.");
                    return store.empty;                                            
                }
            }
            
            var out = vm.callFunction(v, [], []);
            if (vm_pendingCatchable) {
                const catchable = vm_catchable;
                vm_catchable = store.empty;;
                vm_pendingCatchable = 0;
                
                if (vm_pendingCatchableIsError && onSend) {
                    return vm.callFunction(onSend, [catchable], [vm_specialString_message]);
                }
                
                if (vm_pendingCatchableIsError && onError) {
                    vm_pendingCatchableIsError = 0;
                    return vm.callFunction(onError, [catchable], [vm_specialString_message]);
                } else {
                    if (vm_pendingCatchableIsError) {
                        vm_catchable = catchable;
                        vm_pendingCatchable = 1;
                    } else {
                        return catchable
                    }
                }
                return store.empty; // shouldnt get here
            } else {
                return out;
            }
        };
        

        const vm_pushFrame = function() {
            vm_stacksize ++;

            while(vm_callstack.length < vm_stacksize) {
                vm_callstack.push({
                    pc : 0,
                    prettyName : "",
                    context : store.empty,
                    stub : null,
                    valueStack : []
                });
            }
            
            const frame = vm_callstack[vm_stacksize-1];
            frame.stub = null;
            frame.prettyName = "";
            frame.valueStack.length = 0;
            frame.pc = 0;
            frame.referrableSize = 0;
            
            if (vm_pendingRestartCondition) {
                frame.restartCondition = vm_pendingRestartCondition;
                frame.restartConditionData = vm_pendingRestartConditionData;
                vm_pendingRestartCondition = null;
                vm_pendingRestartConditionData = null;
            } else {
                frame.restartCondition = null;
                frame.restartConditionData = null;
            }
            return frame;
        };
        
        const vm_findStub = function(fileid, stubid) {
            return (vm_stubIndex[fileid])[stubid];
        };
        
        const vm_executionLoop = function() {
            const frame = vm_callstack[vm_stacksize-1];
            var inst;
            const stub = frame.stub;
            frame.sfsCount = 0;
            // tucked with RELOOP
            while(true) {
                while(frame.pc < frame.stub.instructionCount) {
                    inst = stub.instructions[frame.pc++];                        
                    vm_opcodeSwitch[inst.opcode](frame, inst);
                    if (vm_pendingCatchable)
                        break;
                        
                    ////////// DEBUGGING PURPOSES ONLY
                    ////////// Please re-comment for deployment!
                        // check if any invalid values were introduced, such as undefined
                        //for(var i = 0; i < frame.valueStack.length; ++i) {
                        //    if (frame.valueStack[i] == undefined || frame.valueStack[i] == null || frame.valueStack[i].binID == undefined)
                        //        throw new Error('Value stack poisoned.');                            
                        //}
                    
                    //////////
                }
                
                var output;
                if (frame.valueStack.length) {
                    output = frame.valueStack[frame.valueStack.length-1];
                    if (vm_pendingCatchable)
                        output = store.empty;
                    while(frame.valueStack.length)
                        frame.valueStack.pop();
                } else {
                    output = store.empty;
                }
                
                if (frame.restartCondition && !vm_pendingCatchable) {
                    if (frame.restartCondition(frame, output, frame.restartConditionData)) {
                        frame.pc = 0;
                        continue; // reloop
                    }
                }
                return output;
            }

        };
        // controls the store
        vm.store = store;


            
        vm.getBuiltinFunctionAsValue = function(extCall) {
            if (extCall > vm.EXT_CALL.GETEXTERNALFUNCTION) return;
            return vm_extFuncs[extCall];
        };
            
            
        // Performs import as if from the code context,
        // invoking "onImport"
        vm.import = function(module, parameters, data) {
            if (vm_imports[module] != undefined)
                return vm_imports[module];
            
            
            if (data == undefined)
                data = vm.onImport(module);
                
            if (!data) {
                vm.raiseErrorString("Could not retrieve bytecode data for import.");
                return;
            }
            
            const v = vm.runBytecode(data, parameters, module);
            vm_imports[module] = v;
            return v;
        };
        
        vm.runBytecode = function(bytes, parameters, name) {
            const fileID = vm_getNewFileID(name);
            const stubs = bytecode.stubsFromBytecode(fileID, bytes);
            // addstubs
            for(var i = 0; i < stubs.length; ++i) {
                const stub = stubs[i];
                var fileindex = vm_stubIndex[stub.fileID];
                if (fileindex == undefined) {
                    fileindex = {};
                    vm_stubIndex[stub.fileID] = fileindex;
                }
                fileindex[stub.stubID] = stub;
            }
            
            if (name == undefined)
                name = '___matte&___' + fileID;
            return vm.runFileID(fileID, parameters, name);
        };
        
        vm.setExternalFunction = function(name, args, fn) {
            var id = vm_externalFunctions[name];
            if (!id) {
                id = vm_externalFunctionIndex.length;
                vm_externalFunctions[name] = id;
            }
            vm_addBuiltIn(id, args, fn);
        };
        
        
        vm.valueToString = function(val) {
            return store.valueAsString(val) + ' => (' + store.valueTypeName(store.valueGetType(val)) + ')';
        };
            
            
        vm.raiseErrorString = function(string) {
            vm.raiseError(store.createString(string));
        };
            
            
        vm.getStackframe = function(i) {
            i = vm_stacksize - 1 - i;
            if (i >= vm_stacksize) {
                vm.raiseErrorString("Invalid stackframe requested");
                return {};
            }
            return vm_callstack[i];
        };
        
        vm.getStackSize = function() {
            return vm_stacksize;
        };
            
            
        vm.raiseError = function(val) {
            if (vm_pendingCatchable)
                return;
                
            const info = vm_infoNewObject(val);

            vm_catchable = info;
            if (vm_stacksize == 0) {
                vm_errorLastFile = 0;
                vm_errorLastLine = -1;
                vm_pendingCatchable = 1;
                vm_pendingCatchableIsError = 1;
                
                // debug here 
                
                return;
            }
            
            const framesrc = vm.getStackframe(0);
            const instcount = framesrc.stub.instructions.length;
            if (framesrc.pc - 1 < instcount) {
                vm_errorLastLine = framesrc.stub.instructions[framesrc.pc-1].lineNumber;
            } else {
                vm_errorLastLine = -1;
            }
            vm_errorLastFile = framesrc.stub.fileID;
            
            
            // debug here 
            vm_pendingCatchable = 1;
            vm_pendingCatchableIsError = 1;
            
        };
            
        vm.runFileID = function(fileid, parameters, importPath) {
            if (fileid == 0xfffffffe) // debug fileID
                throw new Error('Matte debug context is not currently supported');

            const precomp = vm_precomputed[fileid];
            if (precomp != undefined) return precomp;
            
            if (parameters == undefined)
                parameters = store.empty;
            
            const stub = vm_findStub(fileid, 0);
            if (stub == undefined) {
                vm.raiseErrorString('Script has no toplevel context to run');
                return store.empty;
            }
            const func = store.createFunction(stub);
            const result = vm.callFunction(func, [parameters], [vm_specialString_parameters]);
            vm_precomputed[fileid] = result;
            return result;
        },
            
        vm.callVarargFunction = function(func, callingContext, args, dynBind) {
            if (valToType(args) != store.TYPE_OBJECT) {
                vm.raiseErrorString("Vararg calls MUST provide an expression that results in an Object.");
                return store.empty;
            }
            
            const stub = store.valueGetBytecodeStub(func);
            
            if (!stub) {
                vm.raiseErrorString("Cannot use a vararg call on something that isnt a function.");
                return store.empty;
                
            }
            
            
            const argNames = [];
            const argVals = [];
            const dynBindName = store.getDynamicBindToken();
            
            if (dynBind != store.empty) {
                argVals.push(dynBind);
                argNames.push(dynBindName);
            }
            
            
            if (stub.isVarArg) {
                const keys = store.valueObjectKeys(args);
                const len = store.valueObjectGetNumberKeyCount(keys);
                for(var i = 0; i < len; ++i) {
                    const key = store.valueObjectAccessIndex(keys, i);
                    if (valToType(key) != store.TYPE_STRING) continue;
                    
                    if (key == dynBindName) continue;
                    
                    argNames[i] = key;
                    const arg = store.valueObjectAccess(args, key, 0);
                    argVals[i] = arg;
                    argNames[i] = key;
                    
                }
            } else {
                const len = stub.argCount;
                for(var i = 0; i < len; ++i) {
                    const name = store.createString(stub.argNames[i]);
                    
                    if (name == dynBindName) continue;
                    
                    const arg = store.valueObjectAccess(args, name, 0);
                    argVals[i] = arg;
                    argNames[i] = name;
                }
            }
            
            return vm.callFunction(func, argVals, argNames, callingContext);
        }
        
            
        vm.callFunction = function(func, args, argNames, callingContext) {
            if (vm_pendingCatchable) return store.empty;
            if (func == store.emptyFunction) {
                vm_pendingRestartCondition = null;
                vm_pendingRestartConditionData = null;
                return store.empty;
            }
            const callable = store.valueIsCallable(func);
            if (callable == 0) {
                vm.raiseErrorString("Error: cannot call non-function value ");
                return store.empty;
            }
            
            if (args.length != argNames.length) {
                vm.raiseErrorString("VM call as mismatching arguments and parameter names.");
                return store.empty;                        
            }
            
            if (valToType(func) == store.TYPE_TYPE) {
                if (args.length) {
                    if (argNames[0] != vm_specialString_from &&
                        argNames[0] != '') {
                        vm.raiseErrorString("Type conversion failed: unbound parameter to function ('from')");
                    }
                    return store.valueToType(args[0], func);
                } else {
                    vm.raiseErrorString("Type conversion failed (no value given to convert).");
                    return store.empty;
                }
            }
            
            
            //external function 
            const stub = store.valueGetBytecodeStub(func);
            if (stub.fileID == 0) {
                const external = stub.stubID;
                if (external >= vm_externalFunctionIndex.length) {
                    return store.empty;
                }
                
                const set = vm_externalFunctionIndex[external];
                const argsReal = [];
                const lenReal = args.length;
                const len = stub.argCount;

                for(var i = 0; i < len; ++i) {
                    argsReal.push(store.empty);
                }
                
                if (lenReal == 1 && argNames[0] == '') {
                    if (len > 1) {
                        vm.raiseErrorString("Call requested automatic binding using an expression argument, but it is vague which parameter this belongs to. Automatic binding is only available to functions that require a single argument.");
                        return store.empty;                        
                    }
                    argsReal[0] = args[0];                    
                } else if (lenReal == 2 && argNames[0] == '' &&
                                           argNames[1] == 'base'
                ) {
                    if (len > 2) {
                        vm.raiseErrorString("Call requested automatic binding using an expression argument, but it is vague which parameter this belongs to. Automatic binding is only available to functions that require a single argument.");
                        return store.empty;                        
                    }
                    argsReal[0] = args[1];                    
                    argsReal[1] = args[0];                    
                } else {
                    for(var i = 0; i < lenReal; ++i) {
                        for(var n = 0; n < len; ++n) {
                            if (stub.argNames[n] == argNames[i]) {
                                argsReal[n] = args[i];
                                break;
                            }
                        }
                        
                        if (n == len) {
                            var str;
                            if (len) {
                                str = "Could not bind requested parameter: '"+argNames[i]+"'.\n Bindable parameters for this function: ";
                            } else {
                                str = "Could not bind requested parameter: '"+argNames[i]+"'.\n (no bindable parameters for this function)";                                
                            }
                            
                            for(n = 0; n < len; ++n) {
                                str += " \"" + stub.argNames[n] + "\" ";
                            }
                            
                            vm.raiseErrorString(str);
                            return store.empty;
                        }
                    }
                }
                var result;
                if (callable == 2) {
                    const ok = store.valueObjectFunctionPreTypeCheckUnsafe(func, argsReal);
                    if (ok) 
                        result = set.userFunction(func, argsReal, set.userData);
                    store.valueObjectFunctionPostTypeCheckUnsafe(func, result);
                } else {
                    result = set.userFunction(func, argsReal, set.userData);                        
                }
                return result;

            // non-external call
            } else {
                const lenReal = args.length;
                var   len = stub.argCount;
                const referrables = [];
                for(var i = 0; i < len; ++i) {
                    referrables.push(store.empty);
                }
                
                
                if (stub.isVarArg) {
                    const val = store.createObject();
                    for(i = 0; i < lenReal; ++i) {
                        store.valueObjectSet(val, argNames[i], args[i], 0);
                    }
                    referrables[0] = val;
                } else {
                    if (lenReal == 1 && argNames[0] == '') {
                        if (len > 1) {
                            vm.raiseErrorString("Call requested automatic binding using an expression argument, but it is vague which parameter this belongs to. Automatic binding is only available to functions that require a single argument.");
                        }
                        referrables[0] = args[0];                            
                    } else {
                        const nameMap = {};
                        
                        for(var i = 0; i < lenReal; ++i) {
                            nameMap[argNames[i]] = i;
                        };
                        
                        for(var i = 0; i < len; ++i) {
                            const name = stub.argNames[i];
                            const res = nameMap[name];
                            if (res == undefined) {                      
                            } else {
                                delete nameMap[name];
                                referrables[i] = args[res];                            
                            }
                        }
                        
                        
                        const unbound = Object.keys(nameMap);
                        if (unbound.length) {
                            const which = unbound[0];
                            var str;
                            if (len) {
                                str = "Could not bind requested parameter: '"+which+"'.\n Bindable parameters for this function: ";
                            } else {
                                str = "Could not bind requested parameter: '"+which+"'.\n (no bindable parameters for this function)";                                
                            }
                            
                            for(n = 0; n < len; ++n) {
                                str += " \"" + stub.argNames[n] + "\" ";
                            }
                            
                            vm.raiseErrorString(str);
                            return store.empty;                          
                        }
                    }
                }
                
                var ok = 1;
                if (callable == 2 && len) {
                    const arr = referrables.slice(0, referrables.length); 
                    ok = store.valueObjectFunctionPreTypeCheckUnsafe(func, arr);
                }
                len = stub.localCount;
                for(var i = 0; i < len; ++i) {
                    referrables.push(store.empty);
                }
                const frame = vm_pushFrame();
                frame.context = func;
                frame.privateBinding = callingContext;
                frame.stub = stub;
                frame.referrable = store.createObjectArray(referrables);
                frame.referrableSize = referrables.length;
                
                var result;
                if (callable == 2) {
                    if (ok) {
                        result = vm_executionLoop(vm);
                    } 
                    if (!vm_pendingCatchable)
                        store.valueObjectFunctionPostTypeCheckUnsafe(func, result);
                } else {
                    result = vm_executionLoop(vm);
                }
                vm_popFrame();
                
                if (!vm_stacksize && vm_pendingCatchable) {
                    console.log('Uncaught error: ' + store.valueObjectAccessString(vm_catchable, 'summary'));                        
                    /*
                    if (vm_unhandled) {
                        vm_unhandled(
                            vm_errorLastFile,
                            vm_errorLastLine,
                            vm_catchable,
                            vm_unhandledData
                        );
                    }*/
                    if (vm.unhandledError) {
                        vm.unhandledError(
                            vm_errorLastFile,
                            vm_errorLastLine,
                            vm_catchable
                        );
                    } else 
                        throw new Error('Uncaught error');
                    
                    vm_catchable = store.empty;
                    vm_pendingCatchable = 0;
                    vm_pendingCatchableIsError = 0;
                    //throw new Error("Fatal error.");
                    return store.empty;
                }


                return result;
            }
        };
        
            
        // external function that takes a module path
        // and returns a ByteArray
        vm.onImport = onImport;
        
        const vm_operator_2 = function(op, a, b) {
            return vm_operatorFunc[op](a, b);
        };
        
        const vm_operator_1 = function(op, a, b) {
            return vm_operatorFunc[op](a);
        };
        
        
        // implement operators 
        /////////////////////////////////////////////////////////////////
        const vm_badOperator = function(op, value) {
            vm.raiseErrorString(
                op + " operator on value of type " + store.valueTypeName(store.valueGetType(value)) + " is undefined."
            );
        };
        
        const vm_objectHasOperator = function(a, op) {
            const opSrc = store.valueObjectGetAttributesUnsafe(a);
            if (opSrc == undefined)
                return false;
            
            return store.valueObjectAccessString(opSrc, op) != undefined;
        };
        
        const vm_runObjectOperator2 = function(a, op, b) {
            const opSrc = store.valueObjectGetAttributesUnsafe(a);
            if (opSrc == undefined) {
                vm.raiseErrorString(op + ' operator on object without operator overloading is undefined.');
                return store.empty;
            } else {
                const oper = store.valueObjectAccessString(opSrc, op);
                return vm.callFunction(
                    oper,
                    [b],
                    [vm_specialString_value]
                );
            }                
        };
        
        
        const vm_runObjectOperator1 = function(a, op) {
            const opSrc = store.valueObjectGetAttributesUnsafe(a);
            if (opSrc == undefined) {
                vm.raiseErrorString(op + ' operator on object without operator overloading is undefined.');
                return store.empty;
            } else {
                const oper = store.valueObjectAccessString(opSrc, op);
                return vm.callFunction(
                    oper,
                    [],
                    []
                );
            }                
        };          
        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ADD] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a + store.valueAsNumber(b));
                
              case store.TYPE_STRING:
                const astr = a;
                const bstr = store.valueAsString(b);
                return store.createString(astr + bstr);
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '+', b);
                
              default:
                vm_badOperator('+', a);                  
            }
            return store.empty;
        };



        vm_operatorFunc[vm_operator.MATTE_OPERATOR_SUB] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a - store.valueAsNumber(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '-', b);
                
              default:
                vm_badOperator('-', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_DIV] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a / store.valueAsNumber(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '/', b);
                
              default:
                vm_badOperator('/', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_MULT] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a * store.valueAsNumber(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '*', b);
                
              default:
                vm_badOperator('*', a);                  
            }
            return store.empty;
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_NOT] = function(a) {
            switch(valToType(a)) {
              case store.TYPE_BOOLEAN:
                return store.createBoolean(!store.valueAsBoolean(a));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '!');
                
              default:
                vm_badOperator('!', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_NEGATE] = function(a) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(-a);
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '-()');
                
              default:
                vm_badOperator('-()', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_BITWISE_NOT] = function(a) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(~(a));
            
              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '~');
                
              default:
                vm_badOperator('~', a);                  
            }
            return store.empty;
        };
        
        vm_operatorFunc[vm_operator.MATTE_OPERATOR_POUND] = function(a) {return vm_operatorOverloadOnly1("#", a);};

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_BITWISE_OR] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a | store.valueAsNumber(b));

              case store.TYPE_BOOLEAN:
                return store.createBoolean(a | store.valueAsBoolean(b));

              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '|');
                
              default:
                vm_badOperator('|', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_OR] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_BOOLEAN:
                return store.createBoolean(a || store.valueAsBoolean(b));

              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '||');
                
              default:
                vm_badOperator('||', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_BITWISE_AND] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a & store.valueAsNumber(b));                
            
              case store.TYPE_BOOLEAN:
                return store.createBoolean(a & store.valueAsBoolean(b));

              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '&');
                
              default:
                vm_badOperator('&', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_AND] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_BOOLEAN:
                return store.createBoolean(a && store.valueAsBoolean(b));

              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '&&');
                
              default:
                vm_badOperator('&&', a);                  
            }
            return store.empty;                                
        };


        const vm_operatorOverloadOnly2 = function(operator, a, b) {
            switch(valToType(a)) {
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, operator, b);
              default:
                vm_badOperator(operator, a);
            };
            return store.empty;
        };

        const vm_operatorOverloadOnly1 = function(operator, a) {
            switch(valToType(a)) {
              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, operator);
              default:
                vm_badOperator(operator, a);
            };
            return store.empty;
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_POW] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(Math.pow(a, store.valueAsNumber(b)));

              case store.TYPE_OBJECT:
                return vm_runObjectOperator1(a, '**');
                
              default:
                vm_badOperator('**', a);                  
            }
            return store.empty;
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_EQ] = function(a, b) {
            const typ_a = valToType(a);
            const typ_b = valToType(b);
            if (typ_b == store.TYPE_EMPTY && typ_a != store.TYPE_EMPTY) {
                return store.createBoolean(false);
            }
            switch(typ_a) {
              case store.TYPE_EMPTY:
                return store.createBoolean(typ_b == 0);
                
              case store.TYPE_NUMBER:
                return store.createBoolean(a == store.valueAsNumber(b));

              case store.TYPE_STRING:
                return store.createBoolean(a == store.valueAsString(b));

              case store.TYPE_BOOLEAN:
                return store.createBoolean(a == store.valueAsBoolean(b));

              case store.TYPE_OBJECT:
                if (vm_objectHasOperator(a, '==')) {
                    return vm_runObjectOperator2(a, '==', b);
                } else {
                    if (typ_b == 0) {
                        return store.createBoolean(false);
                    } else if (typ_b == store.TYPE_OBJECT) {
                        return store.createBoolean(a === b);
                    } else {
                        vm.raiseErrorString("== operator with object and non-empty or non-object values is undefined.");
                        return store.createBoolean(false);
                    }
                }
              case store.TYPE_TYPE:
                if (typ_b == typ_a) {
                    return store.createBoolean(a.id == b.id);
                } else {
                    vm.raiseErrorString("!= operator with Type and non-type is undefined.");
                    return store.empty;                        
                }
              default:
                vm_badOperator('==', a);                  
            }
            return store.empty;
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_NOTEQ] = function(a, b) {
            const typ_a = valToType(a);
            const typ_b = valToType(b);

            if (typ_b == store.TYPE_EMPTY && typ_a != store.TYPE_EMPTY) {
                return store.createBoolean(true);
            }
            switch(valToType(a)) {
              case store.TYPE_EMPTY:
                return store.createBoolean(typ_b != 0);
                
              case store.TYPE_NUMBER:
                return store.createBoolean(a != store.valueAsNumber(b));

              case store.TYPE_STRING:
                return store.createBoolean(a != store.valueAsString(b));

              case store.TYPE_BOOLEAN:
                return store.createBoolean(a != store.valueAsBoolean(b));


              case store.TYPE_OBJECT:
                if (vm_objectHasOperator(a, '!=')) {
                    return vm_runObjectOperator2(a, '!=', b);
                } else {
                    if (typ_b == 0) {
                        return store.createBoolean(true);
                    } else if (typ_b == store.TYPE_OBJECT) {
                        return store.createBoolean(a != b);
                    } else {
                        vm.raiseErrorString("!= operator with object and non-empty or non-object values is undefined.");
                        return store.createBoolean(true);
                    }
                }
              case store.TYPE_TYPE:
                if (typ_b == typ_a) {
                    return store.createBoolean(a.id != b.id);
                } else {
                    vm.raiseErrorString("!= operator with Type and non-type is undefined.");
                    return store.empty;                        
                }
              default:
                vm_badOperator('!=', a);                  
            }
            return store.empty;
        };




        vm_operatorFunc[vm_operator.MATTE_OPERATOR_LESS] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createBoolean(a < store.valueAsNumber(b));
                
              case store.TYPE_STRING:
                return store.createBoolean(
                    a < store.valueAsString(b)
                );
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "<", b);
              default:
                vm_badOperator("<", a);
                return store.empty;
            }
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_GREATER] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createBoolean(a > store.valueAsNumber(b));
                
              case store.TYPE_STRING:
                return store.createBoolean(
                    a > store.valueAsString(b)
                );
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, ">", b);
              default:
                vm_badOperator(">", a);
                return store.empty;
            }
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_LESSEQ] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createBoolean(a <= store.valueAsNumber(b));
                
              case store.TYPE_STRING:
                return store.createBoolean(
                    a <= store.valueAsString(b)
                );
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "<=", b);
              default:
                vm_badOperator("<=", a);
                return store.empty;
            }
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_GREATEREQ] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createBoolean(a >= store.valueAsNumber(b));
                
              case store.TYPE_STRING:
                return store.createBoolean(
                    a >= store.valueAsString(b)
                );
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, ">=", b);
              default:
                vm_badOperator(">=", a);
                return store.empty;
            }
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_MODULO] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a % store.valueAsNumber(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '%', b);
                
              default:
                vm_badOperator('%', a);                  
            }
            return store.empty;
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_TYPESPEC] = function(a, b) {
            switch(valToType(b)) {
              case store.TYPE_TYPE:
                if (!store.valueIsA(a, b)) {
                    vm.raiseErrorString("Type specifier (=>) failure: expected value of type '"+store.valueTypeName(b)+"', but received value of type '"+store.valueTypeName(store.valueGetType(a)).data+"'");
                }
                return a;
              default:
                vm_badOperator('=>', a);                  
            }
            return store.empty;
        };


        vm_operatorFunc[vm_operator.MATTE_OPERATOR_CARET] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a ^ store.valueAsNumber(b));
              case store.TYPE_BOOLEAN:
                return store.createBoolean(a ^ store.valueAsBoolean(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '^', b);
                
              default:
                vm_badOperator('^', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_SHIFT_LEFT] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a << store.valueAsNumber(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '<<', b);
                
              default:
                vm_badOperator('<<', a);                  
            }
            return store.empty;
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_SHIFT_RIGHT] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a >> store.valueAsNumber(b));
                
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, '>>', b);
                
              default:
                vm_badOperator('>>', a);                  
            }
            return store.empty;
        };


        
        
        vm_operatorFunc[vm_operator.MATTE_OPERATOR_POINT]       = function(a, b) {return vm_operatorOverloadOnly2("->", a, b);}
        vm_operatorFunc[vm_operator.MATTE_OPERATOR_TERNARY]     = function(a, b) {return vm_operatorOverloadOnly2("?",  a, b);}
        vm_operatorFunc[vm_operator.MATTE_OPERATOR_TRANSFORM]   = function(a, b) {return vm_operatorOverloadOnly2("<>", a, b);}
        

    

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_ADD] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a + store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "+=", b);
              default:
                vm_badOperator("+=", a);
                return store.empty;
            }
        };
        

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_SUB] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a - store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "-=", b);
              default:
                vm_badOperator("-=", a);
                return store.empty;
            }
        };

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_DIV] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a / store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "/=", b);
              default:
                vm_badOperator("/=", a);
                return store.empty;
            }
        };            
        
        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_MULT] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a * store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "*=", b);
              default:
                vm_badOperator("*=", a);
                return store.empty;
            }
        };            

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_MOD] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a % store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "%=", b);
              default:
                vm_badOperator("%=", a);
                return store.empty;
            }
        };                 

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_POW] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(Math.pow(a, store.valueAsNumber(b)));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "**=", b);
              default:
                vm_badOperator("**=", a);
                return store.empty;
            }
        };                 

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_AND] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a & store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "&=", b);
              default:
                vm_badOperator("&=", a);
                return store.empty;
            }
        };                 

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_OR] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a | store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "|=", b);
              default:
                vm_badOperator("|=", a);
                return store.empty;
            }
        };                 

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_XOR] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a ^ store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "^=", b);
              default:
                vm_badOperator("^=", a);
                return store.empty;
            }
        };                 

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_BLEFT] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a << store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, "<<=", b);
              default:
                vm_badOperator("<<=", a);
                return store.empty;
            }
        };                 

        vm_operatorFunc[vm_operator.MATTE_OPERATOR_ASSIGNMENT_BRIGHT] = function(a, b) {
            switch(valToType(a)) {
              case store.TYPE_NUMBER:
                return store.createNumber(a >> store.valueAsNumber(b));
              case store.TYPE_OBJECT:
                return vm_runObjectOperator2(a, ">>=", b);
              default:
                vm_badOperator(">>=", a);
                return store.empty;
            }
        };                 

        
        
        
        // implement opcodes:
        ////////////////////////////////////////////////////////////////
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NOP] = function(frame, inst){};

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_LST] = function(frame, inst){
            const v = vm_listen(frame.valueStack[frame.valueStack.length-2], frame.valueStack[frame.valueStack.length-1]);
            frame.valueStack.pop(); 
            frame.valueStack.pop(); 
            frame.valueStack.push(v); 
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_PIP] = function(frame, inst) {
            if (frame.privateBinding == undefined) {
                vm.raiseErrorString("The private interface binding is only available for functions that are called directly from an interface.");
                return;                    
            }
            frame.valueStack.push(frame.privateBinding);
        };
        
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_PRF] = function(frame, inst) {
            const v = vm_stackframeGetReferrable(0, Math.floor(inst.data));
            frame.valueStack.push(v);
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_PNR] = function(frame, inst) {
            throw new Error("TODO"); // or not, DEBUG
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NEM] = function(frame, inst) {
            frame.valueStack.push(store.empty);
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NNM] = function(frame, inst) {
            frame.valueStack.push(store.createNumber(inst.data));
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NBL] = function(frame, inst) {
            frame.valueStack.push(store.createBoolean(Math.floor(inst.data) == 1));
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NST] = function(frame, inst) {
            frame.valueStack.push(store.createString(frame.stub.strings[Math.floor(inst.data)]));
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NOB] = function(frame, inst) {
            frame.valueStack.push(store.createObject());
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SFS] = function(frame, inst) {
            frame.sfsCount = inst.data;
            // the fallthrough
            if (frame.pc >= frame.stub.instructionCount) return;
            inst = frame.stub.instructions[frame.pc++];
            vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NFN](frame, inst);   
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NEF] = function(frame, inst) {
            frame.valueStack.push(store.emptyFunction);
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_NFN] = function(frame, inst) {
            const stub = vm_findStub(inst.nfnFileID, inst.data);
            if (stub == undefined) {
                vm.raiseErrorString("NFN opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)");
                return;
            }
            
            if (frame.sfsCount) {
                if (frame.valueStack.length < frame.sfsCount) {
                    vm.raiseErrorString("VM internal error: too few values on stack to service SFS opcode!");
                    return;
                }
                
                const vals = [];
                for(var i = 0; i < frame.sfsCount; ++i) {
                    vals.push(store.empty);
                }
                for(var i = 0; i < frame.sfsCount; ++i) {
                    vals[frame.sfsCount - i - 1] = frame.valueStack[frame.valueStack.length-1-i];
                }
                const v = store.createTypedFunction(stub, vals);


                for(var i = 0; i < frame.sfsCount; ++i) {
                    frame.valueStack.pop();
                }

                frame.sfsCount = 0;
                frame.valueStack.push(v);
                
                
                
            } else {
                frame.valueStack.push(store.createFunction(stub));
            }
        };
        
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_CAA] = function(frame, inst) {
            if (frame.valueStack.length < 2) {
                vm.raiseErrorString("VM error: missing object - value pair for constructor push");
                return;
            }
            const obj = frame.valueStack[frame.valueStack.length-2];
            store.valueObjectPush(obj, frame.valueStack[frame.valueStack.length-1]);
            frame.valueStack.pop();
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_CAS] = function(frame, inst) {
            if (frame.valueStack.length < 3) {
                vm.raiseErrorString("VM error: missing object - value pair for constructor push");
                return;
            }
            const obj = frame.valueStack[frame.valueStack.length-3];
            store.valueObjectSet(obj, frame.valueStack[frame.valueStack.length-2], frame.valueStack[frame.valueStack.length-1], 1);
            frame.valueStack.pop();
            frame.valueStack.pop();
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SPA] = function(frame, inst) {
            if (frame.valueStack.length < 2) {
                vm.raiseErrorString("VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");
                return;
            }
            const p = frame.valueStack.pop();
            const target = frame.valueStack[frame.valueStack.length-1];
            const len = store.valueObjectGetNumberKeyCount(p);
            var keylen = store.valueObjectGetNumberKeyCount(target);
            for(var i = 0; i < len; ++i) {
                store.valueObjectInsert(
                    target, 
                    keylen++,
                    store.valueObjectAccessIndex(p, i)
                );
            }   
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SPO] = function(frame, inst) {
            if (frame.valueStack.length < 2) {
                vm.raiseErrorString("VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");
                return;
            }
            
            const p = frame.valueStack.pop();
            const keys = store.valueObjectKeys(p);
            const vals = store.valueObjectValues(p);
            
            const target = frame.valueStack[frame.valueStack.length-1];
            const len = store.valueObjectGetNumberKeyCount(keys);
            for(var i = 0; i < len; ++i) {
                store.valueObjectSet(
                    target,
                    store.valueObjectAccessIndex(keys, i),
                    store.valueObjectAccessIndex(vals, i),
                    1
                );
            }
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_PTO] = function(frame, inst) {
            var v;
            switch(Math.floor(inst.data)) {
              case 0: v = store.getEmptyType(); break;
              case 1: v = store.getBooleanType(); break;
              case 2: v = store.getNumberType(); break;
              case 3: v = store.getStringType(); break;
              case 4: v = store.getObjectType(); break;
              case 5: v = store.getFunctionType(); break;
              case 6: v = store.getTypeType(); break;
              case 7: v = store.getAnyType(); break;
              case 8: v = store.getNullableType(); break;
            }
            frame.valueStack.push(v);
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_CLV] = function(frame, isnt) {
            if (frame.valueStack.length < 2) {
                vm.raiseErrorString("VM error: tried to prepare arguments for a vararg call, but insufficient arguments on the stack.");
                return;
            }          

            const func = frame.valueStack[frame.valueStack.length - 2];
            var dynBind;
            var privateBinding;
                
            if (func.idAux != undefined) {        
                const stub = store.valueGetBytecodeStub(func);
                
                if (stub && stub.isDynamicBinding) {
                    dynBind = func.idAux;
                }
                
                if (stub && store.valueObjectGetIsInterfaceUnsafe(func.idAux)) {
                    privateBinding = store.valueObjectGetInterfacePrivateBindingUnsafe(func.idAux);
                }
            }

            const result = vm.callVarargFunction(
                frame.valueStack[frame.valueStack.length - 2],
                privateBinding,
                frame.valueStack[frame.valueStack.length - 1],
                dynBind
            );                

            
            frame.valueStack.pop();
            frame.valueStack.pop();
            frame.valueStack.push(result);
              
        }
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_CAL] = function(frame, inst) {
            if (frame.valueStack.length < 1) {
                vm.raiseErrorString("VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");
                return;
            }
            const args = [];
            const argNames = [];
            
            const stackSize = frame.valueStack.length;
            
            var i = 0;
            var isDynBind = false;
            if (stackSize > 2) {
                while(i < stackSize-1) {
                    var key = frame.valueStack[frame.valueStack.length - 1 - i];
                    var value = frame.valueStack[frame.valueStack.length - 2 - i]

                    if (valToType(key) == store.TYPE_STRING) {
                        argNames.push(key);
                        args.push(value);
                        
                    } else {
                        break;
                    }
                    i += 2;
                }
            }
            
            const argCount = args.length;
            /*
            if (i == stackSize) {
                vm.raiseErrorString("VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");         
                return;
            }
            */
            
            const func = frame.valueStack[frame.valueStack.length - 1 - i];
            var privateBinding;
            if (func.idAux != undefined) {        
                const stub = store.valueGetBytecodeStub(func);
                
                if (stub && stub.isDynamicBinding) {
                    args.push(func.idAux);
                    argNames.push(store.getDynamicBindToken());
                }
                
                if (stub && store.valueObjectGetIsInterfaceUnsafe(func.idAux)) {
                    privateBinding = store.valueObjectGetInterfacePrivateBindingUnsafe(func.idAux);
                }
            }

            
            
            const result = vm.callFunction(func, args, argNames, privateBinding);
            
            for(i = 0; i < argCount; ++i) {
                frame.valueStack.pop();
                frame.valueStack.pop();
            }
            frame.valueStack.pop();
            frame.valueStack.push(result);
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_ARF] = function(frame, inst) {
            if (frame.valueStack.length < 1) {
                vm.raiseErrorString("VM error: tried to prepare arguments for referrable assignment, but insufficient arguments on the stack.");
                return;
            }
            
            const refn = inst.data % 0xffffffff;
            const op = Math.floor(inst.data / 0xffffffff);
            
            const ref = vm_stackframeGetReferrable(0, refn);
            const v = frame.valueStack[frame.valueStack.length-1];
            var vOut;
            switch(op + vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE) {
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE:
                vOut = v;
                vm_stackframeSetReferrable(0, refn, vOut);                    
                break;
                
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_ADD:
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_SUB:
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_MULT:
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_DIV: 
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_MOD:
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_POW:
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_AND:
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_OR: 
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_XOR: 
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_BLEFT: 
              case vm_operator.MATTE_OPERATOR_ASSIGNMENT_BRIGHT: 
                vOut = vm_operatorFunc[op + vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE](ref, v); 
                if (valToType(ref) != store.TYPE_OBJECT)
                    vm_stackframeSetReferrable(0, refn, vOut);                    

                break;
                
              default:
                vOut = store.empty;
                vm.raiseErrorString("VM error: tried to access non-existent referrable operation (corrupt bytecode?).");
            }
            frame.valueStack.pop();
            frame.valueStack.push(vOut);
        };
        
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_POP] = function(frame, inst) {
            var popCount = inst.data;
            while(popCount > 0 && frame.valueStack.length > 0) {
                frame.valueStack.pop();
                popCount--;
            }
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_CPY] = function(frame, inst) {
            if (frame.valueStack.length == 0) {
                vm.raiseErrorString("VM error: cannot CPY with empty stack");
                return;
            }
            
            frame.valueStack.push(frame.valueStack[frame.valueStack.length-1]);
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_OSN] = function(frame, inst) {
            if (frame.valueStack.length < 3) {
                vm.raiseErrorString("VM error: OSN opcode requires 3 on the stack.");
                return;
            }
            
            var opr = Math.floor(inst.data);
            var isBracket = 0;
            if (opr >= vm_operator.MATTE_OPERATOR_STATE_BRACKET) {
                opr -= vm_operator.MATTE_OPERATOR_STATE_BRACKET;
                isBracket = 1;
            }
            opr += vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE;
            const key    = frame.valueStack[frame.valueStack.length-1];
            const object = frame.valueStack[frame.valueStack.length-2];
            const val    = frame.valueStack[frame.valueStack.length-3];
            
            if (opr == vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE) {
                frame.valueStack.pop();
                frame.valueStack.pop();
                frame.valueStack.pop();
                const out = store.valueObjectSet(object, key, val, isBracket);
                frame.valueStack.push(
                    out ? out : store.empty
                );   
            } else {
                var ref = store.valueObjectAccessDirect(object, key, isBracket);
                var isDirect = 1;
                if (!ref) {
                    isDirect = 0;
                    ref = store.valueObjectAccess(object, key, isBracket);
                }
                var out;
                switch(opr) {
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_ADD:
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_SUB:
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_MULT:
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_DIV: 
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_MOD:
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_POW:
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_AND:
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_OR: 
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_XOR: 
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_BLEFT: 
                  case vm_operator.MATTE_OPERATOR_ASSIGNMENT_BRIGHT: 
                    out = vm_operatorFunc[opr](ref, val); 
                }
                
                if (!isDirect || valToType(val) != store.TYPE_OBJECT) {
                    store.valueObjectSet(object, key, out, isBracket);
                }
                frame.valueStack.pop();
                frame.valueStack.pop();
                frame.valueStack.pop();
                frame.valueStack.push(out);
            }
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_OLK] = function(frame, inst) {
            if (frame.valueStack.length < 2) {
                vm.raiseErrorString("VM error: OLK opcode requires 2 on the stack.");
                return;
            }
            
            const isBracket = inst.data != 0.0;
            const key = frame.valueStack[frame.valueStack.length-1];
            const object = frame.valueStack[frame.valueStack.length-2];
            const output = store.valueObjectAccess(object, key, isBracket);
            
            frame.valueStack.pop();
            frame.valueStack.pop();
            
            if (output) {
                const stub = store.valueGetBytecodeStub(output);
                if (stub && valToType(key) == store.TYPE_STRING && valToType(object) == store.TYPE_OBJECT) {
                    output.idAux = object;
                }
            }
            frame.valueStack.push(output);
            
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_EXT] = function(frame, inst) {
            if (inst.data >= vm_externalFunctionIndex.length) {
                vm.raiseErrorString("VM error: unknown external call.");
                return;
            }
            frame.valueStack.push(vm_extFuncs[inst.data]);
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_RET] = function(frame, inst) {
            frame.pc = frame.stub.instructionCount;
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SKP] = function(frame, inst) {
            vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SCA](frame, inst);
            frame.valueStack.pop();
        };            
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SCA] = function(frame, inst) {
            const count = inst.data;
            const condition = frame.valueStack[frame.valueStack.length-1];
            if (!store.valueAsBoolean(condition)) {
                frame.pc += count;
            }
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_SCO] = function(frame, inst) {
            const count = inst.data;
            const condition = frame.valueStack[frame.valueStack.length-1];
            if (store.valueAsBoolean(condition)) {
                frame.pc += count;
            }            
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_OAS] = function(frame, inst) {
            const src  = frame.valueStack[frame.valueStack.length-1];
            const dest = frame.valueStack[frame.valueStack.length-2];
            
            store.valueObjectSetTable(dest, src);
            
            frame.valueStack.pop();
        };
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_FVR] = function(frame, inst) {
            const a = frame.valueStack[frame.valueStack.length-1];
            if (!(valToType(a) == store.TYPE_OBJECT && a.function_stub)) {
                vm.raiseErrorString("'forever' requires the trailing expression to be a function.");
                frame.valueStack.pop();
                return;                    
            }
            
            vm_pendingRestartCondition = vm_extCall_foreverRestartCondition;
            vm.callFunction(a, [], []);                
            frame.valueStack.pop();
        }            
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_FCH] = function(frame, inst) {
            const a = frame.valueStack[frame.valueStack.length-2];
            const b = frame.valueStack[frame.valueStack.length-1];

            if (!(valToType(b) == store.TYPE_OBJECT && b.function_stub)) {
                frame.valueStack.pop();
                frame.valueStack.pop();
                vm.raiseErrorString("'foreach' requires the trailing expression to be a function.");
                return;        
            }
            if (!(valToType(b) == store.TYPE_OBJECT)) {
                frame.valueStack.pop();
                frame.valueStack.pop();
                vm.raiseErrorString("'for' requires an object.");
                return;        
            }

            store.valueObjectForeach(a, b);
            frame.valueStack.pop();
            frame.valueStack.pop();                
        };

        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_LOP] = function(frame, inst) {
            const from = frame.valueStack[frame.valueStack.length-3];
            const to   = frame.valueStack[frame.valueStack.length-2];
            const v    = frame.valueStack[frame.valueStack.length-1];

            if (!(valToType(v) == store.TYPE_OBJECT && v.function_stub)) {
                vm.raiseErrorString("'for' requires the trailing expression to be a function.");
                frame.valueStack.pop();
                frame.valueStack.pop();
                frame.valueStack.pop();
                return;
            }



            const d = {
                i:   store.valueAsNumber(from),
                end: store.valueAsNumber(to), 
                usesi : store.valueGetBytecodeStub(v).argCount != 0
            };
            
            if (d.i == d.end) {
                frame.valueStack.pop();
                frame.valueStack.pop();
                frame.valueStack.pop();
                return;
            }
            
            if (d.i >= d.end) {
                d.offset = -1;
                vm_pendingRestartCondition = vm_extCall_forRestartCondition_down
            } else {
                d.offset = 1;
                vm_pendingRestartCondition = vm_extCall_forRestartCondition_up
            }

            vm_pendingRestartConditionData = d;
            const iter = store.createNumber(d.i);
            
            
            const stub = store.valueGetBytecodeStub(v);
            var arr = [];
            var names = [];
            if (stub.argCount) {
                arr.push(iter);
                names.push(store.createString(stub.argNames[0]));
            }
            
            vm.callFunction(v, arr, names);
            frame.valueStack.pop();
            frame.valueStack.pop();
            frame.valueStack.pop();
        };


        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_ASP] = function(frame, inst) {
            const count = inst.data;
            frame.pc += count;
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_QRY] = function(frame, inst) {
            const o = frame.valueStack[frame.valueStack.length-1];
            const output = store.valueQuery(o, inst.data);
            frame.valueStack.pop();
            frame.valueStack.push(output);
            if (valToType(output) == store.TYPE_OBJECT && output.function_stub != undefined) {
                frame.valueStack.push(o);
                frame.valueStack.push(vm_specialString_base);                    
            }
        };
        
        vm_opcodeSwitch[vm.opcodes.MATTE_OPCODE_OPR] = function(frame, inst) {
            switch(inst.data) {
              case vm_operator.MATTE_OPERATOR_ADD:
              case vm_operator.MATTE_OPERATOR_SUB:
              case vm_operator.MATTE_OPERATOR_DIV:
              case vm_operator.MATTE_OPERATOR_MULT:
              case vm_operator.MATTE_OPERATOR_BITWISE_OR:
              case vm_operator.MATTE_OPERATOR_OR:
              case vm_operator.MATTE_OPERATOR_BITWISE_AND:
              case vm_operator.MATTE_OPERATOR_AND:
              case vm_operator.MATTE_OPERATOR_SHIFT_LEFT:
              case vm_operator.MATTE_OPERATOR_SHIFT_RIGHT:
              case vm_operator.MATTE_OPERATOR_POW:
              case vm_operator.MATTE_OPERATOR_EQ:
              case vm_operator.MATTE_OPERATOR_POINT:
              case vm_operator.MATTE_OPERATOR_TERNARY:
              case vm_operator.MATTE_OPERATOR_GREATER:
              case vm_operator.MATTE_OPERATOR_LESS:
              case vm_operator.MATTE_OPERATOR_GREATEREQ:
              case vm_operator.MATTE_OPERATOR_LESSEQ:
              case vm_operator.MATTE_OPERATOR_TRANSFORM:
              case vm_operator.MATTE_OPERATOR_MODULO:
              case vm_operator.MATTE_OPERATOR_CARET:
              case vm_operator.MATTE_OPERATOR_TYPESPEC:
              case vm_operator.MATTE_OPERATOR_NOTEQ:
                if (frame.valueStack.length < 2) {
                    vm.raiseErrorString("VM error: OPR operator requires 2 operands");
                } else {
                    const a = frame.valueStack[frame.valueStack.length - 2];
                    const b = frame.valueStack[frame.valueStack.length - 1];
                    const v = vm_operator_2(inst.data, a, b);
                    frame.valueStack.pop();
                    frame.valueStack.pop();
                    frame.valueStack.push(v);
                }
                return;
                
                
              case vm_operator.MATTE_OPERATOR_NOT:
              case vm_operator.MATTE_OPERATOR_NEGATE:
              case vm_operator.MATTE_OPERATOR_BITWISE_NOT:
              case vm_operator.MATTE_OPERATOR_POUND:
                if (frame.valueStack.length < 1) {
                    vm.raiseErrorString("VM error: OPR operator requires 1 operand.");
                } else {
                    const a = frame.valueStack[frame.valueStack.length - 1];
                    const v = vm_operator_1(inst.data, a);
                    frame.valueStack.pop();
                    frame.valueStack.push(v);
                }
                return;
                
            
            }
        };
        
        
        ////////////////////////////////////// add builtin ext calls           
        vm_addBuiltIn(vm.EXT_CALL.NOOP, [], function(fn, args){return store.empty;});
        vm_addBuiltIn(vm.EXT_CALL.BREAKPOINT, [], function(fn, args){
            return store.empty;
        });
        
        
        const vm_extCall_foreverRestartCondition = function(frame, result) {
            return 1;
        };
        
        
        vm_addBuiltIn(vm.EXT_CALL.IMPORT, ['module'], function(fn, args) {
            return vm.import(store.valueAsString(args[0]), args[1]);
        });

        vm_addBuiltIn(vm.EXT_CALL.IMPORTMODULE, ['module', 'parameters'], function(fn, args) {
            return vm.import(store.valueAsString(args[0]), undefined);
        });
        
        vm_addBuiltIn(vm.EXT_CALL.PRINT, ['message'], function(fn, args) {
            onPrint(store.valueAsString(args[0]));
            return store.empty;
        });
        
        vm_addBuiltIn(vm.EXT_CALL.SEND, ['message'], function(fn, args) {
            vm_catchable = args[0];
            vm_pendingCatchable = 1;
            vm_pendingCatchableIsError = 0;
            return store.empty;                
        });
        
        vm_addBuiltIn(vm.EXT_CALL.ERROR, ['detail'], function(fn, args) {
            vm.raiseError(args[0]);
            return store.empty;
        });
        
        vm_addBuiltIn(vm.EXT_CALL.GETEXTERNALFUNCTION, ['name'], function(fn, args) {
            const str = store.valueAsString(args[0]);
            if (vm_pendingCatchable) {
                return store.empty;
            }
            
            const id = vm_externalFunctions[str];
            if (!id) {
                vm.raiseErrorString("getExternalFunction() was unable to find an external function of the name: " + str);
                return store.empty;
            }
            
            const out = store.createFunction(vm_extStubs[id]);
            delete vm_externalFunctions[str];
            return out;
        });
        
        
        //// Numbers 
        vm_addBuiltIn(vm.EXT_CALL.NUMBER_PI, [], function(fn, args) {
            return store.createNumber(Math.PI);
        });

        vm_addBuiltIn(vm.EXT_CALL.NUMBER_PARSE, ['string'], function(fn, args) {
            const aconv = store.valueAsString(args[0]);
            const p = Number.parseFloat(aconv);
            if (Number.isNaN(p)) {
                vm.raiseErrorString("Could not interpret String as a Number");
            } else {
                return store.createNumber(p);
            }
            return store.empty;
        });

        vm_addBuiltIn(vm.EXT_CALL.NUMBER_RANDOM, [], function(fn, args) {
            return store.createNumber(Math.random());
        });


        //// strings
        vm_addBuiltIn(vm.EXT_CALL.STRING_COMBINE, ["strings"], function(fn, args) {
            if (valToType(args[0]) != store.TYPE_OBJECT) {
                vm.raiseErrorString( "Expected Object as parameter for string combination. (The object should contain string values to combine).");
                return store.empty;
            }
            
            var index = store.createNumber(0);
            const len = store.valueObjectGetNumberKeyCount(args[0]);
            var str = "";
            for(; index < len; ++index) {
                str += store.valueAsString(store.valueObjectAccessIndex(args[0], index));
            }
            
            return store.createString(str);
        });
        
        
        
        
        //// Objects 
        vm_addBuiltIn(vm.EXT_CALL.OBJECT_NEWTYPE, ["name", "inherits", "layout"], function(fn, args) {
            return store.createType(args[0], args[1], args[2]);
        });
        
        vm_addBuiltIn(vm.EXT_CALL.OBJECT_INSTANTIATE, ["type"], function(fn, args) {
            return store.createObjectTyped(args[0]);
        });
        

        vm_addBuiltIn(vm.EXT_CALL.OBJECT_FREEZEGC, [], function(fn, args) {
            return store.empty;
        });

        vm_addBuiltIn(vm.EXT_CALL.OBJECT_THAWGC, [], function(fn, args) {
            return store.empty;

        });
        vm_addBuiltIn(vm.EXT_CALL.OBJECT_GARBAGECOLLECT, [], function(fn, args) {
            return store.empty;

        });




        /// query number 
        vm_addBuiltIn(vm.EXT_CALL.QUERY_ATAN2, ['base', 'y'], function(fn, args) {
            return store.createNumber(Math.atan2(store.valueAsNumber(args[0]), store.valueAsNumber(args[1]))); 
        });

        const ensureArgObject = function(args) {
            if (valToType(args[0]) != store.TYPE_OBJECT) {
                vm.raiseErrorString("Built-in Object function expects a value of type Object to work with.");
                return 0;
            }
            return 1;
        };


        /// query object
        vm_addBuiltIn(vm.EXT_CALL.QUERY_PUSH, ['base', 'value'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            const ind = store.createNumber(store.valueObjectGetNumberKeyCount(args[0]));
            store.valueObjectSet(args[0], ind, args[1], 1);
            return args[0];
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_SETSIZE, ['base', 'size'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;

            const object = args[0];

            if (object.kv_number == undefined)
                object.kv_number = [];

            const size = store.valueAsNumber(args[1])
            const oldSize = object.kv_number.length;
            
            if (size > oldSize) {
                for(var i = oldSize; i < size; ++i) {
                    object.kv_number.push(store.empty);
                }
            } else if (size == oldSize) {
                // again, whyd you do that!
            } else {
                object.kv_number.splice(size, oldSize-size)
            }
            return store.empty;
        });


        vm_addBuiltIn(vm.EXT_CALL.QUERY_INSERT, ['base', 'at', 'value'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            store.valueObjectInsert(args[0], store.valueAsNumber(args[1]), args[2]);
            return args[0];
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_REMOVE, ['base', 'key'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            store.valueObjectRemoveKey(args[0], args[1])
            return store.empty;
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_SETATTRIBUTES, ['base', 'attributes'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            if (valToType(args[1]) != store.TYPE_OBJECT) {
                vm.raiseErrorString("'setAttributes' requires an Object to be the 'attributes'");
                return store.empty;
            }
            store.valueObjectSetAttributes(args[0], args[1]);
            return store.empty;
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_SORT, ['base', 'comparator'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            if (valToType(args[1]) == store.TYPE_OBJECT && store.function_stub) {
                vm.raiseErrorString("A function comparator is required for sorting.");
                return store.empty;
            }
            store.valueObjectSortUnsafe(args[0], args[1]);
            return store.empty;
        });
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_SUBSET, ['base', 'from', 'to'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            return store.valueSubset(
                args[0],
                store.valueAsNumber(args[1]), 
                store.valueAsNumber(args[2])
            )
        });            
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_FILTER, ['base', 'by'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            
            var ctout = 0;
            const len = store.valueObjectGetNumberKeyCount(args[0]);
            const out = store.createObject();
            const names = [vm_specialString_value];
            const vals = [0];
            for(var i = 0; i < len; ++i) {
                if (vm_pendingCatchable) break;
                const v = store.valueObjectAccessIndex(args[0], i);
                vals[0] = v;

                if (store.valueAsBoolean(vm.callFunction(args[1], vals, names))) {
                    store.valueObjectInsert(out, ctout++, v);
                }
            }
            return out;
        });            


        vm_addBuiltIn(vm.EXT_CALL.QUERY_FINDINDEX, ['base', 'value'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;

            const len = store.valueObjectGetNumberKeyCount(args[0]);

            for(var i = 0; i < len; ++i) {
                if (vm_pendingCatchable) break;
                const v = store.valueObjectAccessIndex(args[0], i);
                if (store.valueAsBoolean(vm_operatorFunc[vm_operator.MATTE_OPERATOR_EQ](v, args[1]))) {
                    return store.createNumber(i);
                }
            }

            return store.createNumber(-1);
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_FINDINDEXCONDITION, ['base', 'query'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;

            const len = store.valueObjectGetNumberKeyCount(args[0]);

            const names = [vm_specialString_value];
            const vals = [];
            if (!store.valueIsCallable(args[1])) {
                vm.raiseErrorString("When specified, the query parameter for findIndexCondition() must be callable.");
            } else {
                for(var i = 0; i < len; ++i) {
                
                    if (vm_pendingCatchable) break;
                    const v = store.valueObjectAccessIndex(args[0], i);
                    vals[0] = v;

                    if (store.valueAsBoolean(vm.callFunction(args[1], vals, names))) {
                        return store.createNumber(i);
                    }                        
                }
            }                

            return store.createNumber(-1);
        });

        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_ISA, ['base', 'type'], function(fn, args) {
            return store.createBoolean(store.valueIsA(args[0], args[1]));
        });            
        
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_MAP, ['base', 'to'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            const names = [vm_specialString_value];
            const vals = [0];
            const object = store.createObject();
            object.kv_number = [];
            
            const len = store.valueObjectGetNumberKeyCount(args[0]);
            for(var i = 0; i < len; ++i) {
                if (vm_pendingCatchable) break;
                const v = store.valueObjectAccessIndex(args[0], i);
                vals[0] = v;
                const newv = vm.callFunction(args[1], vals, names);
                store.valueObjectSetIndexUnsafe(object, i, newv);
            }
            return object;
        });            
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_REDUCE, ['base', 'to'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            const names = [
                vm_specialString_previous,
                vm_specialString_value
            ];
            const vals = [
                0, 0
            ];
            var out = store.empty;
            const len = store.valueObjectGetNumberKeyCount(args[0]);
            for(var i = 0; i < len; ++i) {
                vals[0] = out;
                vals[1] = store.valueObjectAccessIndex(args[0], i);
                
                out = vm.callFunction(args[1], vals, names);
            }
            return out;            
        });
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_ANY, ['base', 'condition'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            const names = [
                vm_specialString_value
            ];
            
            const vals = [0];
            const len = store.valueObjectGetNumberKeyCount(args[0]);
            for(var i = 0; i < len; ++i) {
                vals[0] = store.valueObjectAccessIndex(args[0], i);
                const newv = vm.callFunction(args[1], vals, names);
                if (store.valueAsBoolean(newv)) {
                    return store.createBoolean(true);
                }
            }
            return store.createBoolean(false);
        });            

        vm_addBuiltIn(vm.EXT_CALL.QUERY_ALL, ['base', 'condition'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            const names = [
                vm_specialString_value
            ];
            
            const vals = [0];
            const len = store.valueObjectGetNumberKeyCount(args[0]);
            for(var i = 0; i < len; ++i) {
                vals[0] = store.valueObjectAccessIndex(args[0], i);
                const newv = vm.callFunction(args[1], vals, names);
                if (!store.valueAsBoolean(newv)) {
                    return store.createBoolean(false);
                }
            }
            return store.createBoolean(true);
        }); 

        const vm_extCall_forRestartCondition_up = function(frame, res, data) {
            const d = data;
            if (res != undefined) {
                d.i = store.valueAsNumber(res);
            } else {
                d.i += d.offset;
            }
            
            if (d.usesi) {
                store.valueObjectSetIndexUnsafe(
                    frame.referrable,
                    0,
                    store.createNumber(d.i)
                );
            }
            return d.i < d.end;
        };

        const vm_extCall_forRestartCondition_down = function(frame, res, data) {
            const d = data;
            if (res != undefined) {
                d.i = store.valueAsNumber(res);
            } else {
                d.i += d.offset;
            }
            
            if (d.usesi) {
                store.valueObjectSetIndexUnsafe(
                    frame.referrable,
                    0,
                    store.createNumber(d.i)
                );
            }
            return d.i > d.end;
        };


        vm_addBuiltIn(vm.EXT_CALL.QUERY_FOREACH, ['base', 'do'], function(fn, args) {
            if (!ensureArgObject(args)) return store.empty;
            if (!store.valueIsCallable(args[1])) {
                vm.raiseErrorString("'foreach' requires the first argument to be a function.");
                return store.empty;
            }
            store.valueObjectForeach(args[0], args[1]);
            return args[0];
        });            
        
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_SETISINTERFACE, ['base', 'enabled', 'private'], function(fn, args) {
            const which = store.valueAsBoolean(args[1]);
            if (vm_pendingCatchable) return store.empty;
            store.valueObjectSetIsInterface(args[0], which, args[2]);
            return store.empty;
        });                        
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_SETISRECORD, ['base', 'enabled'], function(fn, args) {
            const which = store.valueAsBoolean(args[1]);
            if (vm_pendingCatchable) return store.empty;
            store.valueObjectSetIsRecord(args[0], which);
            return store.empty;
        });                        
        

        const ensureArgString = function(args) {
            if (valToType(args[0]) != store.TYPE_STRING) {
                vm.raiseErrorString("Built-in String function expects a value of type String to work with.");
                return 0;
            }
            return 1;
        };


        /// query object
        
        
        vm_addBuiltIn(vm.EXT_CALL.QUERY_SEARCH, ['base', 'key'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const str2 = store.valueAsString(args[1]);
            if (str2 == '') return store.createNumber(-1);
            return store.createNumber(args[0].indexOf(str2));
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_CONTAINS, ['base', 'key'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const str2 = store.valueAsString(args[1]);
            if (str2 == '') return store.createBoolean(false);
            return store.createBoolean(args[0].indexOf(str2) != -1);
        });


        vm_addBuiltIn(vm.EXT_CALL.QUERY_SEARCH_ALL, ['base', 'key'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const str2 = store.valueAsString(args[1]);
            
            const outArr = [];
            var str = args[0];
            var at = 0;
            while(true) {
                const index = str.indexOf(str2);
                if (index == -1) {
                    break;
                }
                outArr.push(store.createNumber(at+index));
                at += index+str2.length;
                str = str.substring(index+str2.length, str.length);
            }
            
            return store.createObjectArray(outArr);
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_REPLACE, ['base', 'key', 'with', 'keys'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const str2 = store.valueAsString(args[1]);    
            const str3 = store.valueAsString(args[2]);    
            const strs = (args[3]);
            
            
            if (str2 != undefined) {      
                return store.createString(args[0].replaceAll(str2, str3));
            } else {
                var strOut = args[0];
                const len = store.valueObjectGetNumberKeyCount(strs);
                for(var i = 0; i < len; ++i) {
                    strOut = strOut.replaceAll(store.valueAsString(store.valueObjectAccessIndex(strs, i)), str3);                
                };
                return store.createString(strOut);
            }
        });


        vm_addBuiltIn(vm.EXT_CALL.QUERY_FORMAT, ['base', 'items'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const items = args[1];
            
            if (valToType(items) != store.TYPE_OBJECT) {
                vm.raiseErrorString("items array input must be an object.");
                return 0;                    
            }
            

            const len = store.valueObjectGetNumberKeyCount(items);
            var input = args[0];
            var output = '';
            var iter = 0;

            for(var i = 0; i < input.length; ++i) {
                if (input[i] == '%') {
                    if (iter == input.length-1) {
                        // incomplete sequence
                        output += input[i]
                    } else {
                        if (input[i+1].search(/[0-9]/) != -1) {
                            i+=1;
                            // extract numbers 
                            var number = '';
                            for(; i < input.length; ++i) {
                                if (input[i].search(/[0-9]/) != -1) 
                                    number += input[i];
                                else 
                                    break;
                            }
                            if (i != input.length-1)
                                i--; // backup for next.
                            const index = Number.parseInt(number);
                            if (index >= 0 && index < len) {
                                output += store.valueAsString(store.valueObjectAccessIndex(items, index))
                            }
                        } else {
                            output += input[i];                            
                            if (input[i+1] == '%')
                                i += 1; // escaped;
                        }
                    }
                } else {
                    output += input[i];
                }
            }

            return store.createString(output);
        });


        vm_addBuiltIn(vm.EXT_CALL.QUERY_COUNT, ['base', 'key'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const str2 = store.valueAsString(args[1]);    

            var str = args[0];
            var count = 0;
            while(true) {
                if (str.indexOf(str2) == -1)
                    break;
                    
                count ++;
                str = str.replace(str2, '');
            }           
            return store.createNumber(count); 
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_CHARCODEAT, ['base', 'index'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const index = store.valueAsNumber(args[1]);
            return store.createNumber(args[0].charCodeAt(index));
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_CHARAT, ['base', 'index'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const index = store.valueAsNumber(args[1]);
            return store.createString(args[0].charAt(index));
        });


        vm_addBuiltIn(vm.EXT_CALL.QUERY_SETCHARCODEAT, ['base', 'index', 'value'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const index = store.valueAsNumber(args[1]);
            const val = store.valueAsNumber(args[2]);
            
            var str = args[0];
            return store.createString(str.substring(0, index) + String.fromCharCode(val) + str.substring(index + 1));                
        });


        vm_addBuiltIn(vm.EXT_CALL.QUERY_SETCHARAT, ['base', 'index', 'value'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const index = store.valueAsNumber(args[1]);
            const val = store.valueAsString(args[2]);
            
            var str = args[0];
            return store.createString(str.substring(0, index) + val + str.substring(index + 1));                
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_REMOVECHAR, ['base', 'index'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const index = store.valueAsNumber(args[1]);
            
            var str = args[0];
            return store.createString(str.substring(0, index) + str.substring(index + 1));                
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_SUBSTR, ['base', 'from', 'to'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const from = store.valueAsNumber(args[1]);
            const to   = store.valueAsNumber(args[2]);

            var str = args[0];
            return store.createString(str.substring(from, to+1));                
        });

        vm_addBuiltIn(vm.EXT_CALL.QUERY_SPLIT, ['base', 'token'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;
            const token = store.valueAsString(args[1]);
            const results = args[0].split(token);
            for(var i = 0; i < results.length; ++i) {
                results[i] = store.createString(results[i]);
            }
            return store.createObjectArray(results);
        });                

        vm_addBuiltIn(vm.EXT_CALL.QUERY_SCAN, ['base', 'format'], function(fn, args) {
            if (!ensureArgString(args)) return store.empty;

            // this is not a good method.
            var exp = store.valueAsString(args[1]);
            exp = exp.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
            exp = exp.replaceAll('\\[%\\]', '(.*)');
            const strs = args[0].match(new RegExp(exp, "i"));
            
            if (strs == null || strs.length == 0) {
                return store.createObject();
            }
            
            const arr = [];
            for(var i = 1; i < strs.length; ++i) {
                arr.push(store.createString(strs[i]));
            }
            return store.createObjectArray(arr);
        });
        
        
        
        return vm;
    }());
    ///
    
    Matte.objectToJSON = function(obj) {
        const valToType = store.valToType;
        
        const encode = function(val) {
            switch(valToType(val)) {
              case store.TYPE_NUMBER: return val; break;
              case store.TYPE_STRING: return val; break;
              case store.TYPE_BOOLEAN: return val; break;
              case store.TYPE_EMPTY: return null;
              case store.TYPE_OBJECT: return (function(){
                if (store.valueObjectGetKeyCount(val) != store.valueObjectGetNumberKeyCount(val)) {
                    const keys = store.valueObjectKeys(val);
                    const vals = store.valueObjectValues(val);

                    var out = {};     
                    const len = store.valueObjectGetNumberKeyCount(keys);                   
                    for(var i = 0; i < len; ++i) {
                        const key = store.valueObjectAccessIndex(keys, i);
                        const value = store.valueObjectAccessIndex(vals, i);
                        if (valToType(key) == store.TYPE_OBJECT)
                            continue;
                        out[encode(key)] = encode(value);
                    }
                    return out;
                // simple array
                } else {
                    var out = [];                        
                    const vals = store.valueObjectValues(val);
                    const len = store.valueObjectGetNumberKeyCount(vals);                   
                    for(var i = 0; i < len; ++i) {
                        out.push(encode(store.valueObjectAccessIndex(vals, i)));
                    }                    
                    return out;
                }
              })()
            }
        };
        const str = JSON.stringify(encode(obj));
        return str;
    }
    
    Matte.JSONtoObject = function(str) {
        const object = JSON.parse(str);
        const valToType = store.valToType;
        
        const decode = function(js) {
            switch(typeof js) {
              case 'number':
                return store.createNumber(js);
                
              case 'string':
                return store.createString(js);
                
              case 'boolean':
                return store.createBoolean(js);
                
              case 'undefined':
                return store.empty;
                
              case 'object': return (function() {
                if (js == null)
                    return store.empty;
                
                if (Array.isArray(js)) {
                    const vals = [];
                    for(var i = 0; i < js.length; ++i) {
                        const val = decode(js[i]);
                        vals.push(val);
                    }                        
                    return store.createObjectArray(vals);
                } else {
                
                    
                    const out = store.createObject();
                    const keys = Object.keys(js);
                    
                    for(var i = 0; i < keys.length; ++i) {
                        const key = decode(keys[i]);
                        const val = decode(js[keys[i]]);
                        store.valueObjectSet(out, key, val);
                    }
                    return out;
                }
              })()
              default: throw new Error('Unknown type!');
            }
        };
        return decode(object);
    }
    
    
    vm.setExternalFunction('__matte_::json_encode', ['a'], function(fn, args) {
        return Matte.objectToJSON(args[0]);
    });
    vm.setExternalFunction('__matte_::json_decode', ['a'], function(fn, args) {
        return Matte.JSONtoObject(args[0]);
    });        

    return vm;        
}
