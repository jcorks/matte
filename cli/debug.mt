@Debug = {
    printObject ::(o) {
        @already = {};
        @pspace ::(level) {
            @str = '';
            for([0, level], ::{
                str = str + '  ';
            });
            return str;
        };
        @helper ::(obj, level) {
            @poself = context;

            return match(introspect.type(obj)) {
                (String) : '(type => String): \'' + obj + '\'',
                (Number) : '(type => Number): '+obj,
                (Boolean): '(type => Boolean): '+obj,
                (Empty)  : '<empty>',
                (Object) : ::{
                    when(already[obj] == true) '(type => Object): [already printed]';
                    already[obj] = true;

                    @output = if (introspect.isCallable(obj)) 
                                '(type => Function): {'
                            else 
                                '(type => Object): {';

                    @multi = false;
                    foreach(obj, ::(key, val) {                        
                        output = output + (if (multi) ',\n' else '\n'); 
                        output = output + pspace(level+1)+(String(key))+' : '+poself(val, level+1);
                        multi = true;
                    });
                    output = output + pspace(level) + (if (multi) '\n' + pspace(level)+'}' else '}');
                    return output;                
                }(),
                (Type): '(type => Type): ' + obj,
                default: '(type => ' + introspect.type(obj) + '): {...}'

            };
        };
        return pspace(1) + helper(o, 1);
    }
};


return Debug;
