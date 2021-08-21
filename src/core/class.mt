@pObject ::(o) {
    @already = {};
    @pspace ::(level) {
        @str = '';
        for([0, level], ::{
            str = str + ' ';
        });
        return str;
    };
    @helper ::(obj, level) {
        @poself = context;

        return match(introspect(obj).type()) {
            ('string') :    obj,
            ('number') : ''+obj,
            ('boolean'): ''+obj,
            ('empty')  : 'empty',
            ('object') : ::{
                when(already[obj] == true) '[already printed]';
                 
                @output = '{\n';
                foreach(obj, ::(key, val) {
                    output = output + pspace(level)+(AsString(key))+' : '+poself(val, level+1) + ',\n';
                });
                output = output + pspace(level) + '}\n';
                already[obj] = true;
                return output;                
            }()
        };
    };
    print(helper(o, 0));
};
@class ::(d) {
    // unfortunately, have to stick with these fake array things since we
    // are bootstrapping classes, which will be used to implement real Arrays.
    <@> arraylen ::(a){
        return introspect(a).keycount();
    };

    <@> arraypush ::(a, b){
        a[introspect(a).keycount()] = b;
    };

    <@> arrayclone ::(a) {
        <@> out = {};
        for([0, arraylen(a)], ::(i){
            out[i] = a[i];
        });
        return out;
    };




    <@> classinst = {};
    if(d.declare) d.declare();
    <@> define = d.define;

    classinst.interfaces = [];
    <@> allclass = classinst.interfaces;
    <@> inherits = if(d.inherits)::{
        <@> addbase ::(other){
            for([0, arraylen(other)], ::(n){
                <@>g = other[n];
                @found = false;
                for([0, arraylen(allclass)], ::(i) {
                    when(allclass[i] == g) ::{
                        found = true;
                        return arraylen(allclass);
                    }();
                });
                when(!found) ::{
                    arraypush(allclass, g);
                }();
            });
        };

        when(arraylen(inherits)) ::{
            for([0, arraylen(inherits)], ::(i){
                addbase(inherits[i].interfaces);
            });
            return d.inherits;
        }();
        addbase(inherits.interfaces);
    }();
    arraypush(allclass, classinst);

    // returns whether the isntance inherits from the given class.
    <@> isa ::(d) {
        @found = false;
        for([0, arraylen(allclass)], ::(i){
            when(allclass[i] == d)::{
                found = true;
                return arraylen(allclass);
            }();
        });
        return found;
    };

    classinst.new = ::(args, noseal, outsrc) {
        args = if(args)args else {};
        @out = outsrc;
        if(inherits) ::{
            out = if(out)out else {};
            if(inherits[1] == empty)::{
                for([1, arraylen(inherits)], ::(i){
                    inherits[i].new(args, out);
                });
            }() else ::{
                inherits.new(args, out);
            }();
        }();
        out = if(out) out else {};

        @setters;
        @getters;
        @funcs;
        @varnames;
        @mthnames;
        if(out.publicVars)::{
            setters = out.setters;
            getters = out.getters;
            funcs = out.funcs;
            mthnames = {
                inherited : out.publicMethods
            };

            varnames = {
                inherited : out.publicVars
            };
            out.publicVars = varnames;
            out.publicMethods = mthnames;
        }() else ::{
            setters = {};
            getters = {};
            funcs = {};
            varnames = {};
            mthnames = {};
            out.publicVars = varnames;
            out.publicMethods = mthnames;
            out.setters = setters;
            out.getters = getters;
            out.funcs = funcs;
        }();


        out.interface = ::(obj){
            <@> keys = introspect(obj).keys();
            foreach(obj, ::(key, v) {
                when(introspect(v).type() != 'object')::{
                    error("Class interfaces can only have getters/setters and methods.");
                }();
                if(introspect(v).isCallable())::{
                    funcs[key] = v;
                    mthnames[key] = 'function';
                    //print('ADDING CLASS function: ' + key);
                }() else ::{
                    setters[key] = v.set;
                    getters[key] = v.get;
                    varnames[key] = match((if(v.set) 1 else 0)+
                                          (if(v.get) 2 else 0)) {
                        (1) : 'Write-only',
                        (2) : 'Read-only',
                        (3) : 'Read/Write'
                    };
                }();
            });
        };

        define(out, args, classinst);
        removeKey(out, 'interface');



        if(noseal == empty) ::{
            out.accessor = ::(key) {
                when(key == 'introspect') {
                    public : {
                        variables : varnames,
                        methods : mthnames
                    }
                };
                when(key == 'isa') isa;

                @out = getters[key];
                when(out) out();

                out = funcs[key];
                when(out) out;

                @str = '';
                for([0, introspect(funcs).keycount()], ::(i) {
                    str = str + funcs[i] + ', ';
                });
                error('' +key+ " does not exist within this instances class." + str);
            };

            out.assigner = ::(key, value){
                @out = setters[key];
                when(out) out(value);
                error('' +key+ " does not exist within this instances class.");
            };
        }();
        return out;
    };
    return classinst;
};


