//// Test 103
//
// Erroneous introspection 

@out = '';

listen(to:::{
    out = out + 2423->keys;
}, onError:::(message){
    out = out + 1;
});


listen(to:::{
    out = out + String->values;
}, onError:::(message){
    out = out + 2;
});

listen(to:::{
    out = out + String->keycount;
}, onError:::(message){
    out = out + 3;
});


listen(to:::{
    out = out + '[0, 1, 2]'->keycount;
}, onError:::(message){
    out = out + 4 + 5;
});




listen(to:::{
    out = out + '123456'->charAt(index:5);
    out = out + 1321->charAt(index:2);
}, onError:::(message){
    out = out + 7;
});


listen(to:::{
    out = out + '222222'->charAt(index:'2');
}, onError:::(message){
    out = out + 8;
});

listen(to:::{
    out = out + 1321->charCodeAt(index:1);
}, onError:::(message){
    out = out + 9;
});

listen(to:::{
    out = out + '222222'->charCodeAt(index:'2');
}, onError:::(message){
    out = out + 'A';
});




listen(to:::{
    out = out + 20->subset();
}, onError:::(message){
    out = out + 'B';
});

listen(to:::{
    out = out + [0, 1, 2]->subset(from:1);
}, onError:::(message){
    out = out + 'C';
});


listen(to:::{
    out = out + [0, 1, 2]->subset(from:'1', to:2);
}, onError:::(message){
    out = out + 'C';
});

return out;
