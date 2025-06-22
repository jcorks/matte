////Test 48
//
// Errors

@errMessage;
::?{
        error(detail:'Testing');
} => {
    onError:::(message) {
        errMessage = message.detail;
    }
}

::?{
    error();
} => {
    onError:::(message) {
        errMessage = errMessage + String(from:message.detail == empty);
    }
}


@:handler = {  
    onError:::(message){
        errMessage = errMessage + 'failed!';
    }
}

::? {
        @:a = 2;
        a();
} => handler;


return errMessage;


