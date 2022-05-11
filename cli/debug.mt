@Debug = {
    printObject ::(o) {
        @already = {};
        @pspace ::(level) {
            @str = '';
            for(in:[0, level], do:::{
                str = str + '  ';
            });
            return str;
        };
        @helper ::(obj, level) {
            @poself = context;

            return match(obj->type) {
                (String) : '(type => String): \'' + obj + '\'',
                (Number) : '(type => Number): '+obj,
                (Boolean): '(type => Boolean): '+obj,
                (Empty)  : '<empty>',
                (Object) : ::{
                    when(already[obj] == true) '(type => Object): [already printed]';
                    already[obj] = true;

                    @output = '(type => Object): {';

                    @multi = false;
                    foreach(in:obj, do:::(key, val) {                        
                        output = output + (if (multi) ',\n' else '\n'); 
                        output = output + pspace(level:level+1)+(String(from:key))+' : '+poself(obj:val, level:level+1);
                        multi = true;
                    });
                    output = output + pspace(level:level) + (if (multi) '\n' + pspace(level:level)+'}' else '}');
                    return output;                
                }(),
                (Type): '(type => Type): ' + obj,
                default: '(type => ' + (obj->type) + '): {...}'

            };
        };
        return pspace(level:1) + helper(obj:o, level:1);
    }
};


return Debug;
