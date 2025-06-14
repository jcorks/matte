////Test 48
//
// Errors

@errMessage;
::?{
        error(detail:'Testing');
} : {
    onError:::(message) {
        errMessage = message.detail;
    }
}

::?{
    error();
} : {
    onError:::(message) {
        errMessage = errMessage + String(from:message.detail == empty);
    }
}


::? {
        @:a = 2;
        a();
} : {  
    onError:::(message){
        errMessage = errMessage + 'failed!';
    }
}


return errMessage;


