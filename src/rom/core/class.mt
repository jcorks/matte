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


@:class ::(define => Function, new, name, inherits, statics) {
    @classInstance = Object.instantiate(
        type: classType
    );
    @type = if (inherits == empty) 
        Object.newType(name)
    else 
        Object.newType(name, inherits: [...inherits]->map(to:::(value) <- value.type));
    
    // merge statics into class instance
    if (statics != empty) ::<= {
        foreach(statics) ::(key, val) {
            classInstance[key] = val;
        }
    }
    classInstance.name = {get::<- name};
    classInstance.define = define;
    classInstance.inherits = {get::<- inherits};
    classInstance.type = {get::<- type};
    classInstance.construct = ::(this) {
        if (inherits != empty) ::<= {
            foreach(inherits) ::(key, inherit) {
                inherit.construct(this);
            };
        }
        @constructInterface;
        @constructInit;
        this.interface = {
            set ::(value) {
                constructInterface = value;
            },
            
            get ::<- constructInterface
        };

        this.constructor = {
            set ::(value) {
                constructInit = value;
            }
        };

        this->setIsInterface(enabled:true);
        define(this);        
        this->setIsInterface(enabled:false);

        if (constructInterface) ::<= {
            foreach(constructInterface)::(k, v) {
                when(k->type != String) 
                    error(detail: 'Class interfaces can only have String-keyed members.');
                this[k] = v;
            }
        }
        this->remove(key:'constructor');
        this->remove(key:'interface');
        if (constructInit) ::<= {        
            this->setIsInterface(enabled:true);
            constructInit();
            this->setIsInterface(enabled:false);
        }
    };
    classInstance.defaultNew = :: {
        @this = Object.instantiate(type);
        classInstance.construct(this);
        this->setIsInterface(enabled:true);
        return this;
    };
    
    classInstance.new = if (new) new else classInstance.defaultNew;
   


    
    classInstance->setIsInterface(enabled:true);
    return classInstance;
}
return class;
