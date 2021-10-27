//// Test96
//
// loop and error handling 


@output = '';

listen(::{
    loop();
}, ::{
    output = output + 'noarg';
});


listen(::{
    loop('hi');
}, ::{
    output = output + 'nofn';
});

listen(::{
    loop(::{
        for([0, 10], ::{

        });
        return false;
    });
}, ::{
    output = output + 'norun';
});




return output;
