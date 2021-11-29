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
<@>_string_setcharat  = getExternalFunction("__matte_::string_setcharat");
<@>_string_setcharcodeat= getExternalFunction("__matte_::string_setcharcodeat");
<@>_string_append     = getExternalFunction("__matte_::string_append");
<@>_string_removechar = getExternalFunction("__matte_::string_removechar");
<@>_string_substr     = getExternalFunction("__matte_::string_substr");
<@>_string_split      = getExternalFunction("__matte_::string_split");
<@>_string_get_string = getExternalFunction("__matte_::string_get_string");
@String_ = class({
    name : 'Matte.Core.String',
    define ::(this, args, classinst) {
        <@>MatteString = introspect.type(this);
        <@>handle = _string_create(if (args != empty) String(args) else empty);  

        
        
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
                return classinst.new(_string_substr(handle, from, to));
            },

            split::(sub) {  
                @a = _string_split(handle, String(sub));
                @aconv = Array.new();
                foreach(a, ::(k, v) {
                    aconv.push(classinst.new(v));
                });
                return aconv;
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
                fmt = classinst.new(fmt);
                @tokens = fmt.split('[%]');
                @startsWith = (fmt.charAt(0) == '[' &&
                            fmt.charAt(1) == '%' &&
                            fmt.charAt(2) == ']');

                @len = fmt.length;
                @endsWith = (fmt.charAt(len-3) == '[' &&
                            fmt.charAt(len-2) == '%' &&
                            fmt.charAt(len-1) == ']');

                
                
                
                
                @out = Array.new();
                @searchIndex = 0;
                @lastIndex;

                loop(::{
                    @searchStr = this.substr(searchIndex, this.length-1);
                    @offset = 0;
                    @locs = [];

                    return listen(::{
                        for([0, tokens.length], ::(i) {
                            @localAt = searchStr.search(tokens[i]);
                            locs[i] = {
                                at: localAt + offset,
                                len: tokens[i].length
                            };


                            // if very first sub anchor,
                            // set the initial looping 
                            if (i == 0) ::<={
                                // couldnt find first anchor.... will not pass
                                if (localAt == -1) ::<={
                                    send(false);
                                };

                                // progress the initial iter
                                searchIndex = locs[0].at+locs[0].len;
                            };

                            // abort
                            if (localAt == -1) ::{
                                send(true);
                            };

                            offset = locs[i].at + locs[i].len;
                            // shortens the search path
                            searchStr = searchStr.substr(localAt+locs[i].len, searchStr.length-1);
                        });
                        // all anchors found. Populate find array.
                        if (startsWith) ::<={
                            out.push(this.substr(0, locs[0].at-1));
                        };

                        for([0, introspect.keycount(locs)-1], ::(i) {
                            out.push(this.substr(
                                locs[i  ].at+locs[i  ].len,
                                locs[i+1].at-1
                            ));
                        });


                        if (endsWith) ::<={
                            @loc = locs[introspect.keycount(locs)-1];
                            out.push(this.substr(loc.at + loc.len, this.length-1));
                        };


                        return false;

                    });
                });
                return out;
            }
            
 

        });
        
        this.attributes({
            (String) :: {
                return _string_get_string(handle);
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
                    return _string_charat(handle, index);
                },
                
                
                set ::(index => Number, b) {
                    _string_setcharat(handle, index, String(b));
                }
            }
        });
    }
});
return String_;
