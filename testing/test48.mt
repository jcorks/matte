////Test 48
//
// Errors

@errMessage;
::<={
    context.catch = :: (t){
        errMessage = t.data;
    };

    error('Testing');
};

return errMessage;


