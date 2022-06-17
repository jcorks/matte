var MatteSourceAnalysis = null;
var ApplyMatteColoringPre;
try {
    MatteSourceAnalysis = Module.cwrap('matte_js_syntax_analysis', 'string', ['string']);
    
    // pre-formatted syntax block style
    var color_varname = "#268bd2";
    var color_neutral = "#586e75";
    var color_bg      = "#fdf6e3"; 
    var color_string  = "#2aa198";
    var color_number  = "#cb4b16";
    var color_keyword = "#859900";
    var color_symbol  = "#38a675";
    var color_method  = "#b58900";
    var color_comment = "#839496";
    var color_name    = "#8083e1";

    var type2desc = function(type) {
        switch(type) {
          case 'Numer Literal': return 'A simple number. Can be any valid floating point number.';
          case 'Boolean Literal': return 'A simple boolean. Can be either true or false';
          case 'Local Constant Declarator \'@:\'': return 'Declares that a new constant variable is to be named. The token following this symbol is the name of the new variable';
          case 'Local Variable Declarator \'@\'': return 'For variables that aren\'t function parameters, this lets you create new variables within the function context. The name after this symbol is the name of variable.';
          case 'Variable Name': return 'A variable name. When after a declarator, this is the name of a new variable. Variables can be numbers, strings, objects, function objects, or booleans.';
          case 'Function Constructor \'::\'': return 'Specifies that a new function should be created. Optionally can be followed by an argument list for when the function is called.';
          case 'Function Constructor Dash\'<=\'': return 'When specified, the new function being created is run immediately (with no parameter bindings) and is evaluated to its return value.';
          case 'Assignment Operator \'=\'': return 'Assigns a new value to the variable or writable expression.';
          case 'Variable Name Label': return 'The label to refer to a variable. This is most commonly for variables within functions or parameter bindings for calling functions.';
          case 'Function Content Block \'{\'': return 'The start of a function definition. Until an end } is detected, all statements past here make up the function being defined.';
          case 'Function Content Block \'}\'': return 'The end of a function definition. All statements since the first previous \'{\' define this function\'s behavior';
          case 'String Literal': return 'A series of characters that are defined in code directly. String literals are not changeable, but can be used to build larger strings and to refer to members within Objects.';
          case 'Function Argument List \'(\'': return 'When calling or defining a function, () symbols encase the arguments to the function, if present.';
          case 'Function Argument List \')\'': return 'When calling or defining a function, () symbols encase the arguments to the function, if present.';
          case 'Print built-in': return 'Built-in function provided by the Matte language to display Strings. Has one argument: message';
          case '\'return\' Statement': return 'When the return statement is reached in a function, the function halts and "sends" its the return expression to the calling context.';
          case '\'when\' Statement': return 'In the case that expression proceeding the \'when\' token evaluates to true, the function halts and "sends" its the final expression to the calling context.';
          case 'Function Parameter Specifier \':\'': return 'When calling functions, every parameter must be bound to the name of the arguments specified by the function definition. The name before this symbol is the binding name.';
          case 'Statement End \';\'': return 'Functions consist of separate statements, run one-at-a time. This symbol always marks the end of a single statement.';
          case 'Introspect built-in': return 'Introspect is a built-in value that gives a variety of functions to examine values further.';
        }


        return '';
    }

    var type2color = function(type, next) {
        switch(type) {
            case 'Number Literal': 
            case 'Boolean Literal': 
            case 'Empty Literal': 
                return color_number;
                break;            

            case 'Match built-in':
            case '\'when\' Statement':
            case '\'return\' Statement':
                return color_keyword;
                break;


            case 'String cast built-in':
            case 'Number cast built-in':
            case 'Boolean cast built-in':
            case 'Introspect built-in':
            case 'Print built-in':        
            case 'Error built-in':        
            case 'Get External Function built-in':
            case 'Import built-in':
            case 'Loop built-in':
            case 'For built-in':
            case 'Foreach built-in':
                return color_method;
                break;
            
            case 'Operator':
            case 'Function Constructor \'::\'':
            case 'Local Variable Declarator \'@\'':
            case 'Local Constant Declarator \'<@>\'':
            case 'Assignment Operator \'=\'':
                return color_symbol;
                break;

            case 'Variable Name Label':
                if (next == 'Function Parameter Specifier \':\'') {
                    return color_name;
                }
                return color_varname;
                break;
            case 'Function Parameter Specifier \':\'':
                return color_name;

                
            case 'String Literal': 
                return color_string;
                break;

        }

        return color_neutral;
    }


    // creates a new subelement
    var createSubElement = function(inner, comment, fgColor, desc) {
        var out = document.createElement('code');
        out.innerText = inner;
        out.style.color = fgColor;
        out.style.fontSize = "13pt";
        out.style.borderWidth = "2px";
        out.style.borderStyle = 'solid';
        out.style.borderColor = 'rgba(0, 0, 0, 0)';
        out.style.fontFamily = 'Consolas';
        if (fgColor == color_name)
            out.style.fontStyle = 'italic';
            
        out.style.paddingBottom = '2px';

        out.addEventListener('mouseenter', function(event) {
            if (out.style.borderColor == color_bg) return;
            out.style.borderColor = fgColor;

            out.hoverElement = document.createElement('div');
            var hoverElement = out.hoverElement;
            hoverElement.style.position = 'fixed';
            hoverElement.style.zIndex = 3;
            hoverElement.style.backgroundColor = 'black';
            hoverElement.style.color = '#aeaeae';
            hoverElement.style.fontSize = '12pt';
            hoverElement.style.fontFamily = 'Consolas';
            hoverElement.style.padding = '10px';
            hoverElement.style.width = '300px';
            hoverElement.style.wordWrap = 'normal';
    

            var title = document.createElement('div');
            hoverElement.appendChild(title);
            title.innerText = comment;
            title.style.fontSize = '12pt';
            hoverElement.appendChild(document.createElement('hr'));
            var rect = out.getBoundingClientRect();


            var text = document.createElement('div');
            text.innerText = desc;
            hoverElement.appendChild(text);
            text.style.fontFamily = 'sans serif';
            text.style.fontSize = '10pt';
            

            hoverElement.listenerFn = function(ev) {
                hoverElement.style.left = rect.left + ev.clientX+'px';
                hoverElement.style.top  = rect.top +  35 +'px';
            };
            hoverElement.style.left = rect.left + 'px';
            hoverElement.style.top  = rect.top + 35+'px';
            //window.addEventListener('mousemove', hoverElement.listenerFn);
            document.body.appendChild(hoverElement);
        });


        out.addEventListener('mouseleave', function(event) {
            out.style.borderColor = 'rgba(0, 0, 0, 0)';
            if (out.hoverElement) {
                window.removeEventListener('mousemove', out.hoverElement.listenerFn);
                out.hoverElement.remove();
            }
        });
        
        return out;
    }

    var parseLine = function(aline) {
        if (!aline) return {
            line : 0,
            char : 0,
            comment : '',
            str : ''
        };
        var elts = aline.split('\t');
        if (elts.length != 4) return {
            line : 0,
            char : 0,
            comment : '',
            str : ''
        };
        return {
            line : parseInt(elts[0].split(':')[1]),
            char : parseInt(elts[1].split(':')[1]),
            comment : elts[2].trim(),
            str : elts[3].trim()
        };
    }

    var transformElement = function(el, searchTag) {

        var tagLocation = -1;
        if (searchTag != null) {
            tagLocation = el.innerText.search(searchTag);
            el.innerText = el.innerText.replace(searchTag, "");
            
            // for normal text
            tagLocation++;
        }

        var analysis = MatteSourceAnalysis(el.innerText);
        el.style.backgroundColor = color_bg;
        el.style.margin = 'auto';
        el.style.padding = '10px';

        //console.log(analysis);
        

        var analysisLines = analysis.split('\n');
        var lines = el.innerText.split('\n');
        el.innerText = '';

        var n = 0;
        for(var i = 0; i < lines.length; ++i) {
            const trimmed = lines[i].trim();
            if (trimmed[0] == '/' && trimmed[1] == '/') {
                var subel = createSubElement(lines[i], 'Single-line comment', color_comment, "A comment that spans a single line. This is ignored by the language compiler.");
                el.appendChild(subel);

                var nl = document.createElement('a');
                nl.innerText = '\n';
                el.appendChild(nl);
                continue;
            }

            if (trimmed[0] == '/' && trimmed[1] == '*') {
                var subel = createSubElement(lines[i], 'Multi-line comment', color_comment, "A comment that can span multiple lines. This is ignored by the language compiler.");
                el.appendChild(subel);

                var nl = document.createElement('a');
                nl.innerText = '\n';
                el.appendChild(nl);
                continue;
            }


            var lineIter = 0;
            var parsed = parseLine(analysisLines[n]);
            var parsedNext = parseLine(analysisLines[n+1]);


            while(parsed.line-1 == i) {
                var length = 0;
                if (parsed.comment != '[marker]' || parsed.line == 0) {
                    var from = lineIter;
                    length = (parsed.char - lineIter) + parsed.str.length-1;

                    //console.log(parsed);

                    var subunitText = lines[i].substr(from, length);
                    var subel = createSubElement(subunitText, parsed.comment, type2color(parsed.comment, parsedNext.comment), type2desc(parsed.comment));
                    el.appendChild(subel);
                }
                lineIter +=length;
                n++;
                parsed = parsedNext;
                parsedNext = parseLine(analysisLines[n+1]);

            }

            var nl = document.createElement('a');
            nl.innerText = '\n';
            el.appendChild(nl);

        }
        
        // if requested, need to find node+textIndex of where this innerText offset is.
        if (tagLocation != -1) {    
            var curIndex = 0;
        
            var searchRec = function(node) {
                for(var i = 0; i < node.childNodes.length; ++i) {
                    var child = node.childNodes[i];

                    if (tagLocation >= curIndex &&
                        tagLocation <  curIndex+child.innerText.length
                    ) {
                        console.log('@' + tagLocation + '/' + el.innerText.length + ':"' + child.innerText + '"->' + curIndex + ', ' + (curIndex+child.innerText.length));
                        if (child.children.length) {
                            return searchRec(child);
                        } else {

                            return {
                                element : child,
                                index : tagLocation-curIndex
                            }
                        }                       
                    }
                    curIndex += child.innerText.length;
                }
                
                
                var endNode = node.childNodes[node.childNodes.length-1];
                return {
                    element: endNode,
                    index:endNode.innerText.length-1
                }
            };
            
            
            var result = searchRec(el);
            
            // get the actual text node
            if (result.element.nodeType == 3)
                return result;
            console.log(result.element.nodeType);
                
            for(var i = 0; i < result.element.childNodes.length; ++i) {
                var child = result.element.childNodes[i];
                console.log(child.nodeType);
                
                if (child.nodeType == 3) {
                    result.element = child;
                    return result;
                }
                    
            }
        }
        
    }
    ApplyMatteColoringPre = transformElement;

    Module['onRuntimeInitialized'] = function() {
        var el = document.body.getElementsByTagName('pre');
    
        for(var i = 0; i < el.length; ++i) {
            var p = el[i];
            if (p.id != 'doc-source') return;
    
            transformElement(p, null);
        }
        
    }

} catch(e) {
    console.log(e);
}