@Array = class({
    define ::(this, args, classinst) {
        <@>data = if(AsBoolean(args) && introspect(args).type() == 'object') args else [];
         @ len = introspect(data).keycount();

        this.interface({
            // Read/write variable.
            // Returns/Sets the length of the array.
            length : {
                get :: {
                    return len;
                },
                
                set ::(newV) {
                    len = newV;
                    @o = data[len];
                    data[len] = o;
                }
            },

            // Function, 1 argument. 
            // No return value.
            // Adds an additional element to the array.
            push :: (nm) {
                data[len] = nm;
                len = len + 1;
            },

            // Function, 1 argument.
            // Returns.
            // Removes the element at the specified index and returns it.
            // Like all array operations, the specified index is 
            // 0-indexed.
            remove ::(nm) {
                when(nm < 0 || nm >= len) empty;
                @out = data[nm];
                removeKey(data, nm);
                return out;
            },


            // Function, no arguments.
            // Returns.
            // Creates a new array object that is a shallow copy of this one.
            clone :: {
                @out = classinst.new();
                for([0, len], ::(i){
                    out.push(data[i]);
                });
                return out;
            },

            // Function, one argument, optional.
            // No return value.
            // Re-arranges the contents of the array to be 
            // sorted according to the given comparator.
            // If no comparator is given, a default one using the 
            // "<" operator is used instead.
            sort ::(cmp) {
                // ensures a comparator exists if there isnt one.
                cmp = if(AsBoolean(cmp)) cmp else ::(a, b) {
                    when(a < b) -1;
                    when(a > b)  1;
                    return 0;
                };

                // casts to an integer
                @INT :: (n){
                    return introspect(n).floor();
                };

                // swaps 2 members of the data array
                @swap ::(ai, bi) {
                    @temp = data[ai];
                    data[ai] = data[bi];
                    data[bi] = temp;
                }; 

                // l/r pivots
                @left = 0;
                @right = len-1;


                @partition ::(left, right) {
                    @pivot = data[right];
                    @i = left-1;
                    
                    for([left, right], ::(j) {
                        when(cmp(data[j], pivot) < 0)::{
                            i = i + 1;
                            swap(i, j);
                        }();
                    });
                    swap(i+1, right);             
                    return i + 1;
                };

                @subsort :: (cmp, left, right) {
                    when(right <= left) empty;

                    @pi = partition(left, right);
                    
                    context(cmp, left, pi-1);
                    context(cmp, pi+1, right);
                };

                subsort(cmp, left, right);
            },

            // returns a new array thats a subset of
            subset ::(from, to) {
                @out = classinst.new();
                when(from >= to) out;
                
                from = if(from <  0)   0     else from;                
                to   = if(to   >= len) len-1 else from;                
                
                for([from, to+1], ::(i){
                    out.push(data[i]);
                });
                return out;
            },
            
            // Function, one argument.
            // Returns.
            // Returns a new array of all elements that meet the condition 
            // given. "cond" should be an array that accepts at least one argument.
            // The first argument is the value.
            filter ::(cond) {
                @out = classinst.new();
                foreach(data, ::(v){
                    when(cond(v)) out.push(v);
                });            
                return out;
            },
            
            
            // linear search
            find ::(inval) {
                @index = -1;
                for([0, len], ::(i){
                    when(data[i] == inval)::{
                        index = i;
                        return len;
                    }(); 
                });            
                return index;
            },
            
            binarySearch::(val, cmp) {
                error("not done yet");
            },




            toString ::{
                @str = '[';
                for([0, len], ::(i){
                    str = str + (AsString(data[i]));
                    when(i != len-1)::{
                        str = str + ', ';
                    }();
                });
                return str + ']';
            }
        });
    }
});


@String = class({
    define ::(this, args, classinst) {
        @str = if(args) AsString(args) else "";
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
                @out = '';
                from = if(from < 0  ) 0     else from;
                to   = if(to  >= len) len   else to+1;
                
                for([from, to], ::(i){
                    out = out + this.charAt(i);
                }); 
                return classinst.new(out);
            },

            split::(dl) {
                dl = if (introspect(dl).type() == 'string') String.new(dl) else dl;
                when(dl.length != 1)  error("Split expects a single-character string");
                 
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



@test = Array.new([4, 2, 6, 3, 10, 1, 0, -40, 349, 0, 10, 43]);
print(test);
test.sort();
print('' + test + '\n\n');


@t = String.new("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.");
print(t.split(' '));
print(t.count(' '));


