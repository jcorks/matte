//// Test102
//
// default import 

@val = import(module:'mytestmodule.mt');

@out = '';
out = out + val.mydata;


out = out + Boolean(from:val == import(module:'mytestmodule.mt'));

{:::}{
    out = out + import();   
}: { onError:::(message){
    out = out + 'noim';
}}


{:::}{
    out = out + import(module:'not a file.mt');   
}: { onError:::(message){
    out = out + 'noen';
}}


{:::}{
    out = out + import(module:'badfile.mt');   
}: { onError:::(message){
    out = out + 'nocmp';
}}

return out;
