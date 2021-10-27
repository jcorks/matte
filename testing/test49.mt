////Test 49
//
// Errors

@errMessage;
@emptyOnError = listen(::{    
    listen(
        ::{
            error('Testing');
        },

        ::(msg) {
            errMessage = 'InnerMessage';
            error('whoops');
        }
    );
    
    return 100;
}, ::(t){
    errMessage = 'outerMessage';
});
return errMessage + emptyOnError;


