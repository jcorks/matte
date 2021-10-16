
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
    classinst.typeobj = newtype({'name' : d.name});
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
        @out = outsrc;
        if(inherits) ::{
            @types = [];
            for([0, arraylen(inherits)], ::(i){
                types[i] = inherits[i].typeobj;
            });
        
            out = if(out)out else instantiate({inherits:types});
            if(inherits[0] == empty)::{
                for([0, arraylen(inherits)], ::(i){
                    inherits[i].new(args, true, out);
                });
            }() else ::{
                inherits.new(args, true, out);
            }();
        }();
        out = if(out) out else instantiate(classinst.typeobj);

        @setters;
        @getters;
        @funcs;
        @varnames;
        @mthnames; 
        @ops;
        if(out.publicVars)::{
            setters = out.setters;
            getters = out.getters;
            funcs = out.funcs;
            ops = out.ops;
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
            ops = {};
            out.ops = ops;
            out.publicVars = varnames;
            out.publicMethods = mthnames;
            out.setters = setters;
            out.getters = getters;
            out.funcs = funcs;
        }();


        out.interface = ::(obj){
            <@> keys = introspect(obj).keys();
            foreach(obj, ::(key, v) {
                when(introspect(v).type() != Object)::{
                    error("Class interfaces can only have getters/setters and methods. (has type: " + introspect(v).type() + ")");
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
        
        out.operator = ::(obj) {
            foreach(obj, ::(k, v){
                ops[k] = v;            
            });
        };


        define(out, args, classinst);
        removeKey(out, 'interface');



        if(noseal == empty) ::{
            getters['introspect'] = ::{
                return {
                    public : {
                        variables : varnames,
                        methods : mthnames
                    }
                };
            };
            
            
            getters['isa'] = ::{
                return isa;            
            };
            ops['.'] = {
            
                get ::(key) {

                    @out = getters[key];
                    when(out) out();

                    out = funcs[key];
                    when(out) out;

                    error('' +key+ " does not exist within this instances class.");
                },

                set ::(key, value){
                    @out = setters[key];
                    when(out) out(value);
                    error('' +key+ " does not exist within this instances class.");
                }
            };
            
            setOperator(out, ops);
        }();
        return out;
    };
    return classinst;
};
return class;






/*
@test = Array.new([4, 2, 6, 3, 10, 1, 0, -40, 349, 0, 10, 43]);
print(test);
test.sort();
print('' + test + '\n\n');


@t = String.new("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.");
print(t.split(' '));
print(t.count(' '));
*/

