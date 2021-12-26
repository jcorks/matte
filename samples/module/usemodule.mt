<@>TestModule = import(module:'module.mt');


@a = TestModule.new(initial:300);
a.value0 = 100;
a.value1 = 200;
print(message:a.add());
