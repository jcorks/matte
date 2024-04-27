@:class = import(:'Matte.Core.Class');

@test;

@:MyClass = class(
    define ::(this) {
        test = this;  
    }
);

@myInstance = MyClass.new();

// Should print true
print(:myInstance == test);




