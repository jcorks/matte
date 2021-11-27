<@>class = import('Matte.Core.Class');
<@>Array = import('Matte.Core.Array');

<@>_string_create     = getExternalFunction("__matte_::string_create");
<@>_string_get_length = getExternalFunction("__matte_::string_get_length");
<@>_string_set_length = getExternalFunction("__matte_::string_set_length");
<@>_string_search     = getExternalFunction("__matte_::string_search");
<@>_string_replace    = getExternalFunction("__matte_::string_replace");
<@>_string_count      = getExternalFunction("__matte_::string_count");
<@>_string_charcodeat = getExternalFunction("__matte_::string_charcodeat");
<@>_string_charat     = getExternalFunction("__matte_::string_charat");
<@>_string_setchar    = getExternalFunction("__matte_::string_setchar");
<@>_string_setcharcode= getExternalFunction("__matte_::string_setcharcode");
<@>_string_removechar = getExternalFunction("__matte_::string_removechar");
<@>_string_substr     = getExternalFunction("__matte_::string_substr");
<@>_string_split      = getExternalFunction("__matte_::string_split");

@String_ = class({
    name : 'Matte.Core.String',
    define ::(this, args, classinst) {
        <@>MatteString = introspect.type(this);
        <@>handle = m        

        
        
        this.interface({

            onRevive ::(reviveArgs) {
                _string_set_length(handle, 0);
            },


            length : {
                get :: { 
                    return _string_get_length(handle);
                },
                
                set ::(newlen => Number) {
                    _string_set_length(handle, newlen);
                }
            },
            

            
            // returns the index at which the 
            // substring is at. If the substring is 
            // not contained within the string, -1 is returned.
            search::(sub){
                return _string_search(handle, String(sub));     
            },
            
            contains::(sub) {
                return this.search(sub) != -1;
            },


            containsAny::(arr => Object) {
                <@>vals = introspect.values(arr);
                <@>len = introspect.keycount(vals);
                return listen(::{
                    for([0, len], ::(i) {
                        if (this.contains(vals[i])) ::<={
                            send(true);
                        };
                    });
                    return false;                    
                });

            },


            replace::(sub, with) {
                return _string_replace(handle, String(sub), String(with));
            },
            
            count::(sub) {
                return _string_count(handle, String(sub));
            },
            
            charCodeAt::(i) {
                return _string_charcodeat(handle, Number(i));
            },

            charAt::(i) {
                return _string_charat(handle, Number(i));
            },
                        
            setCharAt::(index => Number, a) {
                return _string_setcharat(handle, index, String(a));
            },

            setCharCodeAt::(index => Number, a) {
                return _string_setcharcodeat(handle, index, Number(a));
            },
            
            append::(a) {
                _string_append(handle, String(a));
            },



            removeChar::(i) {
                _string_removechar(handle, Number(i));
            },
            
            
            substr::(from => Number, to => Number) {
                return classinst.new(_string_subset(handle, from, to));
            },

            split::(sub) {  
                @a = _string_split(handle, String(sub));
                @aconv = Array.new();
                foreach(a, ::(k, v) {
                    aconv.push(classinst.new(v));
                });
                return ;
            },
            
            
            /*
            
            @a = Matte.String.new('Here is my value: 40. Just that.');

            // returns a new array. Each element in the array for each [%]
            // speified in the string. If a match could not be found, empty is returned.
            @result = a.scan('value: [%].');

            // should be "40"
            print(result[0]);
            
            
            */
            scan::(fmt) {
                @tokens = classinst.new(fmt).split('[%]');
                @startsWith = (_string_charat(handle, 0) == '[' &&
                               _string_charat(handle, 1) == '%' &&
                               _string_charat(handle, 2) == ']');

                @len = _string_get_length(handle);
                @endsWith = (_string_charat  (handle, len-3) == '[' &&
                               _string_charat(handle, len-2) == '%' &&
                               _string_charat(handle, len-1) == ']');

                
                
                for([0, tokens.length-1], ::(i) {
                    tokens.insert(1+i*2, empty);
                });

                
                
                @out = Array.new();
                @searchIndex = 0;
                @lastIndex;
                loop(::{
                    @searchStr = this.substr(searchIndex, len-1);
                    return listen(::{
                        lastIndex = empty;
                        for([0, tokens.length], ::(i) {
                            @index = searchStr.search(tokens[i]);
                            when (index == -1) ::<={
                                out.length = 0;
                                // if the first token couldnt be found, then we have Issues
                                lastIndex = empty;
                                send(i != 0);                                
                            };
                            
                            
                            if (startsWith && i == 0) ::<={
                                if (index > 0) ::<={                                
                                    out.push(this.substr(0, index-1));
                                } else ::<= {
                                    out.push(classinst.new(''));                                
                                };
                            };
                            
                            if (i != 0) ::<={
                                out.push(this.substr(lastIndex, index));
                            } else ::<= {
                                searchIndex = index;
                            }; 
                            
                            lastIndex = index;                        
                        });
                        
                        // if all tokens are reached, then we are dont.
                        return false;
                    });
                });


                if (endsWith && lastIndex != empty) ::<={
                    if (index > 0) ::<={                                
                        out.push(this.substr(lastIndex, index-1));
                    } else ::<= {
                        out.push(classinst.new(''));                                
                    };
                };


                return out;
            }
            
 

        });
        
        this.attributes({
            (String) :: {
                when(hasStr) strsrc;
                strsrc = arrintr.arrayToString(arrsrc);
                hasStr = true;
                return strsrc;
            },

            'foreach' :: {
                @a = [];
                for([0, this.length], ::(i) {
                    a[i] = this.charAt(i);
                });
                return a;
            },
            
            '+' :: (other){
                @out = classinst.new(String(this));
                out.append(other);
                return out;
            },
            
            '+=' ::(other) {
                this.append(other);
            },
            
            '[]' : {
                get ::(index => Number) {   
                    when(index < 0 || index >= arrlen) error('Index out of bounds for string.');
                    when(hasStr) introspect.charAt(strsrc, index);
                    return introspect.charAt(String(this), index);
                },
                
                
                set ::(index => Number, b => String) {
                    when(index < 0 || index >= arrlen) error('Index out of bounds for string.');
                    arrsrc[index] = introspect.charCodeAt(b, 0);
                    hasStr = false;
                }
            }
        });
    }
});
return String_;
