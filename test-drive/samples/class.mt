// This demonstrates basic usage of a class.
// Classes are not an inherent language feature, but 
// a built-in module that can be imported.
//
// The class function can be retrieved by importing
// the Matte.Core.Class module.
@:class = import(module:'Matte.Core.Class');


// The class function generates classes by using 
// specific arguments.
@:MyClass = class(
	// The name argument makes it so that the name of 
	// the class is identifiable in the case of errors
	name: 'MyClass',
	
	// The define property is what defines the attributes of
	// any instances of the class. When constructing, "this"
	// refers to new members of the 
	define::(this) {
	
		// the define function makes it convenient to make private
		// variables, since the variables here are only available
		// lexically within this function definition.
		@privateValue;
		
		// The constructor function is essentially 
		// what is called when a user calls new() to 
		// instantiate a class instance. As such,
		// constructor arguments can be provided for new
		// instances. 
		//
		// The return value of the constructor is what 
		// will be returned by "new". Most often than not
		// the most useful thing is to return "this"
		this.constructor = ::(
			initialValue
		) {
			print(message:'Class constructed!');
			privateValue = initialValue;
			return this;
		}
	
	
		// The interface property defines the public 
		// members that are available in class 
		// instances. They can either be functions,
		// or public members.
		this.interface = {
		
			// when defining a setter/getter, a name 
			// is provided, set to an object with "set" and/or
			// "get" functions.
			a : {
				// get is called when accessing the member.
				// Its return value is what the user is given.
				get:: {
					print(message:"'a' was accessed!");
					return privateValue;
				}
			},
			
			b : {
				get ::<-privateValue**2
			},
			
			// when an interface is a function, it is simply
			// set to the function 
			
			c ::(argument) {
				print(message: "'c' was called with argument: " + argument);
				return this.b;
			}
		}
	}
)

// Once the class is created, instances may 
// be created by using new()
@:instance = MyClass.new(initialValue:200);

// The public members may be accessed as if 
// its a normal object.
print(message:instance.a);
print(message:instance.b);

return instance.c(argument:'Hi!');
