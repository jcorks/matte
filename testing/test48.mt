////Test 48
//
// Errors

@errMessage;
listen(to:::{
        error(detail:'Testing');
    },
     
    onMessage:::(message) {
        errMessage = message.detail;
    }
);

listen(to:
    ::{
        error();
    },
     
    onMessage:::(message) {
        errMessage = errMessage + String(from:message.detail == empty);
    }
);


listen(to:
    ::{
        listen();
    }, 
    
    onMessage:::(message){
        errMessage = errMessage + 'failed!';
    }
);


return errMessage;


