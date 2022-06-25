//// Test82
//
// 

@a = 'abcdefgh';


@strout = '';
@c = [];
[0, a->length]->for(do:::(i) {
    c[i] = a->charAt(index:i);
});

strout = String.combine(strings:[
    c[0],     c[1],    c[2],    c[3],
    c[4],     c[5],    c[6],    c[7]

]);


return strout;
