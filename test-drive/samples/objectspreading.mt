// This demonstrates how to use the spread operator
//
//

@:namesA = [
	"Kyle",
	"Adrian",
	"Santeri"
];

@:namesB = [
	"Thomas",
	"Philip",
	"Dakota"
];

// Using the spread operator, we can create new sets that are 
// a combination of already-defined sets.
// For arrays, their order is preserved.
@:allNames = [
	...namesA,
	"Terra",
	...namesB,
	"Kristin"
];

foreach(
	in:allNames, 
	do:::(index, name) <- print(message:name)
);

// Using the spread operator is also a convenient way to make
// a shallow copy of a set for other operations, like ->map.
return [...allNames]->map(to:::(value) <- value->length);
