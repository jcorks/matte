// Create a new Object.
@:a = {};

a.something = "number";
a.ten = 10;
a.test = "Object";

// Should both print 10
print(message:a.ten);
print(message:a["ten"]);
