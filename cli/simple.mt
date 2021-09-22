<@>class = import('Matte.Class');
<@>Array = import('Matte.Array');
@String_ = class({
    name : 'Matte.String',
    define ::(this, args, classinst) {
        <@>MatteString = introspect(this).type();
        
        // ensure we can work with a basic string
        <@>TOSTRINGLITERAL ::(v) {
            return match(introspect(v).type()) {
                (String):      v,
                (MatteString): v.data,
                default:
                    String(v)
            };
        };

        
        <@>STRINGTOARR ::(s) {
            <@>intr = introspect(s);
            <@>out = [];
            <@>len = intr.length();
            for([0, len], ::(i){
                out[i] = intr.charCodeAt(i);
            });  
            return out;
        };


        
        
        @arrsrc = STRINGTOARR(TOSTRINGLITERAL(args));
        @arrintr = introspect(arrsrc);
        @arrlen = arrintr.keycount();
        @strsrc;
        @hasStr = false;
        
        
        // creates space in arrsrc at the 
        // given index for howMany new entries.
        <@>moveUp ::(at, howMany) {
            @temp;
            for([arrlen-1, at-1, -1], ::(i){
                temp = arrsrc[i+howMany];
                arrsrc[i+howMany] = arrsrc[i]; 
            });
            arrlen = arrintr.keycount();

        };
        
        
        this.interface({
            length : {
                get :: { 
                    return arrlen;
                }
            },
            

            
            // returns the index at which the 
            // substring is at. If the substring is 
            // not contained within the string, -1 is returned.
            search::(sub){
                sub = STRINGTOARR(TOSTRINGLITERAL(sub));
                @intrsub = introspect(sub);
                                
                @index = -1;
                @sublen = intrsub.keycount();

                when(sublen == 0) -1;
                
                @checksub::(i){
                    @found = true;
                    for([0, sublen], ::(j){
                        when(arrsrc[i+j] !=
                                sub[j]) ::{
                            found = false;
                            return sublen;
                        }();
                    });
                    return found;
                };
                for([0, arrlen], ::(i){
                    when(checksub(i, sub)) ::{
                        index = i;
                        return arrlen;
                    }();
                });
                
                return index;
            },
            
            contains::(sub) {
                return this.search(sub) != -1;
            },

            containsAny::(arr => Object) {
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
                with = STRINGTOARR(TOSTRINGLITERAL(with));
                sub  = STRINGTOARR(TOSTRINGLITERAL(sub));

                @intrsub = introspect(sub);                                
                @sublen = intrsub.keycount();

                @intrwith = introspect(with);                                
                @withlen = intrwith.keycount();

                
                @checksub::(i){
                    @found = true;
                    for([0, sublen], ::(j){
                        when(arrsrc[i+j] !=
                                sub[j]) ::{
                            found = false;
                            return sublen;
                        }();
                    });
                    return found;
                };


                @index = -1;
                loop(::{
                    for([0, arrlen], ::(i){
                        when(checksub(i, sub)) ::{
                            index = i;
                            return arrlen;
                        }();
                    });     
                               
                    when(index == -1) false;

                    // found a match. Replace.
                    (match(true) {
                        // new string is smaller
                        // cells need to be removed.
                        (withlen < sublen): ::{
                            for([0, withlen], ::(i) {
                                arrsrc[i+index] = with[i];
                            });
                            for([withlen, sublen], ::(i) {
                                removeKey(arrsrc, withlen);
                            });
                        },


                        // newstring is larger. This requires 
                        // insertion, which is not as efficient currently.                        
                        (withlen > sublen): :: {
                            moveUp(index, withlen-sublen);
                            for([0, withlen], ::(i) {
                                arrsrc[i+index] = with[i];
                            });                                                    
                        },
                        
                        default: ::{ // most efficient case
                            for([0, sublen], ::(i) {
                                arrsrc[i+index] = with[i];
                            });
                        }
                    })();
                    index = -1;

                    // update internal stuff since array changed
                    hasStr = false;
                    strsrc = empty;
                    arrlen = arrintr.keycount();


                    return true;
                });

            },
            
            count::(sub) {
                sub = STRINGTOARR(TOSTRINGLITERAL(sub));
                @intrsub = introspect(sub);
                                
                @index = -1;
                @sublen = intrsub.keycount();

                when(sublen == 0) -1;
                
                @checksub::(i){
                    @found = true;
                    for([0, sublen], ::(j){
                        when(arrsrc[i+j] !=
                                sub[j]) ::{
                            found = false;
                            return sublen;
                        }();
                    });
                    return found;
                };
                @count = 0;
                for([0, arrlen], ::(i){
                    when(checksub(i, sub)) ::{
                        count += 1;
                    }();
                });
                
                return count;
            },
            
            charCodeAt::(i) {
                return arrsrc[i];  
            },

            charAt::(i) {
                return introspect([arrsrc[i]]).arrayToString();  
            },
            
            append::(a) {
                (match(introspect(a).type()) {
                    (Number): ::{
                        arrsrc[arrlen] = a;
                    },
                    
                    default: ::{
                        @other = STRINGTOARR(TOSTRINGLITERAL(a));
                        for([0, introspect(other).keycount()], ::(i){
                            arrsrc[i+arrlen] = other[i];
                        });
                    }
                })();

                hasStr = false;
                strsrc = empty;
                arrlen = arrintr.keycount();                
            },

            removeChar::(i) {
                when(i < 0 || i > arrlen) empty;

                removeKey(arrsrc, i);
                hasStr = false;
                strsrc = empty;
                arrlen = arrintr.keycount();
            },
            
            valueize::{
                return Array.new(arrsrc);
            },

            characterize::{
                @out = Array.new();
                for([0, arrlen], ::(i){
                    out.push(this.charAt(i));
                });
                return out;
            },
            
            substr::(from, to) {
                return classinst.new(arrintr.substr(from, to));
            },

            split::(dl) {                
                dl = TOSTRINGLITERAL(dl);
                <@>dlint = introspect(dl);
                when(dlint.length() != 1) error("Split expects a single-character string.");
                dl = dlint.dcharCodeAt(0);

                 
                @out = Array.new();
                @curstr = classinst.new();
                for([0, arrlen], ::(i){
                    if (arrsrc[i] == dl) ::{
                        out.push(classinst.new(curstr));
                        curstr = classinst.new();
                    }() else ::{
                        curstr.append(arrsrc[i]);
                    }();
                });
                out.push(curstr);
                return out;
            },
            
            operator : {
                (String) :: {
                    when(hasStr) strsrc;
                    strsrc = arrintr.arrayToString();
                    hasStr = true;
                    return strsrc;
                },
                
                '[]' ::(index) {
                    return this.charAt(index);
                },
                
                '+=' ::(other) {
                    this.append(other);
                }
            }

        });
    }
});


@a = String_.new('Test');
for([0, 10], ::(i){
    a += String(i);
});
a.replace('t01', 'YYY');
return a;


