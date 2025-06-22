//// Test 97
//
// For in depth


@out = '';

::?{
    for(0, 1)1;
} => { onError:::(message){
    out = out + 'nofn';
}}


::?{
    for(10, 4)::{
        out = out + 'a';
    }
}=>{ onError:::(message){
    out = out + 'norun';
}}


::?{
    for(2, 0)0;
}=>{ onError:::(message){
    out = out + 'nofn';
}}



::?{
    for(5, 0)::{
        out = out + 'b';
    }
}=>{ onError:::(message){
    out = out + 'norun';
}}



::?{
    for(0, 10) ::(i){
        out = out + 'c';        
        when (i == 4) 10; 
    }
}=>{ onError:::(message){
    out = out + 'norun';
}}


for(0, 3)::(i){
    out = out + 'd';        
}


return out;
