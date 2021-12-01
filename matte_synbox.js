var MatteSourceAnalysis = null;
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

    var type2desc = function(type) {
        switch(type) {
          case 'Numer Literal': return 'A simple number. Can be any valid floating point number.';
          case 'Boolean Literal': return 'A simple boolean. Can be either true or false';
          case 'Local Constant Declarator \'<@>\'': return 'Declares that a new constant variable is to be named. The token following this symbol is the name of the new variable';
          case 'Variable Name': return 'A variable name. When after a declarator, this is the name of a new variable. Variables can be numbers, strings, objects, function objects, or booleans.';
          case 'Function Constructor \'::\'': return 'Specifies that a new function should be created. Optionally can be followed by an argument list for when the function is called.';
          case 'Assignment Operator \'=\'': return 'Assigns a new value to the variable.';
        }

        return '';
    }

    var type2color = function(type) {
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

            case 'Variable Name':
                return color_varname;
                break;

                
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

    var transformElement = function(el) {
        el.style.backgroundColor = color_bg;
        el.style.margin = 'auto';
        el.style.padding = '10px';
        var analysis = MatteSourceAnalysis(el.innerText);
        console.log(analysis);

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



            while(parsed.line-1 == i) {
                var length = 0;
                if (parsed.comment != '[marker]' || parsed.line == 0) {
                    var from = lineIter;
                    length = (parsed.char - lineIter) + parsed.str.length-1;

                    console.log(parsed);

                    var subunitText = lines[i].substr(from, length);
                    var subel = createSubElement(subunitText, parsed.comment, type2color(parsed.comment), type2desc(parsed.comment));
                    el.appendChild(subel);
                }
                lineIter +=length;
                n++;
                parsed = parseLine(analysisLines[n]);

            }

            var nl = document.createElement('a');
            nl.innerText = '\n';
            el.appendChild(nl);

        }

        
    }


    Module['onRuntimeInitialized'] = function() {
        var el = document.body.getElementsByTagName('pre');
    
        for(var i = 0; i < el.length; ++i) {
            var p = el[i];
    
            transformElement(p);
        }
    }

} catch(e) {
    console.log(e);
}


