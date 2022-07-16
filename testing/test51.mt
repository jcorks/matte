////Test 51
//
// Errors


@errMessage = 0;
@iterCheck = 0;
@n100 = [::]{    

    [0, 100]->for(do:::(i) {
        errMessage = errMessage + i;        
        when(i == 50) ::{
            iterCheck = i;
            error(detail:100*80);
        }();
    });

    return 100;
} : {
    onError:::(message){
        errMessage = message.detail;
    }
};
return ''+n100 + errMessage + iterCheck;


