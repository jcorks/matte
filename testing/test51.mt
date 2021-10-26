////Test 51
//
// Errors


@errMessage = 0;
@iterCheck = 0;
@n100 = ::{    
    context.catch = ::(i){
        errMessage = i.data;
    };



    for([0, 100], ::(i) {
        errMessage = errMessage + i;
        
        when(i == 50) ::{
            iterCheck = i;
            error(100*80);
        }();
    });

    return 100;
}();
return ''+n100 + errMessage + iterCheck;


