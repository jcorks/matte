<@>_mbuffer_create = getExternalFunction("__matte_::mbuffer_create");
<@>_mbuffer_release = getExternalFunction("__matte_::mbuffer_release");
<@>_mbuffer_set_size = getExternalFunction("__matte_::mbuffer_set_size");
<@>_mbuffer_copy = getExternalFunction("__matte_::mbuffer_copy");
<@>_mbuffer_set = getExternalFunction("__matte_::mbuffer_set");
<@>_mbuffer_subset = getExternalFunction("__matte_::mbuffer_subset");
<@>_mbuffer_append_byte = getExternalFunction("__matte_::mbuffer_append_byte");
<@>_mbuffer_append = getExternalFunction("__matte_::mbuffer_append");
<@>_mbuffer_remove = getExternalFunction("__matte_::mbuffer_remove");
<@>_mbuffer_get_size = getExternalFunction("__matte_::mbuffer_get_size");


<@>_mbuffer_get_index = getExternalFunction("__matte_::mbuffer_get_index");
<@>_mbuffer_set_index = getExternalFunction("__matte_::mbuffer_set_index");


return class({

    define::(this, args, thiclass) {
        <@>MBuffer = thisclass.type;
        @length = 0;
        @buffer = (
            if(args._buffer == empty) ::<={
                return _mbuffer_create()
            } else ::<={
                length = _mbuffer_get_size(args._buffer);
                return args._buffer;
            }
        );
            
        
        
        
        <@> checkReleased::(t){
            when(t.handle == empty) error("Buffer was released. This buffer should not be used any more.");
        };
        
        this.interface({
            // appends a different byte buffer to this one
            append::(other => MBuffer) {
                checkReleased(this);
                checkReleased(other);
                _mbuffer_append(buffer, other.handle);
                length = _mbuffer_get_size(buffer);
            },
            
            
            remove::(from => Number, to => Number) {
                checkReleased(this);
                _mbuffer_remove(buffer, from, to);
                length = _mbuffer_get_size(buffer);
            },
            
            
            // performs a memcpy
            copy::(
                selfOffset => Number, 
                src        =>  MBuffer, 
                srcOffset  => Number, 
                len        =>  Number
            ) {
                checkReleased(this);
                checkReleased(src);
                _mbuffer_copy(buffer, selfOffset, src, srcOffset, len);
            }
            
            // performs a memset
            set::(
                selfOffset => Number, 
                val => Number, 
                len => Number
            ) {
                checkReleased(this);
                checkReleased(src);
                _mbuffer_set(buffer, selfOffset, src, len);
            }
            
            // copies a new buffer.
            subset::(
                from => Number, 
                to   => Number
            ) => MBuffer {
                checkReleased(this);
                return thisclass.new(
                    {
                        _buffer:_mbuffer_subset(buffer, from, to)
                    }
                );
            },
            
            // Frees the buffer and empties this buffer.
            release ::{
                checkReleased(this);
                _mbuffer_release(buffer);
                length = 0;;                                            
            },
            
            
            
            appendByte ::(val => Number){
                checkReleased(this);
                _mbuffer_append_byte(buffer, val);
            },
            
            
            handle : {
                get :: {
                    return buffer;
                }
            }
            
            size : {
                get :: {
                    return length;
                },
                
                
                set ::(v) {
                    checkReleased(this);
                    length = 0;         
                    _mbuffer_set_size(this, 0);       
                }
            }
        });
        
        this.attributes({
            '[]' : {
                get ::(key => Number){
                    checkReleased(this);
                    _mbuffer_get_index(this, key);                    
                },
                
                set ::(key => Number, val => Number) {
                    checkReleased(this);
                    _mbuffer_set_index(this, key, val);                                    
                }
            },
        });
    }
});
