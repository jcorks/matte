@:filesystem   = import(module:'Matte.System.Filesystem');
@:MemoryBuffer = import(module:'Matte.System.MemoryBuffer');
@:class        = import(module:'Matte.Core.Class');
@:JSON         = import(module:'Matte.Core.JSON');

@:compile         = getExternalFunction(name:'package_matte_compile');
@:run_debug       = getExternalFunction(name:'package_matte_run_debug');
@:set_import      = getExternalFunction(name:'package_matte_set_import');
@:get_system_path = getExternalFunction(name:'package_matte_get_system_path'); 


// class that acts as a rolling data buffer,
// keeping track of the current location
@:DataStream = class(
    name: 'Matte.Package.DataStream',
    define:::(this) {
        @iter = 0;
        @buffer = MemoryBuffer.new();
        this.interface = {
            pushU8 ::(value => Number) {
                buffer.appendByte(value);
                iter += 1;
            },

            pushU32 ::(value => Number) {
                buffer.size += 4;
                buffer.writeU32(offset:iter, value);
                iter += 4;
            },
            
            
            pushString ::(value => String) {
                @:mem = MemoryBuffer.new();
                mem.appendUTF8(value);
                this.pushU32(value:mem.size);
                iter += mem.size;
                buffer.append(other:mem);
                mem.release();
            },
            
            pushBytes ::(value => MemoryBuffer.type()) {
                buffer.size += value.size;
                buffer.append(other:value);
                iter += value.size;
            },
            
            buffer : {
                get ::<- buffer
            }
        };
    }
);


// throws an error on a bad json file
@:checkPackageJSON ::(object) {
    // error checking for json.
    @:checkMissing = ::(base => Object, props => Object) <- 
        props->foreach(do:::(i, prop) {
            if (base[prop[0]] == empty)
                error(detail:'package.json missing ' + prop[0] + ' property');
            if (base[prop[0]]->type != prop[1]) 
                error(detail:'package.json property ' + prop[0] + ' must be of type ' + prop[1]);
            
        });

    // check missing attributes
    checkMissing(base: object, props:[
        ['name',         String],
        ['author',       String],
        ['maintainer',   String],
        ['description',  String],

        ['version', Object],
    
        ['depends',      Object], // name, [M, m]
        ['sources',      Object],
    ]);
    
    checkMissing(base: object.version, props:[
        [0, Number],
        [1, Number],
        [2, Number]
    ]);

    
    // the main.mt source is required.
    if (object.sources->findIndex(value:'main.mt') == -1)
        error(detail:'main.mt must be part of package.json.');
        
      
        
};


@:commands = {
    // tests a package by running it in debug mode.
    // by testing the package, the namespace is used
    // for import
    'test' ::() {
        @:json = filesystem.readString(path:'package.json');
        @object = JSON.decode(string:json);
        
        checkPackageJSON(object);

        // Sets the import namespace when importing files.
        // its always the name of the package.
        set_import(namespace:object.name + '.');

        // run the root in debug mode.
        run_debug(source:'main.mt');    
    },

    // makes a package based on a json
    'make' ::() {
        @:JSON = import(module:'Matte.Core.JSON');
        
        
        @:json = filesystem.readString(path:'package.json');
        @object = JSON.decode(string:json);
        
        checkPackageJSON(object);
            
        
        // create a buffer that represents the package
        @:output = DataStream.new();


        // header
        output.pushU8(value: 23);
        output.pushU8(value: 14);
        output.pushU8(value: 'M'->charCodeAt(index:0));
        output.pushU8(value: 'P'->charCodeAt(index:0));
        output.pushU8(value: 'K'->charCodeAt(index:0));
        output.pushU8(value: 'G'->charCodeAt(index:0));
        output.pushU8(value: 1); // package version
        output.pushString(value:json);

        // Do the sources first and compile them.        
        output.pushU32(value:object.sources->keycount);
        object.sources->foreach(do:::(i, source) {            
            compile(source:source => String, output:'.bytecode');
            
            @:bytes = filesystem.readBytes(path:'.bytes');

            output.pushString(value:source);
            output.pushU32(value:bytes.size);
            output.pushBytes(value:bytes);
            
        });

        output.pushU8(value: 0); // whether more packages follow
                
                
        // write the final package 
        filesystem.writeBytes(path:'package', bytes: output.buffer);
        
    },
    
    // like 'make' except that all the 
    // dependencies are bundled as trailing 
    // packages. This means the package can be executed 
    // independently without any additional packages 
    // needed to exist when the created package is 
    // run.
    'bundle' ::() {
    
    }

};

return ::(args) {
    filesystem.cwd = args[1];
    return commands[args[0]]();
};
