//// Test 98
//
// foreach in depth 

@out = '';

{:::}{
    'aaa'->foreach(do:::{
    
    });
}: { onError:::(message){
    out = out + 'noobj';
}}



{:::}{
    {}->foreach(do:'aaa');
}: { onError:::(message){
    out = out + 'nofn';
}}




{:::}{
    {}->foreach(do:::{
        out = 'c';
    });
}: { onError:::(message){
    out = out + 'norun';
}}





{:::}{
    ['a', 'b', 'c']->foreach(do:::(key => Number, val => String) {
        out = out + val;
    });
}: { onError:::(message){
    out = out + 'norun';
}}


{:::}{
    foreach('aaa')::{
    
    }
}: { onError:::(message){
    out = out + 'noobj';
}}



{:::}{
    foreach({})'aaa';
}: { onError:::(message){
    out = out + 'nofn';
}}


{:::}{
    foreach(::{})::{};
}: { onError:::(message){
    out = out + 'noobj2';
}}





{:::}{
    foreach({})::{
        out = 'c';
    }
}: { onError:::(message){
    out = out + 'norun';
}}





{:::}{
    foreach(['a', 'b', 'c'])::(key => Number, val => String) {
        out = out + val;
    }
}: { onError:::(message){
    out = out + 'norun';
}}


return out;
