@Time      = import('Matte.System.Time');
@ConsoleIO = import('Matte.System.ConsoleIO');
@Utility   = import('Matte.System.Utility');

@c = Time.Clock.new();

c.autoRestart = true;
c.start(1000);

c.installHook('onTimeout', ::{
    ConsoleIO.clear();
    ConsoleIO.println('I have finished thinking.');
    ConsoleIO.println('The number is: ' + introspect.floor(Utility.random*100));
    c.start(4000 + Utility.random*4000);
    ConsoleIO.println('Thinking....');
});
    ConsoleIO.println('Thinking....');
loop(::{
    c.update();
    Time.sleep(15);
    return true;
});




