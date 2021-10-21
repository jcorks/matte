////Test 50
//
// Errors


@errMessage;
@n100 = ::<={    
    ::<={
        context.catch = :: (t, info){
            errMessage = 
                'callstack:' + 
                info.callstack.length +':' + 
                info.callstack.frames[0].file + ':' + 
                info.callstack.frames[0].lineNumber;
        };


        error('Testing');
    };
    
    return 100;
};
return ''+n100 + errMessage;


