//// test61
//
// Shadowed vars 
@output;
@hello = 'aa';


::<={
    @hello = false;
    ::<={
        @hello = 100;
        output = ''+hello;
    }
    
    output = output + hello;
}

return hello + output;
