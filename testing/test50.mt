////Test 50
//
// Errors


@errMessage;
@n100 = ::? {
    error(detail:'Testing');    
    return 50;
} : {
    onError::: (message){
        errMessage = 
            'callstack:' + 
            message.callstack.length +':' + 
            message.callstack.frames[0].file + ':' + 
            message.callstack.frames[0].lineNumber;
        return 100;
    }
}
return ''+n100 + errMessage;


