/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the Matte project (https://github.com/jcorks/matte)
matte was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/
@class ::(define, name, inherits, statics) {

    @staticNames = {};
    if (statics != empty)
        statics->foreach(do:::(key, value) {
            staticNames[key] = true;
        })
    else 
        statics = {}
    ;
    
    @staticData = {...statics};
    @classinst = {define : define};
    when(classinst.define->type != Function) error(detail:'class must include a "define" function within its "info" specification');
    @classInherits = inherits;
    @pool = [];
    @poolCount = 0;
    @selftype = if (classInherits != empty) ::<={
        @inheritset = [];
        @inheritCount = 0;
        classInherits->foreach(do:::(k, v) {
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
                        error(detail:'' +key+ " does not exist within " + selftype);
                                                    
                    when(result.isFunction) result.fn;
                    @get = result.get;
                    when(get == empty) error(detail:'The attribute ' + key + ' is not readable within '  + selftype);
                    return get();
                },


                set:: (key, value) {
                    @result = interface[key];
                    when(result == empty) 
                        error(detail:'' +key+ " does not exist within " + selftype);
                    
                    when(result.isFunction) error(detail:'Interface functions cannot be overwritten.');
                    @set = result.set;
                    when(set == empty) error(detail:'The attribute ' + key + ' is not writable within' + selftype);
                    return set(value:value);
                }
            };
               

            
            interface.interface = {
                isFunction : false,
                set ::(value) {
                    value->foreach(do:::(k => String, v) {
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
                        value->foreach(do:::(k, v) {
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
            inherited->foreach(do:::(k, v) {
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
    
    staticNames.new = true;
    
    staticNames.type = true;
    staticData.type = selftype;

    staticNames.inherits = true;
    staticData.inherits = classInherits;
    
    classinst->setAttributes(
        attributes : {
            '.' : {
                get ::(key => String)  {
                    when (key == 'new') ::<={
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

                    when(staticNames[key] != empty) staticData[key];
                    error(detail:'No such member of the class ' + selftype);
                },
                
                set ::(key, value) {
                    when(staticNames[key] != empty) staticData[key] = value;
                    error(detail:'No such member of the class ' +  selftype);
                }
            },
            '[]' : {
            },
            
            
            foreach ::<- staticData,
            keys ::<-staticData->keys,
            values ::<- staticData->values
        }
    );

    return classinst;
};
return class;
