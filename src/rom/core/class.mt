
@class ::(definition) {
    // unfortunately, have to stick with these fake array things since we
    // are bootstrapping classes, which will be used to implement real Arrays.
    <@> arraylen ::(a){
        return introspect.keycount(of:a);
    };

    <@> arraypush ::(a, b){
        a[introspect.keycount(of:a)] = b;
    };

    <@> arrayclone ::(a) {
        <@> out = {};
        for(in:[0, arraylen(a:a)], do:::(i){
            out[i] = a[i];
        });
        return out;
    };

    @type = newtype(name : defintion.name);
    @classinst = {};

    // the actual instantiate call.
    @userInstantiate = definition.instantiate;
    setAttributes(
        of         : classinst,
        attributes : {
            '.' : {
                get ::(key => String) => Function {
                    if (key == 'new'){
                        @newinst = instantiate(type:classinst.type);
                        userInstantiate(this:newinst, thisClass:classinst);

                        
                        @interface = {};
                        if (newinst.interface != empty) ::<={
                            foreach(in:newinst.interface, do:::(k => String, v) {
                                if (introspect.type(of:v) == Function) ::<= {
                                    interface[k] = {
                                        isFunction : true,
                                        fn : v;
                                    };
                                } else ::<={
                                    interface[k] = {
                                        isFunction : false,
                                        get : v.get,
                                        set : v.set
                                    };                                    
                                };
                            });
                        }



                        @attribs;
                        if (newinst.attributes == empty) ::<={
                            attribs = {};
                        } else ::<= {
                            attribs = newinst.attributes;
                        }

                        attribs['.'] = {
                            get:: (key) {
                                @result = interface[key];
                                when(result == empty) 
                                    error('' +key+ " does not exist within this instance's class.");
                                
                                when(result.isFunction) result.fn;
                                @get = result.get;
                                when(get == empty) error('The attribute is not readable.');
                                return get();
                            },


                            set:: (key, value) {
                                @result = interface[key];
                                when(result == empty) 
                                    error('' +key+ " does not exist within this instance's class.");
                                
                                when(result.isFunction) error('Interface functions cannot be overwritten.');
                                @set = result.set;
                                when(set == empty) error('The attribute is not writable.');
                                return set(value:value);
                            }
                        };
                            

                        setAttributes(
                            of: newinst,
                            attributes : attribs
                        );


                        when(newinst.constructor != empty) return newinst.constructor;
                        return ::{return newinst};
                    }
                }
            }
        }
    );

    return classinst;




    /*

    <@> classinst = {};
    <@> define = d.define;

    classinst.interfaces = [];
    <@> allclass = classinst.interfaces;
    <@> inherits = if(d.inherits)::{
        <@> types = [];
        <@> addbase ::(other){
            for(in:[0, arraylen(a:other)], ::(n){
                <@>g = other[n];

                if(listen(to:::{
                    for([0, arraylen(a:allclass)], ::(i) {
                        if (allclass[i] == g) ::<={
                            send(message:true);                        
                        };                
                    });
                    return false;                
                })) ::<={
                    arraypush(a:allclass, b:g);            
                };
            });
        };

        when(arraylen(a:d.inherits)) ::{
            for([0, arraylen(a:d.inherits)], ::(i){
                arraypush(a:types, b:d.inherits[i].type);
                addbase(d.inherits[i].interfaces);
            });
            classinst.type = newtype(name: d.name, inherits:types);
            return d.inherits;
        }();
        classinst.type = newtype(name: d.name};
        addbase(d.inherits.interfaces);
        return d.inherits;
    }() else ::{
        classinst.type = newtype(name: d.name});
    }();
    arraypush(a:allclass, b:classinst);

    if(d.declare) d.declare();


    @dormant = [];
    @dormantCount = 0;
    classinst.new = ::(args, refs, outsrc) {
        
        when(dormantCount > 0)::<={
            @out = dormant[dormantCount-1];
            removeKey(from:dormant, key:dormantCount-1);
            dormantCount-=1;
            for([0, arraylen(a:out.onRevive)], ::(i){
                out.onRevive[i](args);
            });          

            @bops = getAttributes(out);
            bops.preserver = ::{
                dormant[dormantCount] = out;
                dormantCount += 1;
            };     
            setAttributes(out, bops);
                                                      

            return out;
        };
        
        if (refs == empty) ::<={
            refs = {};
        };
        @out = outsrc;
        if(inherits) ::<={
            out = if(out)out else instantiate(classinst.type);
            if(inherits[0] != empty)::<={
                for([0, arraylen(inherits)], ::(i){
                    inherits[i].new(args, refs, out);
                });
            } else ::<={
                inherits.new(args, refs, out);
            };
        };
        out = if(out) out else instantiate(classinst.type);

        @setters;
        @getters;
        @funcs;
        @varnames;
        @mthnames; 
        @ops;
        @onRevive;
        if(introspect.keycount(refs))::<={
            setters = refs.setters;
            getters = refs.getters;
            funcs = refs.funcs;
            ops = refs.ops;
            onRevive = refs.onRevive;
            mthnames = {
                inherited : refs.publicMethods
            };

            varnames = {
                inherited : refs.publicVars
            };
            refs.publicVars = varnames;
            refs.publicMethods = mthnames;
        } else ::<={
            setters = {};
            getters = {};
            funcs = {};
            varnames = {};
            mthnames = {};
            ops = {};
            onRevive = [];
            refs.ops = ops;
            refs.publicVars = varnames;
            refs.publicMethods = mthnames;
            refs.setters = setters;
            refs.getters = getters;
            refs.funcs = funcs;
            refs.onRevive = onRevive;

            getters['introspect'] = ::{
                return {
                    public : {
                        variables : varnames,
                        methods : mthnames
                    }
                };
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

            if (introspect.keycount(onRevive) > 0) ::<={
                ops.preserver =::{
                    dormant[dormantCount] = out;
                    dormantCount += 1;
                };
            };
            setAttributes(out, ops);
        };


        funcs.interface = ::(obj){
            <@> keys = introspect.keys(obj);
            foreach(obj, ::(key, v) {
                //when(introspect.type(v) != Object && introspect.type(v) != Function)::<={
                //    error("Class interfaces can only have getters/setters and methods. (has type: " + introspect.type(v) + ")");
                //};
                
                if(introspect.isCallable(v))::{
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
        
        funcs.attributes = ::(obj) {
            foreach(obj, ::(k, v){
                ops[k] = v;            
            });
        };


        define(out, args, classinst);
        removeKey(funcs, 'interface');
        removeKey(funcs, 'attributes');

        if (funcs['onRevive']) ::<={
            @or = funcs['onRevive'];
            arraypush(onRevive, or);
            funcs['onRevive'] = onRevive;
        };


        return out;
    };
    */
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

