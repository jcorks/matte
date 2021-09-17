////Test 49
//
// Errors

@errMessage;
@emptyOnError = ::{
    context.catch = :: (t) {
        errMessage = 'outerMessage';
    };
    
    ::{
        context.catch = :: (t){
            errMessage = 'InnerMessage';
            error('whoops');
        };


        error('Testing');
    }();
    
    return 100;
}();
return errMessage + emptyOnError;


