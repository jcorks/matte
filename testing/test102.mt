//// Test102
//
// default import 

@val = import(module:'mytestmodule.mt');

@out = '';
out = out + val.mydata;


out = out + Boolean(from:val == import(module:'mytestmodule.mt'));

listen(to:::{
    out = out + import();   
}, onMessage:::(message){
    out = out + 'noim';
});


listen(to:::{
    out = out + import(module:'not a file.mt');   
}, onMessage:::(message){
    out = out + 'noen';
});


listen(to:::{
    out = out + import(module:'badfile.mt');   
}, onMessage:::(message){
    out = out + 'nocmp';
});

return out;
