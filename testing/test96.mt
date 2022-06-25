//// Test96
//
// loop and error handling 


@output = '';

listen(to:::{
    forever();
}, onError:::(message) {
    output = output + 'noarg';
});


listen(to:::{
    forever(do:'hi');
}, onError:::(message){
    output = output + 'nofn';
});

listen(to:::{
    forever(do:::{
        [0, 10]->for(do:::{

        });
        send();
    });
}, onError:::(message){
    output = output + 'norun';
});




return output;
