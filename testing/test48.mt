////Test 48
//
// Errors

@errMessage;
listen(
    ::{
        error('Testing');
    },
     
    ::(t) {
        errMessage = t.data;
    }
);


return errMessage;


