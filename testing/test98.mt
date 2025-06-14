//// Test 98
//
// foreach in depth 

@out = '';

::?{
    'aaa'->foreach(::{
    
    });
}: { onError:::(message){
    out = out + 'noobj';
}}



::?{
    {}->foreach(:'aaa');
}: { onError:::(message){
    out = out + 'nofn';
}}




::?{
    {}->foreach(::{
        out = 'c';
    });
}: { onError:::(message){
    out = out + 'norun';
}}





::?{
    ['a', 'b', 'c']->foreach(::(key => Number, val => String) {
        out = out + val;
    });
}: { onError:::(message){
    out = out + 'norun';
}}


::?{
    foreach('aaa')::{
    
    }
}: { onError:::(message){
    out = out + 'noobj';
}}



::?{
    foreach({})'aaa';
}: { onError:::(message){
    out = out + 'nofn';
}}


::?{
    foreach(::{})::{};
}: { onError:::(message){
    out = out + 'noobj2';
}}





::?{
    foreach({})::{
        out = 'c';
    }
}: { onError:::(message){
    out = out + 'norun';
}}





::?{
    foreach(['a', 'b', 'c'])::(key => Number, val => String) {
        out = out + val;
    }
}: { onError:::(message){
    out = out + 'norun';
}}


return out;
