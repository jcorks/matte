////Test 49
//
// Errors

@errMessage;
@emptyOnError = listen(to:::{    
    listen(to: 
        ::{
            error(detail:'Testing');
        },

        onMessage: ::(msg) {
            errMessage = 'InnerMessage';
            error(detail:'whoops');
        }
    );
    
    return 100;
}, onMessage:::(message){
    errMessage = 'outerMessage';
});
return errMessage + emptyOnError;

