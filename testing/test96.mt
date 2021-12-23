//// Test96
//
// loop and error handling 


@output = '';

listen(to:::{
    loop();
}, onMessage:::(message) {
    output = output + 'noarg';
});


listen(to:::{
    loop(func:'hi');
}, onMessage:::(message){
    output = output + 'nofn';
});

listen(to:::{
    loop(func:::{
        for(in:[0, 10], do:::{

        });
        return false;
    });
}, onMessage:::(message){
    output = output + 'norun';
});




return output;
