//// Test102
//
// default import 

@val = import('mytestmodule.mt');

@out = '';
out = out + val.mydata;


out = out + Boolean(val == import('mytestmodule.mt'));

listen(::{
    out = out + import();   
}, ::{
    out = out + 'noim';
});


listen(::{
    out = out + import('not a file.mt');   
}, ::{
    out = out + 'noen';
});


listen(::{
    out = out + import('badfile.mt');   
}, ::{
    out = out + 'nocmp';
});

return out;
