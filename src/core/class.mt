@pObject ::(o) {
    @already = {};
    @pspace ::(level) {
        @str = '';
        for([0, level], ::{
            str = str + ' ';
        });
        return str;
    };
    @helper ::(obj, level) {
        @poself = context;

        return match(introspect(obj).type()) {
            ('string') :    obj,
            ('number') : ''+obj,
            ('boolean'): ''+obj,
            ('empty')  : 'empty',
            ('object') : ::{
                when(already[obj] == true) '[already printed]';
                 
                @output = '{\n';
                foreach(obj, ::(key, val) {
                    output = output + pspace(level)+(AsString(key))+' : '+poself(val, level+1) + ',\n';
                });
                output = output + pspace(level) + '}\n';
                already[obj] = true;
                return output;                
            }()
        };
    };
    print(helper(o, 0));
};
@class ::(d) {
    // unfortunately, have to stick with these fake array things since we
    // are bootstrapping classes, which will be used to implement real Arrays.
    <@> arraylen ::(a){
        return introspect(a).keycount();
    };

    <@> arraypush ::(a, b){
        a[introspect(a).keycount()] = b;
    };

    <@> arrayclone ::(a) {
        <@> out = {};
        for([0, arraylen(a)], ::(i){
            out[i] = a[i];
        });
        return out;
    };




    <@> classinst = {};
    if(d.declare) d.declare();
    <@> define = d.define;

    classinst.interfaces = [];
    <@> allclass = classinst.interfaces;
    <@> inherits = if(d.inherits)::{
        <@> addbase ::(other){
            for([0, arraylen(other)], ::(n){
                <@>g = other[n];
                @found = false;
                for([0, arraylen(allclass)], ::(i) {
                    when(allclass[i] == g) ::{
                        found = true;
                        return arraylen(allclass);
                    }();
                });
                when(!found) ::{
                    arraypush(allclass, g);
                }();
            });
        };

        when(arraylen(inherits)) ::{
            for([0, arraylen(inherits)], ::(i){
                addbase(inherits[i].interfaces);
            });
            return d.inherits;
        }();
        addbase(inherits.interfaces);
    }();
    arraypush(allclass, classinst);

    // returns whether the isntance inherits from the given class.
    <@> isa ::(d) {
        @found = false;
        for([0, arraylen(allclass)], ::(i){
            when(allclass[i] == d)::{
                found = true;
                return arraylen(allclass);
            }();
        });
        return found;
    };

    classinst.new = ::(args, noseal, outsrc) {
        args = if(args)args else {};
        @out = outsrc;
        if(inherits) ::{
            out = if(out)out else {};
            if(inherits[1] == empty)::{
                for([1, arraylen(inherits)], ::(i){
                    inherits[i].new(args, out);
                });
            }() else ::{
                inherits.new(args, out);
            }();
        }();
        out = if(out) out else {};

        @setters;
        @getters;
        @funcs;
        @varnames;
        @mthnames;
        if(out.publicVars)::{
            setters = out.setters;
            getters = out.getters;
            funcs = out.funcs;
            mthnames = {
                inherited : out.publicMethods
            };

            varnames = {
                inherited : out.publicVars
            };
            out.publicVars = varnames;
            out.publicMethods = mthnames;
        }() else ::{
            setters = {};
            getters = {};
            funcs = {};
            varnames = {};
            mthnames = {};
            out.publicVars = varnames;
            out.publicMethods = mthnames;
            out.setters = setters;
            out.getters = getters;
            out.funcs = funcs;
        }();


        out.interface = ::(obj){
            <@> keys = introspect(obj).keys();
            foreach(obj, ::(key, v) {
                when(introspect(v).type() != 'object')::{
                    error("Class interfaces can only have getters/setters and methods.");
                }();
                if(introspect(v).isCallable())::{
                    funcs[key] = v;
                    mthnames[key] = 'function';
                    //print('ADDING CLASS function: ' + key);
                }() else ::{
                    setters[key] = v.set;
                    getters[key] = v.get;
                    varnames[key] = match((if(v.set) 1 else 0)+
                                          (if(v.get) 2 else 0)) {
                        (1) : 'Write-only',
                        (2) : 'Read-only',
                        (3) : 'Read/Write'
                    };
                }();
            });
        };

        define(out, args, classinst);
        removeKey(out, 'interface');



        if(noseal == empty) ::{
            out.accessor = ::(key) {
                when(key == 'introspect') {
                    public : {
                        variables : varnames,
                        methods : mthnames
                    }
                };
                when(key == 'isa') isa;

                @out = getters[key];
                when(out) out();

                out = funcs[key];
                when(out) out;

                @str = '';
                for([0, introspect(funcs).keycount()], ::(i) {
                    str = str + funcs[i] + ', ';
                });
                error('' +key+ " does not exist within this instances class." + str);
            };

            out.assigner = ::(key, value){
                @out = setters[key];
                when(out) out(value);
                error('' +key+ " does not exist within this instances class.");
            };
        }();
        return out;
    };
    return classinst;
};







/*
@test = Array.new([4, 2, 6, 3, 10, 1, 0, -40, 349, 0, 10, 43]);
print(test);
test.sort();
print('' + test + '\n\n');


@t = String.new("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.");
print(t.split(' '));
print(t.count(' '));
*/

