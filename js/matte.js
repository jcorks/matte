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
                            data       : bytes.chompBytes(8)
                        };
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
                            return vm.call(accessor, [key], [heap_specialString_key]);
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
                
                valueObjectSortUnsafe : function(value, lessFnValue) {
                    if (value.data.kv_number == undefined) return;
                    const vals = value.data.kv_number;
                    if (vals.length == 0) return;
                    
                    const names = [
                        heap.createString('a'),
                        heap.createString('b')
                    ];
                    
                    vals.sort(function(a, b) {
                        return heap.valueAsNumber(vm.call(lessFnValue, [a, b], names));
                    });
                },
                
                
                valueObjectForeach : function(value, func) {
                    if (value.data.table_attribSet) {
                        const set = heap.valueObjectAccess(value.data.table_attribSet, heap_specialString_foreach, 0);
                        if (set.binID) {
                            const v = vm.call(set, [], []);
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
                        vm.call(func, [keys[i], values[i]], names);
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
                        const r = vm.call(assigner, [key, v], [heap_specialString_key, heap_specialString_value]);
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

            const vm_imports = [];
            const vm_stubIndex = {};
            const vm_fileIDPool = 10;
            const vm_runFileID = function(fileID) {
                
            };
            const vm_precomputed = {}; // matte_vm.c: imported
            const vm_specialString_parameters = heap.createString('parameters');
            var vm_pendingCatchable = false;
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
                        return heap.createValue();
                    }
                    const func = heap.createFunction(stub);
                    const result = vm.call(func, [parameters], [vm_specialString_parameters]);
                    vm_precomputed[fileid] = result;
                    return result;
                },
                
                call : function(func, args, argNames) {
                    if (vm_pendingCatchable) return heap.createValue();
                    const callable = heap.valueIsCallable(func);
                    if (callable == 0) {
                        vm.raiseErrorString("Error: cannot call non-function value ");
                        return heap.createValue();
                    }
                    
                    if (args.length != argNames.length) {
                        vm.raiseErrorString("VM call as mismatching arguments and parameter names.");
                        return heap.createValue();                        
                    }
                    
                    if (func.binID == heap.TYPE.TYPE) {
                        if (args.length) {
                            if (argNames[0].data != vm_specailString_from.data) {
                                vm.raiseErrorString("Type conversion failed: unbound parameter to function ('from')");
                            }
                            return heap.valueToType(args[0], func);
                        } else {
                            vm.raiseErrorString("Type conversion failed (no value given to convert).");
                            return heap.createValue();
                        }
                    }
                    
                    
                    //external function 
                    const stub = heap.valueGetBytecodeStub(func);
                    if (stub.fileID == 0) {
                        const external = stub
                    }
                }
                
                
                // external function that takes a module path
                // and returns a ByteArray
                onImport : onImport
            };
        }();
        
        

        return vm;        
    }
};
