<@>class = import('Matte.Class');
<@>Array = import('Matte.Array');
@String = class({
    name : 'Matte.String',
    define ::(this, args, classinst) {
        @str = if(args) String(args) else "";
        @intr = introspect(str);
        @len = intr.length();
        this.interface({
            length : {
                get :: { 
                    return len;
                }
            },
            
            data : {
                get ::{
                    return str;
                }
            },
            
            // returns the index at which the 
            // substring is at. If the substring is 
            // not contained within the string, -1 is returned.
            search::(sub){
                sub = if (introspect(sub).type() == 'string') String.new(sub) else sub;

                @index = -1;
                @sublen = sub.length;
                @self_is = intr;
                @sub_is = introspect(sub.data);

                when(sublen == 0) -1;
                
                @checksub::(i){
                    @found = true;
                    for([0, sublen], ::(j){
                        when(self_is.charCodeAt(i+j) !=
                              sub_is.charCodeAt(j)) ::{
                            found = false;
                            return sublen;
                        }();
                    });
                    return found;
                };
                for([0, len], ::(i){
                    when(checksub(i, sub)) ::{
                        index = i;
                        return len;
                    }();
                });
                
                return index;
            },
            
            contains::(sub) {
                return this.search(sub) != -1;
            },

            containsAny::(arr) {
                <@>vals = introspect(arr).values();
                <@>len = introspect(vals).keycount();
                @contains = false;
                for([0, len], ::(i) {
                    when (this.contains(vals[i])) ::{
                        contains = true;
                        return len;
                    }();
                });
                return contains;
            },


            replace::(sub, with) {
                with = if (introspect(with).type() == 'string') String.new(with) else with;
                sub  = if (introspect(sub).type()  == 'string') String.new(sub)  else sub;
                
                loop(::{
                    <@> index = this.search(sub);
                    when(index == -1) false;

                    str = intr.subset(0, index-1) 
                          + with.data 
                          + intr.subset(
                            index+sub.length, 
                            this.length-1
                          );
                    intr = introspect(str);
                    len = intr.length();
                    return true;
                });

            },
            
            count::(sub) {
                sub = if (introspect(sub).type() == 'string') String.new(sub) else sub;
            
                @count = 0;
                @sublen = sub.length;
                @self_is = intr;
                @sub_is = introspect(sub.data);
                
                @checksub::(i){
                    @found = true;
                    for([0, sublen], ::(j){
                        when(self_is.charCodeAt(i+j) !=
                              sub_is.charCodeAt(j)) ::{
                            found = false;
                            return sublen;
                        }();
                    });
                    return found;
                };
                for([0, len], ::(i){
                    when(checksub(i, sub)) ::{
                        count = count+1;
                    }();
                });
                
                return count;
            },
            
            charCodeAt::(i) {
                return intr.charCodeAt(i);  
            },

            charAt::(i) {
                return intr.charAt(i);  
            },

            removeChar::(i) {
                when(i < 0 || i > len) empty;

                (match(i) {
                    (0): ::{
                        str = intr.subset(1, len-1);
                    },

                    (len-1): ::{
                        str = intr.subset(0, len-2);
                    },

                    default: ::{
                        str = intr.subset(0, i-1) + intr.subset(i+1, len-1);
                    }
                })();
                intr = introspect(str);
                len = len-1;
            },
            
            valueize::{
                @out = Array.new();
                for([0, len], ::(i){
                    out.push(this.charCodeAt(i));
                });
                return out;
            },

            characterize::{
                @out = Array.new();
                for([0, len], ::(i){
                    out.push(this.charAt(i));
                });
                return out;
            },
            
            substr::(from, to) {
                return classinst.new(intr.substr(from, to));
            },

            split::(dl) {
                dl = if (introspect(dl).type() == 'string') String.new(dl) else dl;
                when(dl.length != 1)  error("Split expects a single-character string.");
                 
                @out = Array.new();
                @curstr = '';
                for([0, len], ::(i){
                    if (this.charAt(i) == dl.data) ::{
                        out.push(classinst.new(curstr));
                        curstr = '';
                    }() else ::{
                        curstr = curstr + this.charAt(i);
                    }();
                });
                out.push(classinst.new(curstr));
                return out;
            },

            toString :: {
                return str;
            }   
        });
    }
});

return String;
