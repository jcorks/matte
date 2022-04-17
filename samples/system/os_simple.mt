// Contains general utilities delivered by the OS.
@:OS = import(module:'Matte.System.OS');

// OS.name can be used to differentiate system-based features.
print(message:'Currently running on: ' + OS.name);

// OS.run() can be used to run shell utilities and programs.
// Replace this with your favorite UI-based program.
if (OS.name == 'unix-like') 
    OS.run(command:'firefox &')
else 
    OS.run(command:'notepad.exe')


// OS.exit() will terminate the running program and deliver the 
// exit code to the OS.
OS.exit(code:0);
