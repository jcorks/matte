////Test 48
//
// Errors

@errMessage;
::{
    context.catch = :: (t){
        errMessage = t;
    };

    error('Testing');
}();

return errMessage;


