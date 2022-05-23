

@class ::(define, name, inherits, statics) {


    @classinst = {define : define};
    when(classinst.define->type != Function) error(detail:'class must include a "define" function within its "info" specification');
    @classInherits = inherits;
    @pool = [];
    @poolCount = 0;
    @selftype = if (classInherits != empty) ::<={
        @inheritset = [];
        @inheritCount = 0;
        foreach(in:classInherits, do:::(k, v) {
            inheritset[inheritCount] = v.type;
            inheritCount+=1;
        });
        return Object.newType(name : name, inherits : inheritset);
    } else ::<= {
        return Object.newType(name : name);
    };


    // all recycled members currently waiting
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
                pooled : false,
                newinst : Object.instantiate(type:selftype),
                interface : {},
                bases : [],
                constructors : {}, // keyed with object type
                onRecycles : {}
            };
            
            newinst = instSet.newinst;
            interface = instSet.interface;
            
            @attribs = {};
            
            attribs['.'] = {
                get:: (key) {
                    @result = interface[key];
                    when(result == empty) 
                        error(detail:'' +key+ " does not exist within this instance's class.");
                                                    
                    when(result.isFunction) result.fn;
                    @get = result.get;
                    when(get == empty) error(detail:'The attribute ' + key + ' is not readable.');
                    return get();
                },


                set:: (key, value) {
                    @result = interface[key];
                    when(result == empty) 
                        error(detail:'' +key+ " does not exist within this instance's class.");
                    
                    when(result.isFunction) error(detail:'Interface functions cannot be overwritten.');
                    @set = result.set;
                    when(set == empty) error(detail:'The attribute ' + key + ' is not writable.');
                    return set(value:value);
                }
            };
               

            
            interface.interface = {
                isFunction : false,
                set ::(value) {
                    foreach(in:value, do:::(k => String, v) {
                        if (v->type == Function) ::<= {
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
                }
            };

            newinst->setAttributes(attributes:attribs);
            // default / building interface
            newinst.interface = {
                class : {
                    get ::{
                        return classinst;
                    }
                },
                
                'type' : {
                    get ::{
                        return selftype;
                    }
                },  
                
                constructor : {
                    set ::(value) {
                        instSet.constructor = value;
                    },
                    
                    get ::{
                        return instSet.constructors;                    
                    }
                },
                
                recyclable : {
                    set ::(value => Boolean) {
                        instSet.recycle = value;
                    }
                },

                
                onRecycle : {
                    set ::(value) {
                        instSet.onRecycle = value;
                    },

                    get ::{
                        return instSet.onRecycles;
                    }

                },
                
                attributes : {
                    set ::(value) {
                        foreach(in:value, do:::(k, v) {
                            when(k->type == String && k == '.') empty; // skip 
                            attribs[k] = v;
                        }); 
                        newinst->setAttributes(attributes:attribs);
                    }  
                }
                
            };

        } else ::<= {
            instSet = instSetIn;
            newinst = instSetIn.newinst;
            interface = instSetIn.interface;
            instSetIn.bases->push(value:instSet);            
        };
        
        @runSelf = context;
        if (inherited != empty) ::<={
            foreach(in:inherited, do:::(k, v) {
                runSelf(instSetIn:instSet, classObj:v);            
            });
        };

        instSet.recycle = false;
        classObj['define'](this:newinst);
        
        // constructor is what is returned from the new() function the 
        // user calls.
        // This means that whatever it returns is what the user code works with 
        // It /should/ be the 
        instSet.constructors[classObj] = instSet.constructor;
        instSet.onRecycles[classObj] = instSet.onRecycle;

        
        return instSet;
    };
    

    
    classinst->setAttributes(
        attributes : {
            '.' : {
                get ::(key => String)  {
                    when(key == 'new')::<={
                        when(poolCount > 0) ::<={
                            @out = pool[poolCount-1];
                            pool->remove(key:poolCount-1);
                            poolCount -= 1;
                            
                            @constructor = out.constructor;
                            when(constructor != empty) constructor;
                            return ::{return newinst;};
                        };
                    

                        
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



 
                        @constructor = instSet.constructor;

                        if (recycle == true) ::<= {
                            newinst.interface = {
                                recycle::{
                                    if (instSet.pooled) error(detail:'Object double-recycled.');
                                    instSet.pooled = true;
                                    if (instSet.onRecycle != empty) instSet.onRecycle();
                                    pool[poolCount] = instSet;
                                }
                            };
                        };

                        newinst->remove(keys:['constructor', 'onRecycle', 'interface']);


                        when(constructor != empty) constructor;
                        return ::{return newinst;};
                    };
                    when(key == 'inherits') classInherits;
                    when(key == 'type') selftype;

                    @:r = statics[key];
                    when(r != empty) r;
                    error(detail:'No such member of the class object.');
                }
            }
        }
    );

    return classinst;
};
return class;
