//// Test82
//
// 

@a = 'abcdefgh';


@strout = '';
@c = [];
for(in:[0, String.length(of:a)], do:::(i) {
    c[i] = String.charAt(string:a, index:i);
});

strout = String.combine(strings:[
    c[0],     c[1],    c[2],    c[3],
    c[4],     c[5],    c[6],    c[7]

]);


return strout;
