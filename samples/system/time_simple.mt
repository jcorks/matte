// Time gives the ability to be time-aware within the program. It only 
// contains a few utilities.
@:Time = import(module:'Matte.System.Time');

// GetTicks() reports how long the program has been alive in milliseconds.
print(message: 'Current alive time: ' + Time.getTicks());

// Sleep pauses the running thread for the given number of milliseconds.
// In most cases, this can help reduce CPU usage in more complex 
// programs that use Asynchronous workers.
Time.sleep(milliseconds:1000);

print(message: 'Current alive time: ' + Time.getTicks());



// Time() returns a number containing the current UNIX Epoch time.
print(message: 'Epoch time: ' + Time.time());
