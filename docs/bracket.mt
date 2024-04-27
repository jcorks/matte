// Create a new Object.
@:a = {};

a[0] = "number";
a["ten"] = "string";
a[Number] = "type";

@:key = {};
a[key] = "object";

// Should print 4, as there are 
// 4 unique key-value pairs.
print(:a->keycount);
