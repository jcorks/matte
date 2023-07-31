// Modules are just independent running contexts.
// Their return value becomes the value for that module.
@:output = {
    value : 'Hello, ',
    myFunction ::{
        return output.value;
    }
}


return output;
