//// Test96
//
// loop and error handling 


@output = '';

listen(to:::{
    loop();
}, onMessage:::{
    output = output + 'noarg';
});


listen(to:::{
    loop('hi');
}, onMessage:::{
    output = output + 'nofn';
});

listen(to:::{
    loop(do:::{
        for(in:[0, 10], do:::{

        });
        return false;
    });
}, onMessage:::{
    output = output + 'norun';
});




return output;
