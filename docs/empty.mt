// This value is uninitialized and will be set to empty.
@e;

// "empty" is a special value recognized by the runtime 
// whose value is always empty.
@f = empty;


// both would print true.
print(e == empty);
print(f == e);
