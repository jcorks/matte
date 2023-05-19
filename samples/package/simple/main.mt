@:resource = import('TestPackage.example.mt');

return {
  test::{print(message:resource.value);}
};
