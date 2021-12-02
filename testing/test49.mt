////Test 49
//
// Errors

@errMessage;
@emptyOnError = listen(to:::{    
    listen(to: 
        ::{
            error(data:'Testing');
        },

        onMessage: ::(msg) {
            errMessage = 'InnerMessage';
            error(data:'whoops');
        }
    );
    
    return 100;
}, ::(t){
    errMessage = 'outerMessage';
});
return errMessage + emptyOnError;


