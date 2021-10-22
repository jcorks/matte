<@>TestModule = import('module.mt');


@a = TestModule.new({value2:300});
a.value0 = 100;
a.value1 = 200;
Debug.print(a.add());
