/*
    Implementation for class.mt
    
    This is an example implementation. Any real implementation 
    just needs to model this one's behavior. This can be used as 
    the implementation.
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
