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

listen(
    ::{
        error();
    },
     
    ::(t) {
        errMessage = errMessage + String(t.data == empty);
    }
);


listen(::{
    listen();
}, ::{
    errMessage = errMessage + 'failed!';
});



return errMessage;


