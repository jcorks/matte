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
                (Empty):       '',
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
                },
                
                set ::(newlen => Number) {
                    // already good
                    when(arrlen == newlen) empty;
                    
                    // needs to grow
                    for([arrlen, newlen], ::(i) {
                        arrsrc[i] = 32;
                        arrlen+=1;
                    }); 
                    
                    // need to shrink!
                    for([arrlen, newlen, -1], ::(i) {
                        removeKey(arrsrc, arrlen-1);
                        arrlen-=1;
                    });
                    
                    hasStr = false;
                    strsrc = empty;
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
                when(i < 0 || i >= arrlen) error('Given index is not within the length of the string');
                return arrsrc[i];  
            },

            charAt::(i) {
                @intr = introspect(String(this));
                return intr.subset(i, i);  
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
            
            setCharAt::(index => Number, a) {
                when(index < 0 || index >= arrlen) error('Can only replace a character in the current string. Index is out of bounds.');
                (match(introspect(a).type()) {
                    (Number): ::{
                        arrsrc[index] = a;
                    },
                    
                    default: ::{
                        @other = STRINGTOARR(TOSTRINGLITERAL(a));
                        when(introspect(other).keycount() != 1) error('Can only replace a single character: the string form of the given value is more than one character');
                        arrsrc[index] = a[0];
                    }
                })();
                
                hasStr = false;
                strsrc = empty;
            },

            removeChar::(i => Number) {
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
                @str = String(this);
                @strintr = introspect(str);
                for([0, arrlen], ::(i){
                    out.push(strintr.charAt(i));
                });
                return out;
            },
            
            substr::(from => Number, to => Number) {
                return classinst.new(introspect(String(this)).subset(from, to));
            },

            split::(sub) {  
                sub = STRINGTOARR(TOSTRINGLITERAL(sub));
                @intrsub = introspect(sub);
                                
                @index = -1;
                @sublen = intrsub.keycount();
                @outarr = Array.new();

                when(sublen == 0 || arrlen == 0) ::{
                    outarr.push(classinst(this));
                    return outarr;
                }();

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

                @cursub = classinst.new();
                for([0, arrlen], ::(i){
                    when(checksub(i, sub)) ::{
                        outarr.push(cursub);
                        cursub = classinst.new();
                        return i+sublen; // skip sub
                    }();
                    cursub.append(arrsrc[i]);
                });
                outarr.push(cursub);
                
                return outarr;
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
                // % == 37 
                // [ == 91
                // ] == 93
                @beginsMatch = -1;
                @endsMatch = -1;

                fmt = classinst.new(TOSTRINGLITERAL(fmt));
                @intrfmt = introspect(fmt);
                <@>fmtlen = intrfmt.keycount();

                // needs to hold at least one key for results to be valid
                when(fmtlen <= 3) empty;                

                beginsMatch = fmt[0] == 91 &&
                              fmt[1] == 37 &&
                              fmt[2] == 93;

                <@>tokens = fmt.split('[@]');
                @tokenArrays = Array.new();
                foreach(tokens, ::(k, v) {
                    tokenArrays.push(String(STRINGTOARR(v)));
                }

                @arrayOut = Array.new();

                @curString = classinst.new();
                @preString = classinst.new();
                for([0, arrlen], ::(i) {
                    @iter = i;
                    @curtoken;
                    @pass = true;
                    @curTokenIter = 0;
                    
                    for([i, arrlen], ::(k) {
                        @iter = k;
                        curtoken = tokenArrays[curTokenIter];
                        for([0, curtoken.length], ::(n) {
                            when(curtoken[n] != arrsrc[iter]) ::{
                                curString.append(arrsrc[k]);
                                pass = false;
                                return curtoken.length;
                            }();
                            iter += 1;
                        });
                        
                        // try next token
                        when(!pass) empty
                        // token matches. 


                        if (curTokenIter == 0) :: {
                            arrayOut = Array.new();

                            // if the fmt starts with a [%] matcher
                            // record now!
                            if (beginsMatch) ::{
                                arrayOut.push(preString);
                            }();    
                        }() else ::{
                            arrarOut.push(curString);    
                        }();
                        curString = classinst.new();
                    });

                    preString.append(arrsrc[i]);
                });                
                
                when array
                return arrayOut;

            },
            
            
            // removes
            trim::(chars) {
            
            }

        });
        
        this.operator({
            (String) :: {
                when(hasStr) strsrc;
                strsrc = arrintr.arrayToString();
                hasStr = true;
                return strsrc;
            },

            'foreach' :: {
                return error('Cannot do foreach on a Matte.String.');
            },
            
            '+' :: (other){
                @out = classinst.new(String(this));
                out.append(other);
                return out;
            },
            
            '+=' ::(other) {
                this.append(other);
            }
        });
    }
});

return String_;
