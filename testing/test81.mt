//// Test81
//
// 

@a = 'abcdefgh';


@strout = '';
for(in:[0, String.length(of:a)], do:::(i){
    strout = strout + String.charCodeAt(string:a, index:i);
});



return strout;
