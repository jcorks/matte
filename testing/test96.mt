//// Test96
//
// loop and error handling 


@output = '';



::?{
    forever 'hi';
} => {
    onError:::(message){
        output = output + 'nofn';
    }
}

::?{
    forever ::{
        for(0, 10) ::{

        }
        send();
    }
} => { 
    onError:::(message){
        output = output + 'norun';
    }
}




return output;
