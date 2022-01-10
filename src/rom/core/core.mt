@class = import(module:"Matte.Core.Class");
@json        = import(module:"Matte.Core.JSON");
@eventSystem = import(module:'Matte.Core.EventSystem');

return class(
    define:::(this) {
        this.interface = {
            Class       :{get::{return class;}},
            JSON        :{get::{return json;}},
            EventSystem :{get::{return eventSystem;}}
        };
    }
).new();
