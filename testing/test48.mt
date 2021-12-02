////Test 48
//
// Errors

@errMessage;
listen(to:::{
        error(data:'Testing');
    },
     
    onMessage:::(t) {
        errMessage = t.data;
    }
);

listen(to:
    ::{
        error();
    },
     
    onMessage:::(t) {
        errMessage = errMessage + String(t.data == empty);
    }
);


listen(to:
    ::{
        listen();
    }, 
    
    onMessage:::{
        errMessage = errMessage + 'failed!';
    }
);



return errMessage;


