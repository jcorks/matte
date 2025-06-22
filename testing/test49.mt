////Test 49
//
// Errors

@errMessage;
@emptyOnError = ::?{    
    ::?{
        error(detail:'Testing');
    } => {
        onMessage: ::(msg) {
            errMessage = 'InnerMessage';
            error(detail:'whoops');
        }
    }
    
    return 100;
} => {
    onError:::(message){
        errMessage = 'outerMessage';
    }
}
return errMessage + emptyOnError;


