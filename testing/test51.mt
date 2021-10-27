////Test 51
//
// Errors


@errMessage = 0;
@iterCheck = 0;
@n100 = listen(::{    

    for([0, 100], ::(i) {
        errMessage = errMessage + i;        
        when(i == 50) ::{
            iterCheck = i;
            error(100*80);
        }();
    });

    return 100;
}, ::(i){
    errMessage = i.data;
});
return ''+n100 + errMessage + iterCheck;


