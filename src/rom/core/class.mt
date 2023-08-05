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
@classType = Object.newType(name:'Matte.Class');
@constructType = Object.newType(name:'Matte.Class.Construct');


@:class ::(define => Function, name, inherits, statics) {

    @type = if (inherits == empty) 
        Object.newType(name)
    else 
        Object.newType(name, inherits: [...inherits]->map(to:::(value) <- value.type));
    
    @classInterface = {};
    // merge statics into class instance
    if (statics != empty) ::<= {
        foreach(statics) ::(key, val) {
            classInterface[key] = val;
        }
    }
    classInterface.name = {get::<- name};
    classInterface.define = define;
    classInterface.inherits = {get::<- inherits};
    classInterface.type = {get::<- type};
    classInterface.construct = ::(constructors, this) {
        if (inherits != empty) ::<= {
            foreach(inherits) ::(key, inherit) {
                inherit.construct(constructors, this);
            };
        }
        this.constructor = empty;
        define(this);        
        constructors[classInstance] = this.constructor;
    };
    classInterface.new = {
        get::{
        
            @thisInterface;
            @thisInstance;
            @thisBaseConstructors = [];
            @thisConstructor;
            @this = Object.instantiate(
                type: constructType,
                interface : {
                    interface : {
                        set ::(value) {
                            if (thisInterface == empty) 
                                thisInterface = value
                            else ::<= {
                                foreach(value) ::(k, v) {
                                    thisInterface[k] = v;
                                }
                            }
                        }
                    },
                    
                    constructor : {
                        set ::(value) {
                            thisConstructor = value;
                        },
                        
                        get :: {
                            return thisConstructor;
                        }
                    },
                    
                    
                    instance : {
                        get :: {
                            return thisInstance;
                        }             
                    },
                    
                    baseConstructor : {
                        get :: {
                            return thisBaseConstructors;
                        }
                    }
                }
            );
            classInstance.construct(constructors:thisBaseConstructors, this);
            thisInstance = {:::} {
                return Object.instantiate(
                    type,
                    interface : thisInterface
                );
            } : {
                onError::(message) {
                    error(detail:'Failed to instantiate instance of type ' + classType->type + '. Make sure all your interface members are either Functions or Objects (setter/getters).\nInterface: \n' + (import(module:'Matte.Core.Introspect')(value:thisInterface)));                
                }
            }
            
            
            
            if (thisConstructor == empty)
                thisConstructor = ::{return this.instance;}
            return thisConstructor;
        }
    };
   
    @classInstance = {:::} {
        return Object.instantiate(
            type: classType,
            interface : classInterface
        );
    } : {
        onError ::(message) {
            error(detail:'Failed to initialize the class ' + classType->type + ' itself. If you are defining statics, make sure all your static members are either Functions or Objects (setter/getters).\nStatics passed in are: \n' + (import(module:'Matte.Core.Introspect')(value:classInterface)));
        }
    }
    
    return classInstance;
}
return class;
