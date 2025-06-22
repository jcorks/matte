//// Test 117
//
// Find index 

@:list = [
    {id:0},
    {id:5},
    {id:21},
    {id:33},
    {id:10}
];

@out = '';
out = out + list->findIndex(:list[2]);
out = out + list->findIndex(:{});

out = out + list->findIndexCondition(::(value) <- value.id == 0);
out = out + list->findIndexCondition(::(value) <- value.id == 10);
out = out + list->findIndexCondition(::(value) <- value.id > 20);
out = out + list->findIndexCondition(::(value) <- false);


::? {
    list->findIndex(:21);
} => {
    onError::(message) {
        out = out + 'error';
    }
}




return out;
