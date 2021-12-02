//// Test80
//
// 
@Matte = import(module:'Matte.Core');

@a = Matte.String.new(from:'-=-');
a.append(with:'[][][]');
a.append(with:Matte.String.new('100'));
a.append(with:69);
a.append(with:'');

return String(from:a);
