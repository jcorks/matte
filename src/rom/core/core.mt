@class = import(module:"Matte.Core.Class");
@json        = import(module:"Matte.Core.JSON");
@eventSystem = import(module:'Matte.Core.EventSystem');
@introspect  = import(module:'Matte.Core.Introspect');

return class(
    define:::(this) {
        this.interface = {
            Class       :{get::<- class},   
            JSON        :{get::<- json},
            EventSystem :{get::<- eventSystem},
            Introspect  :{get::<- introspect}
        };
    }
).new();
