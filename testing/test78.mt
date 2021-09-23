//// Test78
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('abcdef!@#$ \n');


return '' +
    a.charCodeAt(0) + 
    a.charCodeAt(1) + 
    a.charCodeAt(2) + 
    a.charCodeAt(3) + 
    a.charCodeAt(4) + 
    a.charCodeAt(5) + 
    a.charCodeAt(6) + 
    a.charCodeAt(7) +
    a.charCodeAt(8) +
    a.charCodeAt(9) +
    a.charCodeAt(10) +
    a.charCodeAt(11);
    
