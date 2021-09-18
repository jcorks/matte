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

            return match(introspect(obj).type()) {
                (String) : '(type => String): \'' + obj + '\'',
                (Number) : '(type => Number): '+obj,
                (Boolean): '(type => Boolean): '+obj,
                (Empty)  : '<empty>',
                (Object) : ::{
                    when(already[obj] == true) '(type => Object): [already printed]';

                    @output = if (introspect(obj).isCallable()) 
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
                    already[obj] = true;
                    return output;                
                }(),
                (Type): '(type => Type): ' + obj,
                default: '(type => ' + introspect(obj).type() + '): {...}'

            };
        };
        return pspace(1) + helper(o, 1);
    }
};


return Debug;
