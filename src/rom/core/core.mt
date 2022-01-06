@class = import(module:"Matte.Core.Class");
@array = import(module:"Matte.Core.Array");
@string = import(module:"Matte.Core.String");
@json        = import(module:"Matte.Core.JSON");
@eventSystem = import(module:'Matte.Core.EventSystem');

return class(
    define:::(this) {
        this.interface = {
            Class       :{get::{return class;}},
            Array       :{get::{return array;}},
           'String'     :{get::{return string;}},
            JSON        :{get::{return json;}},
            EventSystem :{get::{return eventSystem;}}
        };
    }
).new();
