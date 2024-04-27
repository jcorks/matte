// Create a new Object.
@:a = {};

a.something = "number";
a.ten = 10;
a.test = "Object";

// Should both print 10
print(:a.ten);
print(:a["ten"]);
