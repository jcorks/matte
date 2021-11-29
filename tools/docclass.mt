@MatteString = import('Matte.Core.String');
@Array       = import('Matte.Core.Array');
@class       = import('Matte.Core.Class');
@JSON        = import('Matte.Core.JSON');


// returns an object in the following format:
/*

{
    classes : [
        {
            name : 'class name',
            info : 'Comments above the class'
            inherits : {array of class names OR empty},
            
            functions : [
                name : 'Name of var',
                spec : {function spec OR empty},
                ...
            ],        

            variables : [
                {
                    name : 'Name of var',
                    info : 'This is whats in comments',
                    read : {function spec OR empty}
                    write : {function spec OR empty}
                },
                ...
            ],
            
            
            attributes : [
                {
                    name    : 'name/label for operator',
                    info    : 'Comments above the operator'                     
                },
                ...        
            ]
        },
        ...    
    ]
}


// where {function spec} is 
{
    info : 'This is what is in comments above the function',
    args : [
        {
            name : 'name of variable',
            type : {empty OR raw string of type}
        }
    ],
    
    returns : {empty OR raw string of type}
}
*/

@removeLeadingSpace ::(l) {
    // remove leading space.
    loop(::{
        when(l.length > 0 && (l[0] == ' ' || l[0] == '\t')) ::<={
            l.removeChar(0);
            return true;
        };
        return false;
    });
};



@docAll ::(instr) {
    <@>lines = MatteString.new(instr).split('\n');
    for([0, lines.length], ::(i){
        @l = lines[i];
        l.replace('\r', '');
        removeLeadingSpace(l);
    });
    @doc = {classes:[]};
    
    
    // fetches the comments corresponding to the given 
    // line. This can either be a contiguous block of // comments 
    // or it can be a /* */
    @fetchComment ::(line) {
        @end = line-1;
        when(end < 0) '';
        when(lines[end].length < 2) '';
        @output = '';
        return match(true) {
        
            // contiguous
            (lines[end][0] == '/' && lines[end][1] == '/'): ::<= {
                @l = lines[end];
                output = String(l.substr(2, l.length-1));
                @current = end-1;
                loop(::{
                    when(current < 0) false;
                    l = lines[current];
                    when(l.length == 0) false;
                    when(!(l[0] == '/' && l[1] == '/')) false;
                    
                    output = String(l.substr(2, l.length-1)) + output;
                    current -= 1;
                    return true;
                });
                
                return output;
            },
            
            // comment block
            (lines[end].contains('*/')): ::<= {
                @l = lines[end].substr(0, lines[end].length-1);
                output = l.replace('*/', '');
                
                @current = end-1;
                loop(::{
                    when(current < 0) false;
                    @l = lines[current];
                    
                    when(l.contains('/*')) ::<= {
                        l = l.substr(0, l.length);
                        l.replace('/*', '');
                        output = String(l) + output;                        
                        return false;   
                    };
                    
                    output = String(l) + output;
                    current -= 1;
                    return true;
                });
                
                return output;
            },

            // no comment block detected...
            default: '' 
        
        };
    };
    
    @getStartEnd::(startLine, token, startChar, endChar) {
        // next: functions + variables
        // requires finding interface
        @start = startLine;
        
        if (token != empty) ::<= {
            start = listen(::{
                for([startLine, lines.length], ::(i) {
                    if (lines[i].contains(token)) ::<={
                        send(i);
                    };
                });
            });
        };

        when(start == empty) empty;        
    
        
        @end = start;
        @inout = 0;
        loop(::{
            when(end >= lines.length) false;
            inout += lines[end].count(startChar);
            inout -= lines[end].count(endChar);
            when(inout == 0) false;
            end += 1;
            return true;
        });

        return {
            start: start,
            end : end
        };
    };
    
    
    @docFunction ::(startLine, endLine) {
        @spec = {};
        spec.info = fetchComment(startLine);
        spec.args = [];
        @argcount = 0;
        
        // TODO: args / return;
        return spec;        
        
    };
    
    @docClass ::(startLine) {
        @skipTo = startLine;
        
        // first, get comment above class!
        @block = {
            info : fetchComment(startLine)
        };
        
        
        // now we can find the end of the class definition
        @endLine = getStartEnd(startLine, empty, '{', '}').end;

        
        print('Getting class name..');
        // get name.
        listen(::{
            for([startLine, endLine], ::(i) {
                if (lines[i].contains('name')) ::<={
                    @r = lines[i].scan(':[%]')[0];
                    r.replace(',', '');
                    block.name = String(r);
                    send(empty);
                };               
            });
        });
        
        // inherits
        print('Getting class inherits..');
        listen(::{
            for([startLine, endLine], ::(i) {
                if (lines[i].contains('inherits')) ::<={
                    block.inherits = [];
                    @arrLine = i;
                    listen(::{
                        for([i, endLine], ::(n) {
                            when(lines[n].contains(']')) ::<={
                                arrLine = n;
                                send(empty);
                            };
                        });
                    });                    
                    
                    for([i, arrLine+1], ::(n){
                        @s = lines[n].substr(0, lines[n].length);
                        s.replace('[', '');     
                        s.replace('[', '');
                        
                        
                        @names = s.split(',');
                        for([0, names.length], ::(k) {
                            if (names[i] != 'inherits') ::<= {
                                block.inherits[introspect.length(block)] = names[k];
                            };
                        });
                    });
                                        
                    send(empty);
                };             
            });
        });
        
        // next: functions + variables
        // requires finding interface
        @interface = getStartEnd(startLine, 'interface', '{', '}');
        when(interface == empty) endLine;
        block.functions = [];
        block.variables = [];
        
        print('Getting class interface..');

        for([interface.start, interface.end+1], ::(i) {
            when (lines[i].contains('::')) ::<={
                @bounds = getStartEnd(i, empty, '{', '}');
                @f = {};
                f.name = lines[i].scan('[%]::')[0];
                print('- Function:' + f.name);

                // TODO: remove trailing space?
                f.spec = docFunction(i, bounds.end);
                
                block.functions[introspect.keycount(block.functions)] = f;
                
                // skip internal symbols
                return bounds.end+1;
            };
            
            when(lines[i].contains(':')) ::<={
                @v = {};
                v.name = lines[i].scan('[%]:')[0];
                v.info = fetchComment(i);
               
                print('- Member:' + v.name);
                 
                // TODO: get read / write
                
                block.functions[introspect.keycount(block.functions)] = v;
                return getStartEnd(i, empty, '{', '}').end+1;            
            };
        });
        
                
        
    
        // TODO: attributes
    
    
        doc.classes[introspect.keycount(doc.classes)] = block;
        return endLine;  
    };
    
    
    for([0, lines.length], ::(i) {
        when (lines[i].contains('class(')) ::<= {
            return docClass(i);
        };
    });
    
    return doc;
};



@DocClass = class({
    name : 'DocClass',
    
    define::(this) {
        this.interface({
            documentAsHTML::(str => String) => String {
                error('TODO');
            },
            
            documentAsJSON::(str => String) => String {
                return JSON.encode(docAll(str));
            }
        });
    }
}).new();




//return DocClass;


@Filesystem   = import('Matte.System.Filesystem');
//@DocClass     = import('../tools/docclass.mt');



print(DocClass.documentAsJSON(Filesystem.readString('../src/rom/core/array.mt')));




