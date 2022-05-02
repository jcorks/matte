////Test 48
//
// Errors

@errMessage;
listen(to:::{
        error(detail:'Testing');
    },
     
    onError:::(message) {
        errMessage = message.detail;
    }
);

listen(to:
    ::{
        error();
    },
     
    onError:::(message) {
        errMessage = errMessage + String(from:message.detail == empty);
    }
);


listen(to:
    ::{
        listen();
    }, 
    
    onError:::(message){
        errMessage = errMessage + 'failed!';
    }
);


return errMessage;


