//// Test80
//
// 
@Matte = import(module:'Matte.Core');

@a = Matte.String.new(from:'-=-');
a.append(other:'[][][]');
a.append(other:Matte.String.new(from:'100'));
a.append(other:69);
a.append(other:'');

return String(from:a);
