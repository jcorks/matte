@class = import('Matte.Core.Class');
return class({
    name : 'Matte.System',
    define::(this) {
        this.interface({
            ConsoleIO    : import('Matte.System.ConsoleIO'),
            Filesystem   : import('Matte.System.Filesystem'),
            MemoryBuffer : import('Matte.System.MemoryBuffer'),
            SocketIO     : import('Matte.System.SocketIO'),
            Time         : import('Matte.System.Time'),
            Utility      : import('Matte.System.Utility')    
        });
    }
}).new();
