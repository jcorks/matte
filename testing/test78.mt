//// Test78
//
// 
@Matte = import(module:'Matte.Core');

@a = Matte.String.new(from:'abcdef!@#$ \n');


return '' +
    a.charCodeAt(index:0) + 
    a.charCodeAt(index:1) + 
    a.charCodeAt(index:2) + 
    a.charCodeAt(index:3) + 
    a.charCodeAt(index:4) + 
    a.charCodeAt(index:5) + 
    a.charCodeAt(index:6) + 
    a.charCodeAt(index:7) +
    a.charCodeAt(index:8) +
    a.charCodeAt(index:9) +
    a.charCodeAt(index:10) +
    a.charCodeAt(index:11);
    
