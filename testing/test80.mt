//// Test80
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('-=-');
a.append('[][][]');
a.append(Matte.String.new('100'));
a.append(69);
a.append('');

return String(a);
