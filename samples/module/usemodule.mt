<@>TestModule = import(module:'module.mt', parameters:{offset:1000});


@a = TestModule.new(initial:300);
a.value0 = 100;
a.value1 = 200;
print(message:a.add());
