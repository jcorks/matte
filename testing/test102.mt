//// Test102
//
// default import 

@val = import(module:'mytestmodule.mt');

@out = '';
out = out + val.mydata;


out = out + Boolean(from:val == import(module:'mytestmodule.mt'));

listen(to:::{
    out = out + import();   
}, onMessage:::{
    out = out + 'noim';
});


listen(to:::{
    out = out + import(module:'not a file.mt');   
}, onMessage:::{
    out = out + 'noen';
});


listen(to:::{
    out = out + import(module:'badfile.mt');   
}, onMessage:::{
    out = out + 'nocmp';
});

return out;
