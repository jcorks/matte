//// Test82
//
// 

@a = 'abcdefgh';


@strout = '';
@c = [];
for(0, a->length) ::(i) {
    c[i] = a->charAt(:i);
}

strout = String.combine(:[
    c[0],     c[1],    c[2],    c[3],
    c[4],     c[5],    c[6],    c[7]

]);


return strout;
