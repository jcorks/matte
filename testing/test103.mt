//// Test 103
//
// Erroneous introspection 

@out = '';

listen(to:::{
    out = out + introspect.keys(of:2423);
}, onMessage:::(message){
    out = out + 1;
});


listen(to:::{
    out = out + introspect.values(of:String);
}, onMessage:::(message){
    out = out + 2;
});

listen(to:::{
    out = out + introspect.keycount(of:String);
}, onMessage:::(message){
    out = out + 3;
});


listen(to:::{
    out = out + introspect.length(of:[0, 1, 2]);
}, onMessage:::(message){
    out = out + 4;
});


listen(to:::{
    out = out + introspect.arrayToString(of:'aaaaaaa');
}, onMessage:::(message){
    out = out + 5;
});


listen(to:::{
    out = out + introspect.charAt(string:'123456', index:5);
    out = out + introspect.charAt(string:1321, index:2);
}, onMessage:::(message){
    out = out + 7;
});


listen(to:::{
    out = out + introspect.charAt(string:'222222', index:'2');
}, onMessage:::(message){
    out = out + 8;
});

listen(to:::{
    out = out + introspect.charCodeAt(string:1321, index:1);
}, onMessage:::(message){
    out = out + 9;
});

listen(to:::{
    out = out + introspect.charCodeAt(string:'222222', index:'2');
}, onMessage:::(message){
    out = out + 'A';
});




listen(to:::{
    out = out + introspect.subset(set:20);
}, onMessage:::(message){
    out = out + 'B';
});

listen(to:::{
    out = out + introspect.subset(set:[0, 1, 2], from:1);
}, onMessage:::(message){
    out = out + 'C';
});


listen(to:::{
    out = out + introspect.subset(set:[0, 1, 2], from:'1', to:2);
}, onMessage:::(message){
    out = out + 'C';
});

return out;
