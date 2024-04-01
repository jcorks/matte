//// Test 126 
//
// Records

@:a = {
    hello : 'world!',
    eat : 'bread!',
    feel : 'good!',
    testing : empty,
    number : 1,
    (1) : 'one'
}

@out = a[1];

a->setIsRecord(enabled:true);

out = out + a.hello;
out = out + a.testing;
{:::} {
    a.other = 'dwa';
} : {
    onError::(message) {
        out = out + 'nokey';
    }
}


out = out + a['eat'];
{:::} {
    out = out + a[1]
} : {
    onError::(message) {
        out = out + 'nokey2';
    }
}

out = out + a->keys->size;


return out;

