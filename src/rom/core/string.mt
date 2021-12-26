<@>class = import(module:'Matte.Core.Class');
<@>Array = import(module:'Matte.Core.Array');

<@>_string_create     = getExternalFunction(name:"__matte_::string_create");
<@>_string_get_length = getExternalFunction(name:"__matte_::string_get_length");
<@>_string_set_length = getExternalFunction(name:"__matte_::string_set_length");
<@>_string_search     = getExternalFunction(name:"__matte_::string_search");
<@>_string_replace    = getExternalFunction(name:"__matte_::string_replace");
<@>_string_count      = getExternalFunction(name:"__matte_::string_count");
<@>_string_charcodeat = getExternalFunction(name:"__matte_::string_charcodeat");
<@>_string_charat     = getExternalFunction(name:"__matte_::string_charat");
<@>_string_setcharat  = getExternalFunction(name:"__matte_::string_setcharat");
<@>_string_setcharcodeat= getExternalFunction(name:"__matte_::string_setcharcodeat");
<@>_string_append     = getExternalFunction(name:"__matte_::string_append");
<@>_string_removechar = getExternalFunction(name:"__matte_::string_removechar");
<@>_string_substr     = getExternalFunction(name:"__matte_::string_substr");
<@>_string_split      = getExternalFunction(name:"__matte_::string_split");
<@>_string_get_string = getExternalFunction(name:"__matte_::string_get_string");
@String_ = class(info:{
    name : 'Matte.Core.String',
    define::(this) {
        <@>MatteString = introspect.type(of:this);

        @handle;
        this.constructor = ::(from) {
            if (handle == empty) ::<= {
                handle = _string_create(a:if (from != empty) String(from:from) else empty); 
            } else ::<={
                _string_set_length(a:handle, b:0);
                if (from != empty) ::<={
                    _string_append(a:handle, b:String(from:from));
                };
            };
            return this;
        };

        this.recycle = true;
        
        this.interface = {

            length : {
                get :: { 
                    return _string_get_length(a:handle);
                },
                
                set ::(value => Number) {
                    _string_set_length(a:handle, b:value);
                }
            },
            

            
            // returns the index at which the 
            // substring is at. If the substring is 
            // not contained within the string, -1 is returned.
            search::(key){
                return _string_search(a:handle, b:String(from:key));     
            },
            
            contains::(key) {
                return this.search(key:key) != -1;
            },


            containsAny::(keys => Object) {
                <@>vals = introspect.values(of:keys);
                <@>len = introspect.keycount(of:vals);
                return listen(to:::{
                    for(in:[0, len], do:::(i) {
                        if (this.contains(key:keys[i])) ::<={
                            send(message:true);
                        };
                    });
                    return false;                    
                });

            },


            replace::(key, with) {
                return _string_replace(a:handle, b:String(from:key), c:String(from:with));
            },
            
            count::(key) {
                return _string_count(a:handle, b:String(from:key));
            },
            
            charCodeAt::(index) {
                return _string_charcodeat(a:handle, b:Number(from:index));
            },

            charAt::(index) {
                return _string_charat(a:handle, b:Number(from:index));
            },
                        
            setCharAt::(index => Number, value) {
                return _string_setcharat(a:handle, b:index, c:String(from:value));
            },

            setCharCodeAt::(index => Number, a) {
                return _string_setcharcodeat(a:handle, b:index, c:Number(from:a));
            },
            
            append::(other) {
                _string_append(a:handle, b:String(from:other));
            },



            removeChar::(index) {
                _string_removechar(a:handle, b:Number(from:index));
            },
            
            
            substr::(from => Number, to => Number) {
                return this.class.new(from:_string_substr(a:handle, b:from, c:to));
            },

            split::(token) {  
                @a = _string_split(a:handle, b:String(from:token));
                @aconv = Array.new();
                foreach(in:a, do:::(k, v) {
                    aconv.push(value:this.class.new(from:v));
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
            scan::(format) {
                format = this.class.new(from:format);
                @tokens = format.split(token:'[%]');
                @startsWith = (format.charAt(index:0) == '[' &&
                            format.charAt(index:1) == '%' &&
                            format.charAt(index:2) == ']');

                @len = format.length;
                @endsWith = (format.charAt(index:len-3) == '[' &&
                            format.charAt(index:len-2) == '%' &&
                            format.charAt(index:len-1) == ']');

                
                
                
                
                @out = Array.new();
                @searchIndex = 0;
                @lastIndex;

                loop(func:::{
                    @searchStr = this.substr(from:searchIndex, to:this.length-1);
                    @offset = 0;
                    @locs = [];

                    return listen(to:::{
                        for(in:[0, tokens.length], do:::(i) {
                            @localAt = searchStr.search(key:tokens[i]);
                            locs[i] = {
                                at: localAt + offset,
                                len: tokens[i].length
                            };


                            // if very first sub anchor,
                            // set the initial looping 
                            if (i == 0) ::<={
                                // couldnt find first anchor.... will not pass
                                if (localAt == -1) ::<={
                                    send(message:false);
                                };

                                // progress the initial iter
                                searchIndex = locs[0].at+locs[0].len;
                            };

                            // abort
                            if (localAt == -1) ::{
                                send(message:true);
                            };

                            offset = locs[i].at + locs[i].len;
                            // shortens the search path
                            searchStr = searchStr.substr(from:localAt+locs[i].len, to:searchStr.length-1);
                        });
                        // all anchors found. Populate find array.
                        if (startsWith) ::<={
                            out.push(value:this.substr(from:0, to:locs[0].at-1));
                        };

                        for(in:[0, introspect.keycount(of:locs)-1], do:::(i) {
                            out.push(value:this.substr(
                                from: locs[i  ].at+locs[i  ].len,
                                to:   locs[i+1].at-1
                            ));
                        });


                        if (endsWith) ::<={
                            @loc = locs[introspect.keycount(of:locs)-1];
                            out.push(value: this.substr(from:loc.at + loc.len, to: this.length-1));
                        };


                        return false;

                    });
                });
                return out;
            }
            
 

        };
        
        this.attributes = {
            (String) :: {
                return _string_get_string(a:handle);
            },

            'foreach' :: {
                @a = [];
                for(in:[0, this.length], do:::(i) {
                    a[i] = this.charAt(index:i);
                });
                return a;
            },
            
            '+' :: (value){
                @out = this.class.new();
                out.append(other:String(from:this));
                return out;
            },
            
            '+=' ::(value) {
                this.append(other:value);
            },
            
            '[]' : {
                get ::(value => Number) {   
                    return _string_charat(a:handle, b:value);
                },
                
                
                set ::(key => Number, value) {
                    _string_setcharat(a:handle, b:key, c:String(from:value));
                }
            }
        };
    }
});

// boostrap
::<={
    @a = [];
    @acount = 0;
    for(in:[0, 5], do:::(i) {
        a[acount] = String_.new();    
        acount += 1;
    });
};
return String_;
