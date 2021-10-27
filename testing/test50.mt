////Test 50
//
// Errors


@errMessage;
@n100 = listen(::{
    error('Testing');    
    return 50;
}, :: (info){
    errMessage = 
        'callstack:' + 
        info.callstack.length +':' + 
        info.callstack.frames[0].file + ':' + 
        info.callstack.frames[0].lineNumber;
    return 100;
});
return ''+n100 + errMessage;


