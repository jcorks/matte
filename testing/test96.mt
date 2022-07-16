//// Test96
//
// loop and error handling 


@output = '';

[::]{
    forever();
} : {
    onError:::(message) {
        output = output + 'noarg';
    }
};


[::]{
    forever(do:'hi');
} : {
    onError:::(message){
        output = output + 'nofn';
    }
};

[::]{
    forever(do:::{
        [0, 10]->for(do:::{

        });
        send();
    });
} : { 
    onError:::(message){
        output = output + 'norun';
    }
};




return output;
