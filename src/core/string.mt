<@>class = import('Matte.Class');
<@>Array = import('Matte.Array');
@String_ = class({
    name : 'Matte.String',
    define ::(this, args, classinst) {
        <@>MatteString = introspect.type(this);
        
        // ensure we can work with a basic string
        <@>TOSTRINGLITERAL ::(v) {
            return match(introspect.type(v)) {
                (String):      v,
                (Empty):       '',
                default:
                    String(v)
            };
        };

        
        <@>STRINGTOARR ::(s) {
            <@>intr = introspect;
            <@>out = [];
            <@>len = intr.length(s);
            for([0, len], ::(i){
                out[i] = intr.charCodeAt(s, i);
            });  
            return out;
        };


        
        
        @arrsrc = STRINGTOARR(TOSTRINGLITERAL(args));
        @arrintr = introspect;
        @arrlen = arrintr.keycount(arrsrc);
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
            arrlen = arrintr.keycount(arrsrc);

        };
        
        
        this.interface({

            onRevive ::(reviveArgs) {
                arrsrc = STRINGTOARR(TOSTRINGLITERAL(reviveArgs));
                arrlen = arrintr.keycount(arrsrc);
                strsrc = '';
                hasStr = false;
            },


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
                @intrsub = introspect;
                                
                @index = -1;
                @sublen = intrsub.keycount(sub);

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
                <@>vals = introspect.values(arr);
                <@>len = introspect.keycount(vals);
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

                @intrsub = introspect;                                
                @sublen = intrsub.keycount(sub);

                @intrwith = introspect;                                
                @withlen = intrwith.keycount(with);

                
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
                                removeKey(arrsrc, index+withlen);
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
                    arrlen = arrintr.keycount(arrsrc);


                    return true;
                });

            },
            
            count::(sub) {
                sub = STRINGTOARR(TOSTRINGLITERAL(sub));
                @intrsub = introspect;
                                
                @index = -1;
                @sublen = intrsub.keycount(sub);

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
                @intr = introspect;
                return intr.subset(String(this), i, i);  
            },
            
            append::(a) {
                (match(introspect.type(a)) {
                    (Number): ::{
                        arrsrc[arrlen] = a;
                    },
                    
                    default: ::{
                        @other = STRINGTOARR(TOSTRINGLITERAL(a));
                        for([0, introspect.keycount(other)], ::(i){
                            arrsrc[i+arrlen] = other[i];
                        });
                    }
                })();

                hasStr = false;
                strsrc = empty;
                arrlen = arrintr.keycount(arrsrc);                
            },
            
            setCharAt::(index => Number, a) {
                when(index < 0 || index >= arrlen) error('Can only replace a character in the current string. Index is out of bounds.');
                (match(introspect.type(a)) {
                    (Number): ::{
                        arrsrc[index] = a;
                    },
                    
                    default: ::{
                        @other = STRINGTOARR(TOSTRINGLITERAL(a));
                        when(introspect.keycount(other) != 1) error('Can only replace a single character: the string form of the given value is more than one character');
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
                arrlen = arrintr.keycount(arrsrc);
            },
            
            valueize::{
                return Array.new(arrsrc);
            },

            characterize::{
                @out = Array.new();
                @str = String(this);
                @strintr = introspect;
                for([0, arrlen], ::(i){
                    out.push(strintr.charAt(str, i));
                });
                return out;
            },
            
            substr::(from => Number, to => Number) {
                return classinst.new(introspect.subset(String(this), from, to));
            },

            split::(sub) {  
                sub = STRINGTOARR(TOSTRINGLITERAL(sub));
                @intrsub = introspect;
                                
                @index = -1;
                @sublen = intrsub.keycount(sub);
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
                        if (cursub.length) ::{
                            outarr.push(cursub);
                            cursub = classinst.new();
                        }();
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
            
                // converts a format string into a format array.
                // Format arrays consist of integer anchor IDs and 
                // charcode arrays.
                //
                // Throws error if:
                // - format includes to anchors [%] unseparated by a string.
                // - format is empty.
                // - format contains only an anchor 
                // - format contains only no anchors
                <@>markify ::(fmtsrc) {
                    @fmtstrl = TOSTRINGLITERAL(fmtsrc);
                    @fmtarr = STRINGTOARR(fmtstrl);
                    @fmtlen = introspect.keycount(fmtarr);
                    <@>output = [];
                    <@>beginsMatch = fmtarr[0]== 91 &&
                                     fmtarr[1] == 37 &&
                                     fmtarr[2] == 93;

                    <@>endsMatch = fmtarr[fmtlen-1] == 93 &&
                                   fmtarr[fmtlen-2] == 37 &&
                                   fmtarr[fmtlen-3] == 91;
                    @anchorIndex = 0;                       
                    @contentIndex = 0;
                    if (beginsMatch) ::{
                        output[contentIndex] = 0;
                        contentIndex =1;
                        anchorIndex = 1;   
                    }();              
                       
                    @tokens = classinst.new(fmtstrl).split('[%]');
                    for([0, tokens.length], ::(i) {
                        output[contentIndex] = STRINGTOARR(String(tokens[i]));
                        contentIndex+=1;

                        if (i < tokens.length-1 || (i == tokens.length-1 && endsMatch)) ::{
                            output[contentIndex] = anchorIndex;
                            anchorIndex += 1;
                            contentIndex += 1;                        
                        }();
                    });
                    
                    @output_intr = introspect;

                    // Check empty
                    when(output_intr.keycount(output) == 0) 
                        error('MatteString.scan format string does not contain any anchors or is empty.');

    
                    // Check toggle
                    @isNum = beginsMatch;
                    for([0, contentIndex], ::(i){
                        when(introspect.type(output[i]) != (if(isNum) Number else Object)) error('MatteString.scan format string is in an invalid format.');
                        isNum = !isNum;
                    });
                   
                    return output;    
                };
            
            
                // sees if a can be found within b.
                // -1 if not found, else the offset of b
                // where a was first found.
                <@>findsubstr :: (offset, a, alen, b, blen) {
                    @found = -1;
                    for([offset, blen], ::(i) {
                        @matches = true;
                        
                        for([0, alen], ::(n){
                            when(b[i+n] != a[n]) ::{
                                matches = false;
                                return alen;
                            }();
                        });                                 
                        
                        
                        when(matches) ::{
                            found = i;
                            return blen;
                        }();
                    });
                    return found;
                };
            
                // Finds an anchored string ([%]), in the configuration of:
                //
                //   [%]anchor          -> find_A
                //   anchor[%]anchor    -> findA_B
                //   anchor[%]          -> findA_
                //
                // Whats returned is an object with the 
                // following attributes:
                // {
                //      // Of type Number, returns the offset 
                //      // Within strArr where the anchored string ends in strArr
                //      offset,
                //      // A String of the anchored content string
                //      content
                // }
                //
                // Arguments:
                //
                // strArr -> this string, but as a charcode array 
                // offset -> where to start "find"ing from within strArr
                // anchorArr(A|B) -> anchor charcode arrays
                <@>find_A ::(
                    strArr,             
                    anchorArr
                ) {
                    @strArrLen = introspect.keycount(strArr);
                    @anchorArrLen = introspect.keycount(anchorArr);                    

                    @content = classinst.new();
                    @where = findsubstr(0, anchorArr, anchorArrLen, strArr, strArrLen);                    

                    // couldnt find
                    when(where==-1) empty;
                    
                    
                    
                    @contentVals = introspect.subset(strArr, 0, where-1);                                       
                    for([0, where], ::(i){
                        content.append(contentVals[i]);
                    });

                    return {
                        content: content,
                        offset: where
                    };
                };
                <@>findA_B ::(
                    strArr,             //<- full string array 
                    offset,   //<- 
                    
                    anchorArrA,
                    anchorArrB
                ) {
                    @strArrLen = introspect.keycount(strArr);
                    @anchorArrALen = introspect.keycount(anchorArrA);                    
                    @anchorArrBLen = introspect.keycount(anchorArrB);                    

                    @content = classinst.new();
                    @where0 = findsubstr(offset, anchorArrA, anchorArrALen, strArr, strArrLen); 
                    // first anchor not found
                    when(where0 == -1) empty;
                    where0 += anchorArrALen;
                    @where1 = findsubstr(where0, anchorArrB, anchorArrBLen, strArr, strArrLen);                                        
                    when(where1 == -1) empty;

                    @contentVals = introspect.subset(strArr, where0, where1-1);                                       
                    for([0, where1 - where0], ::(i){
                        content.append(contentVals[i]);
                    });

                    return {
                        content: content,
                        offset: where1
                    };
                };
                <@>findA_ ::(
                    strArr,             //<- full string array 
                    offset,   //<- 
                    
                    anchorArr
                ) {
                    @strArrLen = introspect.keycount(strArr);
                    @anchorArrLen = introspect.keycount(anchorArr);                    

                    @content = classinst.new();
                    @where = findsubstr(offset, anchorArr, anchorArrLen, strArr, strArrLen);                    

                    // couldnt find
                    when(where==-1) empty;
                    
                    
                    
                    @contentVals = introspect.subset(strArr, where, strArrLen-1);                                       
                    for([0, strArrLen-where], ::(i){
                        content.append(contentVals[i]);
                    });

                    return {
                        content: content,
                        offset: where
                    };
                };


                @outputArray = Array.new();
                @offset = 0;
                @success = true;
                @fmtArr = markify(fmt);
                <@>len = introspect.keycount(fmtArr);
                
                loop(::{
                    @foundFirst = false;
                    for([0, len], ::(i){
                        return (match(true) {
                
                            // [%]string. Always first!
                            (introspect.type(fmtArr[i]) == Number): ::{
                                <@>result = find_A(arrsrc, fmtArr[i+1]);
                                // end the loop, couldnt find required token
                                when(result == empty) ::{
                                    success = false;
                                    return len;
                                }();
                                foundFirst = true;
                                outputArray.push(result.content);
                                offset = result.offset; // where the next token begins.
                                return i+1;
                            },
                            

                            // string[%]string
                            (i+2 < len): ::{
                                <@>result = findA_B(arrsrc, offset, fmtArr[i], fmtArr[i+2]);
                                // end the loop, couldnt find required token
                                when(result == empty) ::{
                                    success = false;
                                    return len;
                                }();
                                foundFirst = true;                                
                                outputArray.push(result.content);
                                offset = result.offset; // where the next token begins.                        
                                return i+2;
                            },

                            // only 1 item, end.
                            (len - i == 1): ::{
                                return len;
                            },

                            // string[%]
                            default: ::{
                                <@>result = findA_(arrsrc, offset, fmtArr[i]);
                                // end the loop, couldnt find required token
                                when(result == empty) ::{
                                    success = false;
                                    return len;
                                }();                                
                                foundFirst = true;
                                outputArray.push(result.content);
                                offset = result.offset; // where the next token begins.                                                
                                return len;
                            }
                        })();
                    });
                    when(success) false;
                    when(!foundFirst) false;
                    
                    return true;
                });

                return outputArray;            
            },
            
            
            // removes
            trim::(chars) {
            
            }

        });
        
        this.operator({
            (String) :: {
                when(hasStr) strsrc;
                strsrc = arrintr.arrayToString(arrsrc);
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
