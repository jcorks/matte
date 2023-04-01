'use strict';

const Matte = {
    newVM : function(
        onImport, // function 
    ) {
        if (onImport == undefined) 
            throw new Error("onImport MUST be defined");
    
        // matte_bytecode_stub.c
        const bytecode = {
            // converts bytecode into stub objects.
            // bytecode should be an ArrayBuffer.
            stubsFromBytecode : function(fileID, bytecode) {
                // simple byte iterator matching what is done in C
                const bytes = {
                    iter : 0,
                    chompUInt8 : function() {
                        const out = UInt8Array(bytecode.slice(bytes.iter, bytes.iter+1)[0];
                        iter += 1;
                        return out;
                    },
                    chompUInt16 : function() {
                        const out = UInt16Array(bytecode.slice(iter, iter+2)[0];
                        iter += 2;
                        return out;
                    },
                    
                    chompUInt32 : function() {
                        const out = UInt32Array(bytecode.slice(iter, iter+4)[0];
                        iter += 4;
                        return out;
                    },
                    chompInt32 : function() {
                        const out = Int32Array(bytecode.slice(iter, iter+4)[0];
                        iter += 4;
                        return out;
                    },
                    chompDouble : function() {
                        const out = Float64Array(bytecode.slice(iter, iter+8)[0];
                        iter += 4;
                        return out;
                    },
                    
                    chompString : function() {
                        const size = bytes.chompUInt32();
                        var str = '';
                        // inefficient.
                        for(var i = 0; i < size; ++i) {
                            str = str + String.fromCharCode(bytes.chompInt32());
                        }
                        return str;
                    }
                    
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
                    stub.argCount = bytes.chompUInt8();
                    stub.argNames = [];
                    for(var i = 0; i < stub.argCount; ++i) {
                        stub.argNames[i] = bytes.chompString();
                    }
                    
                    stub.localCount = bytes.chompUInt8();
                    stub.localNames = [];
                    for(var i = 0; i < stub.argCount; ++i) {
                        stub.localNames[i] = bytes.chompString();
                    }

                    stub.stringCount = bytes.chompUInt32();
                    stub.strings = [];
                    for(var i = 0; i < stub.argCount; ++i) {
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
                    for(var i = 0; i < stub.instructionCount; ++i) {
                        stub.instruction[i] = {
                            lineNumber : bytes.chompUInt32(),
                            opcode     : bytes.chompUInt8(),
                            data       : bytes.chompDouble(),
                        };
                        
                        if (stub.instruction[n].opcode == MATTE_OPCODE_NFN)
                            stub.instruction[n].nfnFileID = fileID;
                    }
                    stub.endByte = bytes.iter;


                    return stub;
                }
                
                
                const stubs = [];
                while(bytes.iter < bytecode.lengthBytes) {
                    stubs.push(createStub());
                }
                return stubs;
            }
        };
    
        // matte_heap.c
        
        const heap = function(){
            const TYPE = {
                EMPTY: 0,
                BOOLEAN : 1,
                NUMBER: 2,
                STRING: 3,
                OBJECT: 4,
                TYPE : 5
            };
            
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
                SEARCH_ALL : 22,
                CONTAINS : 23,
                REPLACE : 24,
                COUNT : 25,
                CHARCODEAT : 26,
                CHARAT : 27,
                SETCHARCODEAT : 28,
                SETCHARAT : 29,
                
                KEYCOUNT : 30,
                KEYS : 31,
                VALUES : 32,
                PUSH : 33,
                POP : 34,
                INSERT : 35,
                REMOVE : 36,
                SETATTRIBUTES : 37,
                ATTRIBUTES : 38,
                SORT : 39,
                SUBSET : 40,
                FILTER : 41,
                FINDINDEX : 42,
                ISA : 43,
                MAP : 44,
                REDUCE : 45,
                ANY : 46,
                ALL : 47,
                FOR : 48,
                FOREACH : 49
            };
            
            var typecode_id_pool = 10;
            const TYPECODES = {
                OBJECT : 1
            }
        
            const createValue() {
                return {
                    binID : TYPE.EMPTY
                    data : null
                }
            };

            const createObject() {
                return {
                    binID : TYPE.OBJECT
                    data : {
                        typecode : TYPECODES.OBJECT
                    }
                }
            };
            
            const objectPutProp = function(object, key, value) {
                switch(key.binID) {
                  case TYPE.STRING:
                    if (object.data.kv_string = undefined)
                        object.data.kv_string = {};
                    object.data.kv_string[key.data] = value;
                    break;
                    
                  case TYPE.NUMBER:
                    if (object.data.kv_number = undefined)
                        object.data.kv_number = [];
                        
                    if (key.value.number > object.data.kv_number.length) {
                        const index = Math.floor(key.value.number);
                        
                        for(i = object.data.kv_number.length; i < index+1; ++i) {
                            object.data.kv_number[i] = heap.createEmpty();                        
                        }
                    }
                    
                    object.data.kv_number[Math.floor(key.value.number)] = value;
                    break;
                  case TYPE.BOOLEAN:
                    if (key.data == true) {
                        object.data.kv_true = value;
                    } else {
                        object.data.kv_false = value;                    
                    } 
                    break;
                  case TYPE.OBJECT:
                    object.data.kv_object[key] = value;
                    break;
                    
                  case TYPE.TYPE:
                    throw new Error("TODO");
                    break;
                }
            }
            
            

            const objectGetConvOperator = function(m, type) {
                out = heap.valueObjectAccess(m.data.table_attribSet, type, 0);
            };

            const objectGetAccessOperator = function(object, isBracket, read) {
                var out = createValue();
                if (object.data.table_attribSet == undefined)
                    return out;
                
                const set = heap.valueObjectAccess(object.data.table_attribSet, isBracket ? heap_specialString_bracketAccess : heap_specialString_dotAccess, 0);
                if (set.binID == TYPE.OBJECT) {
                    vm.raiseErrorString("operator['[]'] and operator['.'] property must be an Object if it is set.");
                } else {
                    out = heap.valueObjectAccess(set, read ? heap_specialString_get : heap_specailString_set, 0);
                }
                return out;
            };


            // either returns lookup value or undefined if no such key
            const objectLookup = function(object, key) {
                switch(key.binID) {
                  case TYPE.EMPTY: return undefined;

                  case TYPE.STRING:
                    if (object.data.kv_string == undefined) 
                        return undefined;
                    return object.data.kv_string[key.data];

                  case TYPE.NUMBER:
                    if (key.data < 0) return undefined;
                    if (object.data.kv_number == undefined)
                        return undefined;
                    if (key.data >= object.data.kv_number.length)
                        return undefined;
                    return object.data.kv_number[Math.floor(key.data)];
                   
                  case TYPE.BOOLEAN:
                    if (key.data == true) {
                        return object.data.kv_true;
                    } else {
                        return object.data.kv_true;                    
                    }
                  case TYPE.OBJECT:
                    if (object.data.kv_object == undefined)
                        return undefined;
                    return object.data.kv_object[object];
                  case TYPE.TYPE:
                    if (!object.data.kv_types)
                        return undefined;
                    return object.data.kv_types[key];
                  
                }
            };

            const heap_lock = {};
            var heap_typepool = 0;
            const heap_typecode2data = [[]];
            const heap_type_number_methods = {
                PI : vm.EXT_CALL.NUMBER_PI,
                parse : vm.EXT_CALL.NUMBER_PARSE,
                random : vm.EXT_CALL.NUMBER_RANDOM
            };
            const heap_type_object_methods = {
                newType : vm.EXT_CALL.OBJECT_NEWTYPE,
                instantiate : vm.EXT_CALL.OBJECT_INSTANTIATE,
                freezeGC : vm.EXT_CALL.OBJECT_FREEZEGC,
                thawGC : vm.EXT_CALL.OBJECT_THAWGC,
                garbageCollect : vm.EXT_CALL.OBJECT_GARBAGECOLLECT
            };
            const heap_type_string_methods = {
                combine : vm.EXT_CALL.STRING_COMBINE
            };
            const heap_typecode2data = [];
            const heap_queryTable = {};
            const heap = {
                TYPE : TYPE,
                createEmpty : function() {
                    return createValue();
                },
                
                createNumber : function(val) {
                    const out = createValue();
                    out.binID = TYPE.NUMBER;
                    out.data = val;
                    return out
                },
                
                createBoolean : function(val) {
                    const out = createValue();
                    out.binID = TYPE.BOOLEAN;
                    out.data = val;
                    return out;
                },
                
                createString : function(val) {
                    const out = createValue();
                    out.binID = TYPE.STRING;
                    out.data = val;
                    return out;
                },
                
                createObject : function() {
                    return createObject();
                },
                
                createObjectTyped : function(type) {
                    const out = createObject();
                    if (type.binID != TYPE.TYPE) {
                        vm.raiseErrorString("Cannot instantiate object without a Type. (given value is of type " + heap.valueTypeName(heap.valueGetType(type)) + ')');
                        return out;
                    }
                    out.data.typecode = type;
                    return out;
                },
                
                createType : function(name, inherits) {
                    const out = createValue();
                    out.binID = TYPE.TYPE;
                    if (heap_typepool == 0xffffffff) {
                        vm.raiseErrorString('Type count limit reached. No more types can be created.');
                        return createValue();
                    } else {
                        out.data = {
                            id : ++heap_typepool
                        };
                    }
                    
                    if (name && name.binID) {
                        out.data.name = heap.valueAsString(name);
                    } else {
                        out.data.name = heap.createString("<anonymous type " + out.data.id + ">");
                    }

                    if (inherits && inherits.binID) {
                        if (inherits.binID != TYPE.OBJECT) {
                            vm.raiseErrorString("'inherits' attribute must be an object.");
                            return createValue();
                        }                    
                        out.data.isa = typeArrayToIsA(inherits);
                    }
                    heap_typecode2data.push(out);
                    
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
                    out.data.kv_number = [];
                    for(var i = 0; i < values.length; ++i) {
                        out.data.kv_number[i] = valueArray[i];
                    }
                    return out;
                },
                
                createFunction : function(stub) {
                    const out = createObject();
                    out.data.function_stub = stub;
                    out.data.function_captures = [];


                    const captures = stub.captures;
                    // always preserve origin
                    if (vm.stackframe.length > 0) {
                        const frame = vm.stackframe[0];
                        out.data.function_origin = frame.context;
                        out.data.function_origin_referrable = frame.referrable;
                    }
                    
                    if (captures.length == 0) return out;


                    if (vm.stackframe.length == 0)
                        throw new Error('This shouldnt happen.');
                                            
                    const frame = vm.stackframe[0];
                    for(i = 0; i < captures.length; ++i) {
                        var context = frame.context;
                        var contextReferrable = frame.referrable;
                        
                        while(context.binID != 0) {
                            const origin = context;
                            if (stub.stubID == captures[i].stubID) {
                                const ref = {};
                                ref.referrableSrc = contextReferrable;
                                ref.index = captures[i].referrable;
                                
                                out.data.function_captures.push(ref);
                            }
                            context = origin.data.function_origin;
                            contextReferrable = origin.data.function_origin_referrable;
                        }
                        
                        if (context.binID == 0) {
                            vm.raiseErrorString("Could not find captured variable!");
                            out.data.function_captures.push({referrableSrc:createValue(), index:0});
                        }
                    }
                },
                
                createExternalFunction : function(stub) {
                    return heap.createFunction(stub);
                },
                
                createTypedFunction : function(stub, args) {
                    throw new Error('TODO');
                },
                
                valueObjectPushLock : function(value) {
                    if (heap_lock[value] != undefined) {
                        heap_lock[value]++;
                    } else {
                        heap_lock[value] = 1;                    
                    }
                },
                
                valueObjectPopLock : function(value) {
                    if (heap_lock[value] == undefined)
                        return;
                    heap_lock[value]--;
                    if (heap_lock[value] <= 0)
                        delete heap_lock[value];
                },
                
                valueObjectArrayAtUnsafe : function(value, index) {
                    return value.data.kv_number[index];
                },
                
                valueStringGetStringUnsafe : function(value, index) {
                    return value.data;
                },
                
                valueFrameGetNamedReferrable : function(frame, value) {
                    throw new Error('TODO'); // or not. This VM does not do debugging.
                },
                
                valueSetCapturedValue : function(value, index, capt) {
                    if (index >= value.data.function_captures.length) return;
                    const ref = value.data.function_captures[index];
                    heap.valueObjectSet(
                        referrable.referrableSrc, 
                        heap.createNumber(referrable.index),
                        capt 
                    );
                },
                
                valueGetBytecodeStub : function(value) {
                    if (value.binID == TYPE.OBJECT && value.data.stub != undefined)
                        return value.data.stub;
                },
                
                valueGetCapturedValue : function(value, index) {
                    if (value.binID != TYPE.OBJECT || value.data.function_stub == undefined) return;
                    if (index >= value.data.function_captures.length) return;
                    return heap.valueObjectArrayAtUnsafe(
                        value.data.function_captures[index].referrableSrc,
                        value.data.function_captures[index].index
                    );
                },
                
                valueSubset : function(value, from, to) {
                    if (value.binID == TYPE.OBJECT) {
                        const curlen = value.data.kv_number ? value.data.kv_number.length : 0;
                        if (from >= curlen || to >= curlen) return createValue();
                        
                        return heap.createObjectArray(value.data.kv_number.slice(from, to+1));
                    } else if (value.binID == TYPE.STRING) {
                        
                        const curlen = value.data.length;;
                        if (from >= curlen || to >= curlen) return createValue();
                        return heap.createString(
                            value.data.substr(from, to+1)
                        );
                    }
                },
                
                // valueObjectSetUserData
                // valueObjectGetUserData
                // valueObjectSetNativeFinalizer are not implemented.
                
                valueIsEmpty : function(value) {
                    return value.binID == TYPE.EMPTY
                },
                
                valueAsNumber : function(value) {
                    switch(value.binID) {
                      case TYPE.EMPTY:
                        vm.raiseErrorString('Cannot convert empty into a number');
                        return 0;

                      case TYPE.TYPE:
                        vm.raiseErrorString('Cannot convert type value into a number');
                        return 0;

                      case TYPE.NUMBER:
                        return value.data;
                        
                      case TYPE.BOOLEAN:
                        return value.data == true ? 1 : 0;
                        
                      case TYPE.STRING:
                        vm.raiseErrorString('Cannot convert string value into a number');
                        return 0;
                      case TYPE.OBJECT:
                        throw new Error('TODO');
                        
                    }
                },
                
                valueAsString : function(value) {
                    switch(value.binID) {
                      case TYPE.EMPTY:
                        return heap_specialString_empty;

                      case TYPE.TYPE:
                        return heap.valueTypeName(value);

                      case TYPE.NUMBER:
                        return heap.createString(Number.parse(value.data));

                      case TYPE.BOOLEAN:
                        return value.data == true ? heap_specialString_true : heap_specialString_false;
                        
                      case TYPE.STRING:
                        return value;
                        
                      case TYPE.OBJECT:
                        if (value.data.function_stub != undefined) {
                            vm.raiseErrorString("Cannot convert function into a string.");
                            return heap_specialString_empty;
                        }
                        throw new Error("TODO");
                            
                    }
                    // shouldnt get here.
                    return heap_specialString_empty;
                },
                
                
                valueAsBoolean : function(value) {
                    switch(value.binID) {
                      case TYPE.EMPTY:   return 0;
                      case TYPE.TYPE:    return 1;
                      case TYPE.NUMBER:  return value.data != 0;
                      case TYPE.BOOLEAN: return value.data;
                      case TYPE.STRING:  return 1;
                      case TYPE.OBJECT:
                        if (value.data.function_stub != undefined)
                            return 1;
                            
                        throw new Error("TODO");
                        
                        
                      break;
                    }
                    return 0;
                },
                
                valueToType : function(value, type) {
                    throw new Error("TODO");
                },
                
                valueIsCallable : function(value) {
                    if (value.data.function_stub == undefined) return 0;
                    if (value.data.function_types == undefined) return 1;
                    return 2; // type-strict
                },
                
                valueObjectFunctionPreTypeCheckUnsafe : function(func, args) {
                    throw new Error("TODO");
                },
                
                valueObjectFunctionPostTypeCheckUnsafe : function(func, args) {
                    throw new Error("TODO");
                },
                
                valueObjectAccess : function(value, key, isBracket) [
                    switch(value.binID) {
                      case TYPE.TYPE:
                        return valueObjectAccessDirect(value, key, isBracket);

                      case TYPE.OBJECT:
                        if (value.data.function_stub != undefined) {
                            vm.raiseErrorString("Functions do not have members.");
                            return createValue();
                        }
                        const accessor = objectGetAccessOperator(value, isBracket, 1);
                        if (accessor.binID) {
                            return vm.callFunction(accessor, [key], [heap_specialString_key]);
                        } else {
                            return objectLookup(value, key);
                        }
                      default:
                        vm.raiseErrorString("Cannot access member of a non-object type. (Given value is of type: " + heap.valueTypeName(heap.valueGetType(value)).data + ")");
                        return createValue();
                    }
                },
                
                // will return undefined if no such access
                valueObjectAccessDirect : function(value, key, isBracket) {
                    switch(value.binID) {
                      case TYPE.TYPE:
                        if (isBracket) {
                            vm.raiseErrorString("Types can only yield access to built-in functions through the dot '.' accessor.");
                            return;
                        }
                        
                        if (key.binID != TYPE.STRING) {
                            vm.raiseErrorString("Types can only yield access to built-in functions through the string keys.");
                            return;
                        }
                        
                        var base;
                        switch(value.data.id) {
                          case 3: base = heap_type_number_methods; break;
                          case 4: base = heap_type_string_methods; break;
                          case 5: base = heap_type_object_methods; break;
                        }
                        if (base == undefined) {
                            vm.raiseErrorString("The given type has no built-in functions. (Given value is of type: " + 
                            return;
                        }
                        const out = base[key.data];
                        if (out == undefined) {
                            vm.raiseErrorString("The given type has no built-in function by the name '" + key.data + "'");
                            return;
                        }
                        return vm.externalBuiltinFunctionAsValue(out);
                    }
                },
                
                
                valueObjectAccessString: function(value, plainString) {
                    return heap.valueObjectAccess(
                        value,
                        heap.createString(plainString),
                        0
                    );
                },
                
                valueObjectAccessIndex : function(value, index) {
                    return heap.valueObjectAccess(
                        value,
                        heap.createNumber(index),
                        1
                    );                
                },
                
                valueObjectKeys : function(value) {
                    if (value.data.table_attribSet != undefined) {
                        const set = heap.valueObjectAccess(value.data.table_attribSet, heap_specialString_keys, 0);
                        if (set && set.binID)
                            return heap.valueObjectKeys(set);
                    }
                    const out = [];
                    if (value.data.kv_number) {
                        const len = value.data.kv_number.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(heap.createNumber(i));
                        }
                    }
                                    
                    if (value.data.kv_string) {
                        const keys = value.data.kv_string.keys();
                        const len = keys.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(heap.createString(keys[i]));
                        }
                    }

                    if (value.data.kv_object) {
                        const keys = value.data.kv_object.keys();
                        const len = keys.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(keys[i]); // OK, object 
                        }
                    }
                    
                    if (value.data.kv_true) {
                        out.push(heap.createBoolean(true);
                    }

                    if (value.data.kv_false) {
                        out.push(heap.createBoolean(false);
                    }


                    if (value.data.kv_types) {
                        const keys = value.data.kv_types.keys();
                        const len = keys.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(keys[i]); // OK, type
                        }
                    }
                    return out;                
                },
                
                valueObjectValues : function(value) {
                    if (value.data.table_attribSet != undefined) {
                        const set = heap.valueObjectAccess(value.data.table_attribSet, heap_specialString_values, 0);
                        if (set && set.binID)
                            return heap.valueObjectKeys(set);
                    }
                    const out = [];
                    if (value.data.kv_number) {
                        const len = value.data.kv_number.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(value.data.kv_number[i]);
                        }
                    }
                                    
                    if (value.data.kv_string) {
                        const values = value.data.kv_string.values();
                        const len = values.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(values[i]);
                        }
                    }

                    if (value.data.kv_object) {
                        const values = value.data.kv_object.values();
                        const len = values.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(values[i]); // OK, object 
                        }
                    }
                    
                    if (value.data.kv_true) {
                        out.push(heap.createBoolean(value.data.kv_true);
                    }

                    if (value.data.kv_false) {
                        out.push(heap.createBoolean(value.data.kv_false);
                    }


                    if (value.data.kv_types) {
                        const values = value.data.kv_types.values();
                        const len = keys.length;
                        for(var i = 0; i < len; ++i) {
                            out.push(values[i]); // OK, type
                        }
                    }
                    return out;                  
                },
                
                
                valueObjectGetKeyCount : function(value) {
                    if (value.binID != TYPE.OBJECT) return 0;
                    var out = 0;
                    if (value.data.kv_number) {
                        out += value.data.kv_number.length;
                    }
                                    
                    if (value.data.kv_string) {
                        const keys = value.data.kv_string.keys();
                        out += keys.length;
                    }

                    if (value.data.kv_object) {
                        const keys = value.data.kv_object.keys();
                        out +=  keys.length;
                    }
                    
                    if (value.data.kv_true) {
                        out += 1;
                    }

                    if (value.data.kv_false) {
                        out += 1;
                    }


                    if (value.data.kv_types) {
                        const keys = value.data.kv_types.keys();
                        out += keys.length;
                    }
                    return out;                     
                },
                
                valueObjectGetNumberKeyCount : function(value) {
                    if (value.binID != TYPE.OBJECT) return 0;
                    return value.data.kv_number.length;                    
                },
                
                valueObjectInsert : function(value, plainIndex, v) {
                    if (value.binID != TYPE.OBJECT) return;
                    if (value.data.kv_number == undefined) value.data.kv_number = [];
                    value.data.kv_number.splice(plainIndex, 0, v);
                },

                valueObjectPush : function(value, plainIndex, v) {
                    if (value.binID != TYPE.OBJECT) return;
                    if (value.data.kv_number == undefined) value.data.kv_number = [];
                    value.data.kv_number.push(v);
                },

                
                valueObjectSortUnsafe : function(value, lessFnValue) {
                    if (value.data.kv_number == undefined) return;
                    const vals = value.data.kv_number;
                    if (vals.length == 0) return;
                    
                    const names = [
                        heap.createString('a'),
                        heap.createString('b')
                    ];
                    
                    vals.sort(function(a, b) {
                        return heap.valueAsNumber(vm.callFunction(lessFnValue, [a, b], names));
                    });
                },
                
                
                valueObjectForeach : function(value, func) {
                    if (value.data.table_attribSet) {
                        const set = heap.valueObjectAccess(value.data.table_attribSet, heap_specialString_foreach, 0);
                        if (set.binID) {
                            const v = vm.callFunction(set, [], []);
                            if (binID != TYPE.OBJECT) {
                                vm.raiseErrorString("foreach attribute MUST return an object.");
                                return;
                            } else {
                                value = v;
                            }
                        }
                    }
                    
                    const names = [
                        heap_specialString_key,
                        heap_specialString_value
                    ];
                    
                    const keys   = valueObjectKeys(value);
                    const values = valueObjectValues(value);
   
                    for(var i = 0; i < keys.length; ++i) {
                        vm.callFunction(func, [keys[i], values[i]], names);
                    }
                },
                
                valueObjectSet : function(value, key, v, isBracket) {
                    if (value.binID != TYPE.OBJECT) {
                        vm.raiseErrorString("Cannot set property on something that isn't an object.");
                        return;
                    }
                    
                    if (key.binID == TYPE.EMPTY) {
                        vm.raiseErrorString("Cannot set property with an empty key");
                        return;                    
                    }
                    
                    const assigner = objectGetAccessOperator(value, isBracket, 0);
                    if (assigner.binID) {
                        const r = vm.callFunction(assigner, [key, v], [heap_specialString_key, heap_specialString_value]);
                        if (r.binID == TYPE.BOOLEAN && r.data) {
                            return;
                        }
                    }
                    return objectPutProp(value, key, v);
                },
                
                valueObjectSetTable: function(value, srcTable) {
                    if (srcTable.binID != TYPE.OBJECT) {
                        vm.raiseErrorString("Cannot use object set assignment syntax something that isnt an object.");
                        return;
                    }
                    const keys = heap.valueObjectKeys(srcTable);
                    for(var i = 0; i < keys.length; ++i) {
                        const key = heap.valueObjectArrayAtUnsafe(keys, i);
                        const val = heap.valueObjectAccess(srcTable, key, 1);
                        valueObjectSet(value, key, val, 0);
                    }
                },
                
                valueObjectSetIndexUnsafe : function(value, plainIndex, v) {
                    value.data.kv_number[plainIndex] = v;
                },
                
                valueObjectSetAttributes : function(value, opObject) {
                    if (value.binID != TYPE.OBJECT) {
                        vm.raiseErrorString("Cannot assign attributes set to something that isnt an object.");
                        return;
                    }
                    
                    if (opObject.binID != TYPE.OBJECT) {
                        vm.raiseErrorString("Cannot assign attributes set that isn't an object.");
                        return;
                    }
                    
                    if (opObject === value) {
                        vm.raiseErrorString("Cannot assign attributes set as its own object.");
                        return;
                    }
                    
                    value.data.table_attribSet = v;
                },
                
                valueObjectGetAttributesUnsafe : function(value) {
                    return value.data.table_attribSet;
                },
                
                valueObjectRemoveKey : function(value, key) {
                    if (value.binID != TYPE.OBJECT) {
                        return; // no error?
                    }
                    
                    switch(key.binID) {
                      case TYPE.OBJECT: return;
                      case TYPE.STRING:
                        if (value.data.kv_string == undefined) return;
                        delete value.data.kv_string[key.data];
                        break;
                      case TYPE.TYPE:
                        if (value.data.kv_types == undefined) return;
                        delete value.data.kv_types[key.data];
                        break;
                      case TYPE.NUMBER:
                        if (value.data.kv_number == undefined || value.data.kv_number.length == 0) return;
                        value.data.kv_number.splice(Math.floor(key.data), 1);
                        break;
                      case TYPE.BOOLEAN:
                        if (key.data) {
                            value.data.kv_true = undefined;
                        } else {
                            value.data.kv_false = undefined;                        
                        }
                        break;
                      case TYPE.OBJECT:
                        if (value.data.kv_object == undefined) return;
                        delete value.data.kv_object[key];
                        break;
                    }
                },
                
                
                valueObjectRemoveKeyString : function(value, plainString) {
                    if (value.binID != TYPE.OBJECT) {
                        return; // no error?
                    }
                    
                    if (value.data.kv_string == undefined) return;
                    delete value.data.kv_string[plainString];
                },
                
                valueTypeGetTypecode : function(value) {
                    if (value.binID != TYPE.TYPE) return 0;
                    return value.data.typecode;
                },
                
                valueIsA : function(value, typeobj) {
                    if (typeobj.binID != TYPE.TYPE) {
                        vm.raiseErrorString("VM error: cannot query isa() with a non Type value.");
                        return 0;
                    }
                    
                    if (typeobj.data.id == heap_type_any.data.id) return 1;
                    if (value.binID != TYPE.OBJECT) {
                        return (heap.valueGetType(value)).data.id == typeobj.data.id;
                    } else {
                        if (typeobj.data.id == value.data.typecode) return 1;
                        const typep = heap.valueGetType(value); 
                        if (typep.data.id == typeobj.data.id) return 1;
                        
                        if (typep.data.isa == undefined) return 0;
                        for(var i = 0; i < typep.data.isa.length; ++i) {
                            if (typep.data.isa[i] == typeobj.data.id) return 1;
                        }
                        return 0;
                    }
                },
                
                valueGetType : function(value) {
                    switch(value.binID) {
                      case TYPE.EMPTY:      return heap_type_empty;
                      case TYPE.BOOLEAN:    return heap_type_boolean;
                      case TYPE.NUMBER:     return heap_type_number;
                      case TYPE.STRING:     return heap_type_string;
                      case TYPE.TYPE:       return heap_type_type;
                      case TYPE.OBJECT: 
                        if (value.data.function_stub != undefined)
                            return heap_type_function;
                        else {
                            return heap_typecode2data[value.data.typecode];
                        } 
                    }
                },
                
                
                valueQuery : function(value, query) {
                    const out = heap_queryTable[query](value);
                    if (out == undefined) {
                        vm.raiseErrorString("VM error: unknown query operator");
                    }
                    return out;
                },
                
                valueTypeName : function(value) {
                    if (value.binID != TYPE.TYPE) {
                        vm.raiseErrorString("VM error: cannot get type name of a non Type value.");
                        return heap_specialString_empty;
                    }
                    switch(value.data.id) {
                      case 1: return heap_specialString_empty;
                      case 2: return heap_specialString_empty;
                      case 3: return heap_specialString_empty;
                      case 4: return heap_specialString_empty;
                      case 5: return heap_specialString_empty;
                      case 6: return heap_specialString_empty;
                      case 7: return heap_specialString_empty;
                      default:
                        return value.data.name;
                    }
                },
                
                getEmptyType    : function() {return heap_type_empty;},
                getBooleanType  : function() {return heap_type_boolean;},
                getNumberType   : function() {return heap_type_number;},
                getStringType   : function() {return heap_type_string;},
                getObjectType   : function() {return heap_type_object;},
                getFunctionType : function() {return heap_type_function;},
                getTypeType     : function() {return heap_type_type;},
                getAnyType      : function() {return heap_type_any;}
                
                                
            };
            
            heap_queryTable[QUERY.TYPE] = function(value) {
                heap.valueGetType(value);
            };
            
                        
            heap_queryTable[QUERY.COS] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("cos requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.cos(value.data));
            };
            
            heap_queryTable[QUERY.SIN] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("sin requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.sin(value.data));
            };
            
            
            heap_queryTable[QUERY.TAN] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("tan requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.tan(value.data));
            };
            

            heap_queryTable[QUERY.ACOS] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("acos requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.acos(value.data));
            };
            
            heap_queryTable[QUERY.ASIN] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("asin requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.asin(value.data));
            };
            
            
            heap_queryTable[QUERY.ATAN] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("atan requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.atan(value.data));
            };
            heap_queryTable[QUERY.ATAN2] = function(value) {
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ATAN2);
            };
            heap_queryTable[QUERY.SQRT] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("sqrt requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.atan(value.data));
            };
            heap_queryTable[QUERY.ABS] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("abs requires base value to be a number.");
                    return createValue();
                }
                return heap.createNumber(Math.abs(value.data));
            };
            heap_queryTable[QUERY.ISNAN] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("isNaN requires base value to be a number.");
                    return createValue();
                }
                return heap.createBoolean(Math.isNaN(value.data));
            };
            heap_queryTable[QUERY.FLOOR] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("floor requires base value to be a number.");
                    return createValue();
                }
                return heap.createBoolean(Math.floor(value.data));
            };
            heap_queryTable[QUERY.CEIL] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("ceil requires base value to be a number.");
                    return createValue();
                }
                return heap.createBoolean(Math.ceil(value.data));
            };
            heap_queryTable[QUERY.ROUND] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("round requires base value to be a number.");
                    return createValue();
                }
                return heap.createBoolean(Math.round(value.data));
            };
            heap_queryTable[QUERY.RADIANS] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("radian conversion requires base value to be a number.");
                    return createValue();
                }
                return heap.createBoolean(value.data * (Math.PI / 180.0));
            };
            heap_queryTable[QUERY.DEGREES] = function(value) {
                if (value.binID != TYPE.NUMBER) {
                    vm.raiseErrorString("degree conversion requires base value to be a number.");
                    return createValue();
                }
                return heap.createBoolean(value.data * (180.0 / Math.PI));
            };







            heap_queryTable[QUERY.REMOVECHAR] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("removeChar requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REMOVECHAR);
            };

            heap_queryTable[QUERY.SUBSTR] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("substr requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SUBSTR);
            };
            
            heap_queryTable[QUERY.SPLIT] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("split requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SPLIT);
            };

            heap_queryTable[QUERY.SCAN] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("scan requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SCAN);
            };

            heap_queryTable[QUERY.LENGTH] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("length requires base value to be a string.");
                    return createValue();
                }
                return heap.createNumber(value.data.length);
            };
            
            heap_queryTable[QUERY.SEARCH] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("search requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SEARCH);
            };

            heap_queryTable[QUERY.SEARCH_ALL] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("searchAll requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SEARCH_ALL);
            };

            heap_queryTable[QUERY.CONTAINS] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("contains requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_CONTAINS);
            };

            heap_queryTable[QUERY.REPLACE] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("replace requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REPLACE);
            };

            heap_queryTable[QUERY.COUNT] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("count requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_COUNT);
            };

            heap_queryTable[QUERY.SETCHARCODEAT] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("setCharCodeAt requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETCHARCODEAT);
            };

            heap_queryTable[QUERY.SETCHARAT] = function(value) {
                if (value.binID != TYPE.STRING) {
                    vm.raiseErrorString("setCharAt requires base value to be a string.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETCHARAT);
            };






            heap_queryTable[QUERY.KEYCOUNT] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("keycount requires base value to be an object.");
                    return createValue();
                }
                return heap.valueObjectGetKeyCount(value);
            };

            heap_queryTable[QUERY.KEYS] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("keys requires base value to be an object.");
                    return createValue();
                }
                return heap.valueObjectKeys(value);
            };

            heap_queryTable[QUERY.VALUES] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("values requires base value to be an object.");
                    return createValue();
                }
                return heap.valueObjectValues(value);
            };

            heap_queryTable[QUERY.PUSH] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("push requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_PUSH);
            };

            heap_queryTable[QUERY.POP] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("pop requires base value to be an object.");
                    return createValue();
                }
                
                const indexPlain = heap.valueObjectGetNumberKeyCount(value) - 1;
                const key = heap.createNumber(indexPlain);
                const out = heap.valueObjectAccessDirect(value, key, 1);
                if (out != undefined) {
                    heap.valueObjectRemoveKey(value, key);
                    return out;
                }
                return createValue();
            };
            
            heap_queryTable[QUERY.INSERT] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("insert requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_INSERT);
            };
            
            heap_queryTable[QUERY.REMOVE] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("remove requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REMOVE);
            };

            heap_queryTable[QUERY.SETATTRIBUTES] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("setAttributes requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SETATTRIBUTES);
            };

            heap_queryTable[QUERY.ATTRIBUTES] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("setAttributes requires base value to be an object.");
                    return createValue();
                }
                const out = heap.valueObjectGetAttributesUnsafe(value);
                if (out == undefined) {
                    return createValue();
                }
                return out;
            };

            heap_queryTable[QUERY.SORT] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("sort requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SORT);
            };

            heap_queryTable[QUERY.SUBSET] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("subset requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_SUBSET);
            };

            heap_queryTable[QUERY.FILTER] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("filter requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FILTER);
            };

            heap_queryTable[QUERY.FINDINDEX] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("findIndex requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FINDINDEX);
            };

            heap_queryTable[QUERY.ISA] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("isa requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ISA);
            };

            heap_queryTable[QUERY.MAP] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("map requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_MAP);
            };

            heap_queryTable[QUERY.ANY] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("any requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ANY);
            };

            heap_queryTable[QUERY.FOR] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("for requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FOR);
            };

            heap_queryTable[QUERY.FOREACH] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("foreach requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_FOREACH);
            };

            heap_queryTable[QUERY.ALL] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("all requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_ALL);
            };

            heap_queryTable[QUERY.REDUCE] = function(value) {
                if (value.binID != TYPE.OBJECT) {
                    vm.raiseErrorString("reduce requires base value to be an object.");
                    return createValue();
                }
                return vm.getBuiltinFunctionAsValue(vm.EXT_CALL.QUERY_REDUCE);
            };

            




            

            
            

            
            const heap_type_empty = heap.createType();
            const heap_type_boolean = heap.createType();
            const heap_type_number = heap.createType();
            const heap_type_string = heap.createType();
            const heap_type_object = heap.createType();
            const heap_type_function = heap.createType();
            const heap_type_type = heap.createType();
            const heap_type_any = heap.createType();
            
            
            const heap_specialString_false = heap.createString('false');
            const heap_specialString_true = heap.createString('true');
            const heap_specialString_empty = heap.createString('empty');
            const heap_specialString_nothing = heap.createString('');

            const heap_specialString_type_boolean = heap.createString('Boolean');
            const heap_specialString_type_empty = heap.createString('Empty');
            const heap_specialString_type_number = heap.createString('Number');
            const heap_specialString_type_string = heap.createString('String');
            const heap_specialString_type_object = heap.createString('Object');
            const heap_specialString_type_type = heap.createString('Type');
            const heap_specialString_type_function = heap.createString('Function');

            const heap_specialString_bracketAccess = heap.createString('[]');
            const heap_specialString_dotAccess = heap.createString('.');
            const heap_specialString_set = heap.createString('set');
            const heap_specialString_get = heap.createString('get');
            const heap_specialString_foreach = heap.createString('foreach');
            const heap_specialString_keys = heap.createString('keys');
            const heap_specialString_values = heap.createString('values');
            const heap_specialString_name = heap.createString('name');
            const heap_specialString_inherits = heap.createString('inherits');
            const heap_specialString_key = heap.createString('key');
            
            return heap; 
        }();    
    
    
        
        var vm;
        vm = function(){
            const vm_opcodes = {
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
                MATTE_OPCODE_OAS  : 30           
            
            };
            
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
                MATTE_OPERATOR_TOKEN : 17, // $ 1 operand
                MATTE_OPERATOR_GREATER : 18, // > 2 operands
                MATTE_OPERATOR_LESS : 19, // < 2 operands
                MATTE_OPERATOR_GREATEREQ : 20, // >= 2 operands
                MATTE_OPERATOR_LESSEQ : 21, // <= 2 operands
                MATTE_OPERATOR_TRANSFORM : 22, // <> 2 operands
                MATTE_OPERATOR_NOTEQ : 23, // != 2 operands
                MATTE_OPERATOR_MODULO : 24, // % 2 operands
                MATTE_OPERATOR_CARET : 25, // ^ 2 operands
                MATTE_OPERATOR_NEGATE : 26, // - 1 operand
                MATTE_OPERATOR_TYPESPEC : 27, // => 2 operands

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
            const vm_fileIDPool = 10;
            const vm_precomputed = {}; // matte_vm.c: imported
            const vm_specialString_parameters = heap.createString('parameters');
            const vm_specialString_onsend = heap.createString('onSend');
            const vm_specialString_onerror = heap.createString('onError');
            const vm_specialString_message = heap.createString('message');
            const vm_specialString_value = heap.createString('value');
            const vm_externalFunctionIndex = []; // all external functions
            const vm_externalFunctions = {}; // name -> id in index
            const vm_callstack = [];
            const vm_opcodesSwitch = [];
            const vm_valueStack = [];

            var vm_pendingCatchable = false;
            var vm_pendingRestartCondition = false;
            var vm_stacksize = 0;
            var vm_frame;

            const vm_stackframeGetReferrable(i, referrableID) {
                i = vm_stacksize - 1 - i;
                if (i >= vm_stacksize) {
                    vm.raiseErrorString("Invalid stackframe requested in referrable query.");
                    return undefined;
                }
                
                if (referrableID < vm_callstack[i].referrableSize) {
                    return heap.valueObjectArrayAtUnsafe(vm_callstack[i].referrable, referrableID);
                } else {
                    const ref = heap.valueGetCapturedValue(vm_callstack[i].context, referrableID - vm_callstack[i].referrableSize);
                    if (!ref) {
                        vm.raiseErrorString("Invalid referrable");
                        return undefined;
                    }
                    return ref;
                }
            };


            const vm_listen = function(v, respObject) {
                if (!heap.valueIsCallable(v)) {
                    vm.raiseErrorString("Listen expressions require that the listened-to expression is a function. ");
                    return heap.createEmpty();
                }
                
                if (respObject.binID != heap.TYPE.EMPTY && respObject.binID != heap.TYPE.OBJECT) {
                    vm.raiseErrorString("Listen requires that the response expression is an object.");
                    return heap.createEmpty();                    
                }
                
                var onSend;
                var onError;
                if (respObject.binID != heap.TYPE.EMPTY) {
                    onSend = heap.valueObjectAccessDirect(respObject, vm_specailString_onsend, 0);
                    if (onSend && onSend.binID != heap.TYPE.EMPTY && !heap.valueIsCallable(onSend)) {
                        vm.raiseErrorString("Listen requires that the response object's 'onSend' attribute be a Function.");
                        return heap.createEmpty();                                            
                    }
                    onError = heap.valueObjectAccessDirect(respObject, vm_specailString_onerror, 0);
                    if (onError && onError.binID != heap.TYPE.EMPTY && !heap.valueIsCallable(onError)) {
                        vm.raiseErrorString("Listen requires that the response object's 'onError' attribute be a Function.");
                        return heap.createEmpty();                                            
                    }
                }
                
                var out = vm.callFunction(v, [], []);
                if (vm_pendingCatchable) {
                    const catchable = vm_catchable;
                    vm_catchable.binID = 0;
                    vm_pendingCatchable = 0;
                    
                    if (vm.pendingCatchableIsError && onSend) {
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
                    return heap.createEmpty(); // shouldnt get here
                } else {
                    return out;
                }
            };
            

            const vm_pushFrame = function() {
                vm_stacksize ++;
                var frame;
                if (vm_callstack.length < vm_stacksize) {
                    while(vm_callstack.length < vm_stacksize) {
                        vm_callstack.push({});
                    }
                    frame = vm_callstack[vm_stacksize-1];
                    frame.pc = 0;
                    frame.prettyName = "";
                    frame.context = heap.createEmpty();
                    frame.stub = null;
                    frame.valueStack = []
                } else {
                    frame = vm_callstack[vm_stacksize-1];
                    frame.stub = null;
                    frame.prettyName = "";
                    frame.callstack.length = 0;
                    frame.pc = 0;
                }
                
                if (vm_pendingRestartCondition) {
                    frame.restartCondition = vm_pendingRestartCondition;
                    frame.restartConditionData = vm_pendingRestartConditionData;
                } else {
                    frame.restartCondition = null;
                    frame.restartConditionData = null;
                }
                return frame;
            };
            
            const vm_findStub = function(fileid, stubid) {
                return (vm_stubIndex[fileid])[stubid];
            };
            
            const vm_executableLoop = function() {
                vm_frame = vm_callstack[vm_stacksize-1];
                var inst;
                var instCount = frame.stub.instructionCount;
                vm_sfsCount = 0;

                // tucked with RELOOP
                while(true) {while(frame.pc < instCount) {
                    inst = stub.isntructions[frame.pc++];
                    vm_opcodeSwitch[inst.opcode](inst);
                }}

            };
            return {
                // controls the heap
                heap : heap,

                // the external function set.
                externalFunctions : {},
                
                // constains stackframe objects
                stackframe : []
                
                // IDs of built-in external calls
                // matte_opcode.h: matteExtCall_t
                EXT_CALL : {
                    NOOP : 0,
                    FOREVER : 1,
                    IMPORT : 2,
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
                    QUERY_ISA : 25,
                    QUERY_MAP : 26,
                    QUERY_REDUCE : 27,
                    QUERY_ANY : 28,
                    QUERY_ALL : 29,
                    QUERY_FOR : 30,
                    QUERY_FOREACH : 31,
                    
                    QUERY_CHARAT : 32,
                    QUERY_CHARCODEAT : 33,
                    QUERY_SETCHARAT : 34,
                    QUERY_SETCHARCODEAT : 35,
                    QUERY_SCAN : 36,
                    QUERY_SEARCH : 37,
                    QUERY_SEARCH_ALL : 38,
                    QUERY_SPLIT : 39,
                    QUERY_SUBSTR : 40,
                    QUERY_CONTAINS : 41,
                    QUERY_COUNT : 42,
                    QUERY_REPLACE : 43,
                    QUERY_REMOVECHAR : 44,
                    
                    GETEXTERNALFUNCTION : 45
                    
                },
                
                getBuiltinFunctionAsValue : function(extCall) {
                    throw new Error("TODO");
                },
                
                
                // Performs import as if from the code context,
                // invoking "onImport"
                import : function(module) {
                    if (vm_imports[module] != undefined)
                        return vm_imports[module];
                    
                    // check built-in imports
                    // TODO
                    const fileID = vm_fileIDPool++;
                    const stubs = bytecode.stubsFromBytecode(fileID, vm.onImport(module));
                    // addstubs
                    for(var i = 0; i < stubs.length; ++i) {
                        const stub = stubs[i];
                        var fileindex = vm_stubIndex[stub.fileID];
                        if (fileindex == empty) {
                            fileindex = {};
                            vm_stubIndex[stub.fileID] = fileindex;
                        }
                        fileindex[stub.stubID] = stub;
                    }
                    
                    
                    // for now, no parameters
                    return vm.runFileID(fileID, heap.createEmpty(), module);
                },
                
                
                raiseErrorString : function(string) {
                    vm.raiseError(heap.createString(string));
                }
                
                raiseError : function(value) {
                
                },
                
                runFileID : function(fileID, parameters, importPath) {
                    if (fileid == 0xfffffffe) // debug fileID
                        throw new Error('Matte debug context is not currently supported');

                    const precomp = vm_precomputed[fileid];
                    if (precomp != undefined) return precomp;
                    
                    const stub = vm_stubIndex[fileid];
                    if (stub == undefined) {
                        vm.raiseErrorString('Script has no toplevel context to run');
                        return heap.createEmpty();
                    }
                    const func = heap.createFunction(stub);
                    const result = vm.callFunction(func, [parameters], [vm_specialString_parameters]);
                    vm_precomputed[fileid] = result;
                    return result;
                },
                
                callFunction : function(func, args, argNames) {
                    if (vm_pendingCatchable) return heap.createEmpty();
                    const callable = heap.valueIsCallable(func);
                    if (callable == 0) {
                        vm.raiseErrorString("Error: cannot call non-function value ");
                        return heap.createEmpty();
                    }
                    
                    if (args.length != argNames.length) {
                        vm.raiseErrorString("VM call as mismatching arguments and parameter names.");
                        return heap.createEmpty();                        
                    }
                    
                    if (func.binID == heap.TYPE.TYPE) {
                        if (args.length) {
                            if (argNames[0].data != vm_specailString_from.data) {
                                vm.raiseErrorString("Type conversion failed: unbound parameter to function ('from')");
                            }
                            return heap.valueToType(args[0], func);
                        } else {
                            vm.raiseErrorString("Type conversion failed (no value given to convert).");
                            return heap.createEmpty();
                        }
                    }
                    
                    
                    //external function 
                    const stub = heap.valueGetBytecodeStub(func);
                    if (stub.fileID == 0) {
                        const external = stub.stubID;
                        if (external >= vm_externalFunctionIndex.length) {
                            return heap.createEmpty();
                        }
                        
                        const set = vm_externalFunctionIndex[external];
                        const argsReal = [];
                        const lenReal = args.length;
                        const len = stub.argCount;
                        
                        
                        for(var i = 0; i < lenReal; ++i) {
                            for(var n = 0; n < len; ++n) {
                                if (stub.argNames[n] == argNames[i].data) {
                                    argsReal[n] = args[i];
                                    break;
                                }
                            }
                            
                            if (n == len) {
                                var str;
                                if (len) {
                                    str = "Could not bind requested parameter: '"+argNames[i].data+"'.\n Bindable parameters for this function: ";
                                } else {
                                    str = "Could not bind requested parameter: '"+argNames[i].data+"'.\n (no bindable parameters for this function)";                                
                                }
                                
                                for(n = 0; n < len; ++n) {
                                    str += " \"" + stub.argNames[n] + "\" ";
                                }
                                
                                vm.raiseErrorString(str);
                                return heap.createEmpty();
                            }
                        }
                        
                        var result;
                        if (callable == 2) {
                            const ok = heap.valueObjectFunctionPreTypeCheckUnsafe(func, argsReal);
                            if (ok) 
                                result = set.userFunction(func, argsReal, set.userData);
                            heap.valueObjectFunctionPostTypeCheckUnsafe(func, result);
                        } else {
                            result = set.userFunction(func, argsReal, set.userData);                        
                        }
                        return result;

                    // non-external call
                    } else {
                        const argsReal = [];
                        const lenReal = args.length;
                        var   len = stub.argCount;
                        const referrables = [];
                        
                        for(var i = 0; i < lenReal; ++i) {
                            for(var n = 0; n < len; ++n) {
                                if (stub.argNames[n] == argNames[i].data) {
                                    referrables[n+1] = args[i];
                                    break;
                                }
                            }
                            
                            if (n == len) {
                                var str;
                                if (len) {
                                    str = "Could not bind requested parameter: '"+argNames[i].data+"'.\n Bindable parameters for this function: ";
                                } else {
                                    str = "Could not bind requested parameter: '"+argNames[i].data+"'.\n (no bindable parameters for this function)";                                
                                }
                                
                                for(n = 0; n < len; ++n) {
                                    str += " \"" + stub.argNames[n] + "\" ";
                                }
                                
                                vm.raiseErrorString(str);
                                return heap.createEmpty();
                            }
                        }
                        
                        var ok = 1;
                        if (callable == 2 && len) {
                            const arr = referrables.slice(1, len); 
                            ok = heap.valueObjectFunctionPreTypeCheckUnsafe(d, arr);
                        }
                        len = stub.localCount;
                        for(var i = 0; i < len; ++i) {
                            referrables.push(heap.createEmpty());
                        }
                        const frame = vm_pushFrame();
                        frame.context = func;
                        frame.stub = stub;
                        referrables[0] = func;
                        frame.referrable = heap.createObjectArray(referrables);
                        frame.referrableSize = referrables.length;
                        
                        var result;
                        if (callable == 2) {
                            if (ok) {
                                result = vm_executionLoop(vm);
                            } 
                            if (!vm_pendingCatchable)
                                heap.valueObjectFunctionPostTypeCheckUnsafe(func, result);
                        } else {
                            result = vm_executionLoop(vm);
                        }
                        
                        if (!vm_stacksize && vm_pendingCatchable) {
                            if (vm_unhandled) {
                                vm_unhandled(
                                    vm_errorLastFile,
                                    vm_errorLastLine,
                                    vm_catchable,
                                    vm_unhandledData
                                );
                            }
                            vm_catchable = heap.createEmpty();
                            vm_pendingCatchable = 0;
                            vm_pendingCatchableIsError = 0
                            return heap.createEmpty();
                        }
                        return result;
                    }
                }
                
                
                // external function that takes a module path
                // and returns a ByteArray
                onImport : onImport
            };
            
            
            
            // implement operators 
            /////////////////////////////////////////////////////////////////
            const vm_badOperator = function(op, value) {
                vm.raiseErrorString(
                    op + " operator on value of type " + heap.valueTypeName(heap.valueGetType(value)) + " is undefined."
                );
            };
            
            const vm_objectHasOperator = function(a, op) {
                const opSrc = heap.valueObjectGetAttributesUnsafe(a);
                if (opSrc == undefined)
                    return false;
                
                return heap.valueObjectAccessString(opSrc, heap.createString(op)).binID != 0;
            };
            
            const vm_runObjectOperator2(a, op, b) {
                const opSrc = heap.valueObjectGetAttributesUnsafe(a);
                if (opSrc == undefined) {
                    vm.raiseErrorString(op + ' operator on object without operator overloading is undefined.');
                    return heap.createValue;
                } else {
                    const oper = heap.valueObjectAccessString(opSrc, heap.createString(op));
                    return vm.callFunction(
                        oper,
                        [b],
                        [vm_specialString_value]
                    );
                }                
            };
            
            
            const vm_runObjectOperator1(a, op) {
                const opSrc = heap.valueObjectGetAttributesUnsafe(a);
                if (opSrc == undefined) {
                    vm.raiseErrorString(op + ' operator on object without operator overloading is undefined.');
                    return heap.createValue;
                } else {
                    const oper = heap.valueObjectAccessString(opSrc, heap.createString(op));
                    return vm.callFunction(
                        oper,
                        [],
                        []
                    );
                }                
            };          
            

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_ADD] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.NUMBER:
                    return heap.createNumber(a.data + heap.valueAsNumber(b).data);
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "+=", b);
                  default:
                    vm_badOperator("+=", a);
                    return heap.createEmpty();
                }
            };
            

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_SUB] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.NUMBER:
                    return heap.createNumber(a.data - heap.valueAsNumber(b).data);
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "-=", b);
                  default:
                    vm_badOperator("-=", a);
                    return heap.createEmpty();
                }
            };

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_DIV] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.NUMBER:
                    return heap.createNumber(a.data / heap.valueAsNumber(b).data);
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "/=", b);
                  default:
                    vm_badOperator("/=", a);
                    return heap.createEmpty();
                }
            };            
            
            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_MULT] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.NUMBER:
                    return heap.createNumber(a.data * heap.valueAsNumber(b).data);
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "*=", b);
                  default:
                    vm_badOperator("*=", a);
                    return heap.createEmpty();
                }
            };            

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_MOD] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.NUMBER:
                    return heap.createNumber(a.data % heap.valueAsNumber(b).data);
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "%=", b);
                  default:
                    vm_badOperator("%=", a);
                    return heap.createEmpty();
                }
            };                 

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_POW] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.NUMBER:
                    return heap.createNumber(Math.pow(a.data, heap.valueAsNumber(b).data));
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "**=", b);
                  default:
                    vm_badOperator("**=", a);
                    return heap.createEmpty();
                }
            };                 

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_AND] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "&=", b);
                  default:
                    vm_badOperator("&=", a);
                    return heap.createEmpty();
                }
            };                 

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_OR] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "|=", b);
                  default:
                    vm_badOperator("|=", a);
                    return heap.createEmpty();
                }
            };                 

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_XOR] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "^=", b);
                  default:
                    vm_badOperator("^=", a);
                    return heap.createEmpty();
                }
            };                 

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_BLEFT] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, "<<=", b);
                  default:
                    vm_badOperator("<<=", a);
                    return heap.createEmpty();
                }
            };                 

            vm_operatorFunc[vm_opcodes.MATTE_OPERATOR_ASSIGNMENT_BRIGHT] = function(a, b) {
                switch(a.binID) {
                  case heap.TYPE.OBJECT:
                    return vm_runObjectOperator2(a, ">>=", b);
                  default:
                    vm_badOperator(">>=", a);
                    return heap.createEmpty();
                }
            };                 

            
            
            
            // implement opcodes:
            ////////////////////////////////////////////////////////////////
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NOP]] = function(inst){};

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_LST]] = function(inst){
                const v = vm_listen(vm_valueStack[vm_valueStack.length-2], vm_valueStack[vm_valueStack.length-1]);
                vm_valueStack.pop(); 
                vm_valueStack.pop(); 
                vm_valueStack.push(v); 
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_PRF]] = function(inst) {
                const v = vm_stackframeGetReferrable(0, Math.floor(inst.data));
                if (v) {
                    vm_valueStack.push(v);
                }
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_PNR]] = function(inst) {
                throw new Error("TODO"); // or not, DEBUG
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NEM]] = function(inst) {
                vm_valueStack.push(heap.createEmpty());
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NNM]] = function(inst) {
                vm_valueStack.push(heap.createNumber(inst.data));
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NBL]] = function(inst) {
                vm_valueStack.push(heap.createBoolean(Math.floor(inst.data) == 0.0));
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NST]] = function(inst) {
                vm_valueStack.push(heap.createString(vm_frame.stub.strings[Math.floor(inst->data)]));
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NST]] = function(inst) {
                vm_valueStack.push(heap.createObject());
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_SFS]] = function(inst) {
                vm_sfsCount = inst.data;
                // the fallthrough
                vm_opcodes[MATTE_OPCODE_NFN](frame.stub.instructions[frame.pc++]);   
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_NFN]] = function(inst) {
                const stub = vm_findStub(inst.nfnFileID, inst.data);
                if (stub == undefined) {
                    vm.raiseErrorString("NFN opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)");
                    return;
                }
                
                if (vm_sfsCount) {
                    if (vm_valueStack.length < vm_sfsCount) {
                        vm.raiseErrorString("VM internal error: too few values on stack to service SFS opcode!");
                        return;
                    }
                    
                    const vals = [];
                    for(var i = 0; i < vm_sfsCount; ++i) {
                        vals[vm_sfsCount - i - 1] = vm_valueStack[vm_valueStack.length-1-i];
                    }
                    const v = heap.createTypedFunction(stub, vals);
                    vm_sfsCount = 0;
                    vm_valueStack.push(v);
                } else {
                    vm_valueStack.push(heap.createFunction(stub));
                }
            };
            
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_CAA]] = function(inst) {
                if (vm_valueStack.length < 2) {
                    vm.raiseErrorString("VM error: missing object - value pair for constructor push");
                    return;
                }
                const obj = vm_valueStack[vm_valueStack.length-2];
                heap.valueObjectPush(obj, vm_valueStack[vm_valueStack.length-1]);
                vm_valueStack.pop();
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_CAS]] = function(inst) {
                if (vm_valueStack.length < 3) {
                    vm.raiseErrorString("VM error: missing object - value pair for constructor push");
                    return;
                }
                const obj = vm_valueStack[vm_valueStack.length-3];
                heap.valueObjectSet(obj, vm_valueStack[vm_valueStack.length-2], vm_valueStack[vm_valueStack.length-1], 1);
                vm_valueStack.pop();
                vm_valueStack.pop();
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_SPA]] = function(inst) {
                if (vm_valueStack.length < 2) {
                    vm.raiseErrorString("VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");
                    return;
                }
                const p = vm_valueStack.pop();
                const target = vm_valueStack[vm_valueStack.length-1];
                const len = heap.valueObjectGetNumberKeyCount(p);
                var keylen = heap.valueObjectGetNumberKeyCount(p);
                for(var i = 0; i < len; ++i) {
                    heap.valueObjectInsert(
                        target, 
                        keylen++,
                        heap.valueObjectAccessIndex(p, i)
                    );
                }   
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_SPO]] = function(inst) {
                if (vm_valueStack.length < 2) {
                    vm.raiseErrorString("VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");
                    return;
                }
                
                const p = vm_valueStack.pop();
                const keys = heap.valueObjectKeys(p);
                const vals = heap.valueObjectValues(p);
                
                const target = vm_valueStack[vm_valueStack.length-1];
                const len = heap.valueObjectGetNumberKeyCount(keys);
                for(var i = 0; i < len; ++i) {
                    heap.valueObjectSet(
                        target,
                        heap.valueObjectAccessDirect(keys, i),
                        heap.valueObjectAccessDirect(vals, i),
                        1
                    );
                }
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_PTO]] = function(inst) {
                var v;
                switch(Math.floor(inst.data)) {
                  case 0: v = heap.getEmptyType(); break;
                  case 1: v = heap.getBooleanType(); break;
                  case 2: v = heap.getNumberType(); break;
                  case 3: v = heap.getStringType(); break;
                  case 4: v = heap.getObjectType(); break;
                  case 5: v = heap.getFunctionType(); break;
                  case 6: v = heap.getTypeType(); break;
                  case 7: v = heap.getAnyType(); break;
                }
                vm_valueStack.push(v);
            };
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_CAL]] = function(inst) {
                if (vm_valueStack.length < 0) {
                    vm.raiseErrorString("VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");
                    return;
                }
                const args = [];
                const argNames = [];
                
                const stackSize = vm_valueStack.length;
                const key = vm_valueStack[vm_valueStack.length-1];
                
                if (stackSize > 2 && key.binID == heap.TYPE.STRING) {
                    while(i < stackSize && key.binID == heap.TYPE.STRING) {
                        argNames.push(key);
                        i++;
                        key = vm_valueStack[vm_valueStack.length - 1 - i];
                        args.push(key);
                        i++;
                        key = vm_valueStack[vm_valueStack.length - 1 - i];
                    }
                }
                
                const argCount = args.length;
                
                if (i == stackSize) {
                    vm.raiseErrorString("VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");         
                    return;
                }
                
                const func = vm_valueStack[vm_valueStack.length - 1 - i];
                const result = vm.callFunction(func, args, argNames);
                
                for(i = 0; i < argCount; ++i) {
                    vm_valueStack.pop();
                    vm_valueStack.pop();
                }
                vm_valueStack.push(result);
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_ARF]] = function(inst) {
                if (vm_valueStack.length < 1) {
                    vm.raiseErrorString("VM error: tried to prepare arguments for referrable assignment, but insufficient arguments on the stack.");
                    return;
                }
                
                const refn = inst.data % 0xffffffff;
                const op = Math.floor(inst->data / 0xffffffff);
                
                const ref = vm_stackframeGetReferrable(0, refn);
                if (ref) {
                    const v = vm_valueStack[vm_valueStack.length-1];
                    var vOut;
                    switch(op + vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE) {
                      case vm_operator.MATTE_OPERATOR_ASSIGNMENT_NONE:
                        vOut = v;
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
                        break;
                        
                      default:
                        vOut = heap.createEmpty();
                        vm.raiseErrorString("VM error: tried to access non-existent referrable operation (corrupt bytecode?).");
                    }
                    if (vOut.binID == heap.TYPE.OBJECT) {    
                        // already in its proper place: simply mutates object.            
                    } else {
                        matte_vm_stackframe_set_referrable(vm, 0, refn, vOut);                    
                    }
                    vm_valueStack.pop();
                    vm_valueStack.push(vOut);

                } else {
                    vm.raiseErrorString("VM error: tried to access non-existent referrable.");
                }
            };
            
            
            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_POP]] = function(inst) {
                var popCount = inst.data;
                while(popCount > 0 && vm_valueStack.length > 0) {
                    vm_valueStack.pop();
                    popCount--;
                }
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_CPY]] = function(inst) {
                if (vm_valueStack.length == 0) {
                    vm.raiseErrorString("VM error: cannot CPY with empty stack");
                }
                
                vm_valueStack.push(vm_valueStack[vm_valueStack.length-1]);
            };

            vm_opcodeSwitch[vm_opcodes[MATTE_OPCODE_OSN]] = function(inst) {
                if (vm_valueStack.length < 3) {
                    
                }
            };

            
            
            
            

        }();
        
        

        return vm;        
    }
};
