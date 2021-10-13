@MatteString = import('Matte.String');

@a = MatteString.new('This is my value: 54. Nothin else!');
@b = a.scan('my value: [%]. ');

return b[0];
