@class = import(module:'Matte.Core.Class');
return class(
    name : 'Matte.System',
    define:::(this) {
        this.interface = {
            ConsoleIO    : import(module:'Matte.System.ConsoleIO'),
            Filesystem   : import(module:'Matte.System.Filesystem'),
            MemoryBuffer : import(module:'Matte.System.MemoryBuffer'),
            SocketIO     : import(module:'Matte.System.SocketIO'),
            Time         : import(module:'Matte.System.Time'),
            Utility      : import(module:'Matte.System.Utility')    ,
            Async        : import(module:'Matte.System.Async')
        };
    }
).new();
