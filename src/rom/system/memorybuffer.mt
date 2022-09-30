@:_mbuffer_create = getExternalFunction(name:"__matte_::mbuffer_create");
@:_mbuffer_release = getExternalFunction(name:"__matte_::mbuffer_release");
@:_mbuffer_set_size = getExternalFunction(name:"__matte_::mbuffer_set_size");
@:_mbuffer_copy = getExternalFunction(name:"__matte_::mbuffer_copy");
@:_mbuffer_set = getExternalFunction(name:"__matte_::mbuffer_set");
@:_mbuffer_subset = getExternalFunction(name:"__matte_::mbuffer_subset");
@:_mbuffer_append_byte = getExternalFunction(name:"__matte_::mbuffer_append_byte");
@:_mbuffer_append = getExternalFunction(name:"__matte_::mbuffer_append");
@:_mbuffer_remove = getExternalFunction(name:"__matte_::mbuffer_remove");
@:_mbuffer_get_size = getExternalFunction(name:"__matte_::mbuffer_get_size");
@:_mbuffer_as_utf8 = getExternalFunction(name:"__matte_::mbuffer_as_utf8");
@:_mbuffer_read_primitive = getExternalFunction(name:'__matte_::mbuffer_read_primitive');
@:_mbuffer_write_primitive = getExternalFunction(name:'__matte_::mbuffer_write_primitive');
@:_mbuffer_get_index = getExternalFunction(name:"__matte_::mbuffer_get_index");
@:_mbuffer_set_index = getExternalFunction(name:"__matte_::mbuffer_set_index");



@:class = import(module:'Matte.Core.Class');
@MemoryBuffer = class(
    name: 'Matte.System.MemoryBuffer',
    define:::(this) {
        @:MBuffer = this.class.type;
        @length = 0;
        @buffer;


        this.constructor= ::(handle) {
            buffer = (
                if (handle == empty) ::<={
                    return _mbuffer_create();
                } else ::<={
                    length = _mbuffer_get_size(a:handle);
                    return handle;
                }
            );
            return this;
        };
            
        
        
        
        @: checkReleased::(t){
            when(t.handle == empty) error(message:"Buffer was released. This buffer should not be used any more.");
        };
        
        this.interface = {
            // appends a different byte buffer to this one
            append::(other => MBuffer) {
                checkReleased(t:this);
                checkReleased(t:other);
                _mbuffer_append(a:buffer, b:other.handle);
                length = _mbuffer_get_size(a:buffer);
            },
            
            
            remove::(from => Number, to => Number) {
                checkReleased(t:this);
                _mbuffer_remove(a:buffer, b:from, c:to);
                length = _mbuffer_get_size(a:buffer);
            },
            
            
            // performs a memcpy
            copy::(
                selfOffset => Number, 
                src        =>  MBuffer, 
                srcOffset  => Number, 
                len        =>  Number
            ) {
                checkReleased(t:this);
                checkReleased(t:src);
                _mbuffer_copy(a:buffer, b:selfOffset, c:src.handle, d:srcOffset, e:len);
            },
            
            // performs a memset
            set::(
                selfOffset => Number, 
                value => Number, 
                len => Number
            ) {
                checkReleased(t:this);
                _mbuffer_set(a:buffer, b:selfOffset, c:value, d:len);
            },
            
            // copies a new buffer.
            subset::(
                from => Number, 
                to   => Number
            ) => MBuffer {
                checkReleased(t:this);
                return this.class.new(handle:
                    _mbuffer_subset(a:buffer, b:from, c:to)
                );
            },
            
            

            // Frees the buffer and empties this buffer.
            release ::{
                checkReleased(t:this);
                _mbuffer_release(a:buffer);
                length = 0;;                                            
            },
            
            
            
            appendByte ::(val => Number){
                checkReleased(t:this);
                _mbuffer_append_byte(a:buffer, b:val);
                length += 1;
            },
            
            utf8 : {
                get :: {
                    return _mbuffer_as_utf8(a:buffer);
                }
            },
            
            handle : {
                get :: {
                    return buffer;
                }
            },
            
            size : {
                get :: {
                    return length;
                },
                
                
                set ::(value) {
                    checkReleased(t:this);
                    length = value;         
                    _mbuffer_set_size(a:buffer, b:value);       
                }
            },
            
            
    /*
    primitives : {
        0 -> i8
        1 -> i16,
        2 -> i32,
        3 -> i64,
        4 -> u8
        5 -> u16
        6 -> u32
        7 -> u64
        8 -> float,
        9 -> double
    }
    */
            readI8::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:0);
            },

            readI16::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:1);
            },

            readI32::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:2);
            },

            readI64::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:3);
            },



            readU8::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:4);
            },

            readU16::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:5);
            },

            readU32::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:6);
            },

            readU64::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:7);
            },


            readFloat32::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:8);
            },

            readFloat64::(offset) {
                checkReleased(t:this);
                return _mbuffer_read_primitive(a:buffer, b:offset, c:9);
            },






            writeI8::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:0, d:value);
            },

            writeI16::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:1, d:value);
            },

            writeI32::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:2, d:value);
            },

            writeI64::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:3, d:value);
            },



            writeU8::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:4, d:value);
            },

            writeU16::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:5, d:value);
            },

            writeU32::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:6, d:value);
            },

            writeU64::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:7, d:value);
            },


            writeFloat32::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:8, d:value);
            },

            writeFloat64::(offset, value) {
                checkReleased(t:this);
                return _mbuffer_write_primitive(a:buffer, b:offset, c:9, d:value);
            },




                        
            
            
            
            
            
            
            
            
            
        };
        
        this.attributes = {
            '[]' : {
                get ::(key => Number){
                    checkReleased(t:this);
                    return _mbuffer_get_index(a:buffer, b:key);                    
                },
                
                set ::(key => Number, value => Number) {
                    checkReleased(t:this);
                    _mbuffer_set_index(a:buffer, b:key, c:value);                                    
                }
            }
        };
    }
);

return MemoryBuffer;
//@MemoryBuffer = import('Matte.System.MemoryBuffer');
/*
@m = MemoryBuffer.new();
m.size = 10000;

print('-'+m[1000]);
print('-'+m[2001]);

m[2001] = 180;

print('-'+m[1000]);
print('-'+m[2001]);



m.release();

*/
