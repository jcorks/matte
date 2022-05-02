//// Test 103
//
// Erroneous introspection 

@out = '';

listen(to:::{
    out = out + Object.keys(of:2423);
}, onError:::(message){
    out = out + 1;
});


listen(to:::{
    out = out + Object.values(of:String);
}, onError:::(message){
    out = out + 2;
});

listen(to:::{
    out = out + Object.keycount(of:String);
}, onError:::(message){
    out = out + 3;
});


listen(to:::{
    out = out + Object.length(of:'[0, 1, 2]');
}, onError:::(message){
    out = out + 4 + 5;
});




listen(to:::{
    out = out + String.charAt(string:'123456', index:5);
    out = out + String.charAt(string:1321, index:2);
}, onError:::(message){
    out = out + 7;
});


listen(to:::{
    out = out + String.charAt(string:'222222', index:'2');
}, onError:::(message){
    out = out + 8;
});

listen(to:::{
    out = out + String.charCodeAt(string:1321, index:1);
}, onError:::(message){
    out = out + 9;
});

listen(to:::{
    out = out + String.charCodeAt(string:'222222', index:'2');
}, onError:::(message){
    out = out + 'A';
});




listen(to:::{
    out = out + Object.subset(set:20);
}, onError:::(message){
    out = out + 'B';
});

listen(to:::{
    out = out + Object.subset(set:[0, 1, 2], from:1);
}, onError:::(message){
    out = out + 'C';
});


listen(to:::{
    out = out + Object.subset(set:[0, 1, 2], from:'1', to:2);
}, onError:::(message){
    out = out + 'C';
});

return out;
