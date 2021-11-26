@class = import("Matte.Core.Class");
@array = import("Matte.Core.Array");
@string = import("Matte.Core.String");
@json        = import("Matte.Core.JSON");
@eventSystem = import('Matte.Core.EventSystem');

return class({
    define::(this) {
        this.interface({
            Class       :{get::{return class;}},
            Array       :{get::{return array;}},
           'String'     :{get::{return string;}},
            JSON        :{get::{return json;}},
            EventSystem :{get::{return eventSystem;}}
        });
    }
}).new();
