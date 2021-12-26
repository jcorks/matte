

@class ::(info) {
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

    @classinst = {define : info.define};
    when(introspect.type(of:classinst.define) != Function) error(detail:'class must include a "define" function within its "info" specification');
    @classInherits = info.inherits;
    @type = if (classInherits != empty) ::<={
        @inheritset = [];
        @inheritCount = 0;
        foreach(in:classInherits, do:::(k, v) {
            inheritset[inheritCount] = v.type;
            inheritCount+=1;
        });
        return newtype(name : info.name, inherits : inheritset);
    } else ::<= {
        return newtype(name : info.name);
    };


    // all recycled members currently waiting
    @recycled = [];
    @recycledCount = 0;
    @test = 0;

    
    @makeInstance::(instSetIn, classObj) {
        @newinst;
        @interface;
        @instSet;
        @inherited = classObj.inherits;

        
        // only create a new object ONCE. This is called by 
        // the "new" function access, and will ALWAYS be of 
        // the child class type.
        if (instSetIn == empty) ::<= {
            instSet = {
                newinst : instantiate(type:type),
                interface : {},
                bases : []
            };
            
            newinst = instSet.newinst;
            interface = instSet.interface;
        } else ::<= {
            instSet = {
                newinst : instSetIn.newinst,
                class : classObj,
                bases : []
            };
            newinst = instSetIn.newinst;
            interface = instSetIn.interface;
            arraypush(a:instSetIn.bases, b:instSet);            
        };
        
        @runSelf = context;
        if (inherited != empty) ::<={
            foreach(in:inherited, do:::(k, v) {
                runSelf(instSetIn:instSet, classObj:v);            
            });
        };

        newinst.class = classinst;
        newinst.type = type;
        newinst.constructor = empty;
        newinst.destructor = empty;
        classObj['define'](this:newinst);
        
        // constructor is what is returned from the new() function the 
        // user calls.
        // This means that whatever it returns is what the user code works with 
        // It /should/ be the 
            instSet.constructor = newinst.constructor;
            instSet.destructor = newinst.destructor;

            newinst[classObj] = {
                constructor : newinst.constructor,
                destructor : newinst.destructor
            };        
        instSet.recycle = newinst.recycle;
        
        
        if (newinst.interface != empty) ::<={
            foreach(in:newinst.interface, do:::(k => String, v) {
                if (introspect.type(of:v) == Function) ::<= {
                    interface[k] = {
                        isFunction : true,
                        fn : v
                    };
                } else ::<={
                    interface[k] = {
                        isFunction : false,
                        get : v.get,
                        set : v.set
                    };                                    
                };
            });
        };
        return instSet;
    };
    

    
    setAttributes(
        of         : classinst,
        attributes : {
            '.' : {
                get ::(key => String)  {
                    when(key == 'new')::<={
                        when(recycledCount > 0) ::()=>Function {
                            @out = recycled[recycledCount-1];
                            recycledCount-=1;
                            removeKey(from:recycled, key:recycledCount);
                            print(message:'----Remaining: ' + introspect.keycount(of:recycled));
                            @constructor = getAttributes(of:out).constructor;
                            when(constructor != empty) constructor;
                            return ::{return newinst;};
                        }();
                    

                        
                        // populates interfaces and such with inheritance considerations if needed.
                        @instSet = makeInstance(classObj:classinst); 
                        @newinst = instSet.newinst;
                        @interface = instSet.interface;
                        @recycle = instSet.recycle;
                        
                        interface.class = {
                            isFunction : false,
                            get ::{
                                return classinst;
                            }
                        };


                        @attribs;
                        if (newinst.attributes == empty) ::<={
                            attribs = {};
                        } else ::<= {
                            attribs = newinst.attributes;
                        };

                        attribs['.'] = {
                            get:: (key) {
                                @result = interface[key];
                                when(result == empty) 
                                    error(detail:'' +key+ " does not exist within this instance's class.");
                                                                
                                when(result.isFunction) result.fn;
                                @get = result.get;
                                when(get == empty) error(detail:'The attribute is not readable.');
                                return get();
                            },


                            set:: (key, value) {
                                @result = interface[key];
                                when(result == empty) 
                                    error(detail:'' +key+ " does not exist within this instance's class.");
                                
                                when(result.isFunction) error(detail:'Interface functions cannot be overwritten.');
                                @set = result.set;
                                when(set == empty) error(detail:'The attribute is not writable.');
                                return set(value:value);
                            }
                        };
                            
                        removeKey(from:newinst, keys:['class', 'type', 'constructor', 'interfaces']);
                        @preserve;
                        @constructor = instSet.constructor;
                        @destructor = instSet.destructor;

                        if (recycle == true) ::<= {
                            if (destructor) ::<= {
                                attribs.preserver = ::{
                                    destructor();
                                    attribs.preserver = context;
                                    setAttributes(
                                        of: newinst,
                                        attributes : attribs
                                    );

                                    recycled[recycledCount] = newinst;
                                    recycledCount += 1;
                                    attribs.constructor = constructor; // needed for recycling return val.
                                    // TODO: need a better way to handle constructor data.
                                };  
                            } else ::<= {
                                attribs.preserver = ::{
                                    attribs.preserver = context;
                                    setAttributes(
                                        of: newinst,
                                        attributes : attribs
                                    );

                                    recycled[recycledCount] = newinst;
                                    recycledCount += 1;
                                    attribs.constructor = constructor; // needed for recycling return val.
                                    // TODO: need a better way to handle constructor data.
                                };
                            };
                        } else ::<= {
                            if (destructor) ::<= {
                                attribs.preserver = ::{
                                    destructor();
                                };                            
                            };
                        };


                        setAttributes(
                            of: newinst,
                            attributes : attribs
                        );


                        when(constructor != empty) constructor;
                        return ::{return newinst;};
                    };
                    
                    when(key == 'inherits') classInherits;
                    when(key == 'type') type;
                    
                    error(detail:'No such member of the class object.');
                }
            }
        }
    );

    return classinst;
};
return class;
