//// Test 103
//
// Erroneous introspection 

@out = '';

{:::}{
    out = out + 2423->keys;
}: { onError:::(message){
    out = out + 1;
}}


{:::}{
    out = out + String->values;
}: { onError:::(message){
    out = out + 2;
}}

{:::}{
    out = out + String->keycount;
}: { onError:::(message){
    out = out + 3;
}}


{:::}{
    out = out + '[0, 1, 2]'->keycount;
}: { onError:::(message){
    out = out + 4 + 5;
}}




{:::}{
    out = out + '123456'->charAt(index:5);
    out = out + 1321->charAt(index:2);
}: { onError:::(message){
    out = out + 7;
}}


{:::}{
    out = out + '222222'->charAt(index:'2');
}: { onError:::(message){
    out = out + 8;
}}

{:::}{
    out = out + 1321->charCodeAt(index:1);
}: { onError:::(message){
    out = out + 9;
}}

{:::}{
    out = out + '222222'->charCodeAt(index:'2');
}: { onError:::(message){
    out = out + 'A';
}}




{:::}{
    out = out + 20->subset();
}: { onError:::(message){
    out = out + 'B';
}}

{:::}{
    out = out + [0, 1, 2]->subset(from:1);
}: { onError:::(message){
    out = out + 'C';
}}


{:::}{
    out = out + [0, 1, 2]->subset(from:'1', to:2);
}: { onError:::(message){
    out = out + 'C';
}}

return out;
