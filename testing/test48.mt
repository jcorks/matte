////Test 48
//
// Errors

@errMessage;
::{
    context.onError = :: {
        errMessage = true;
    };


    error('Testing');
}();

return errMessage;


