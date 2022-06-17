MatteCompiler = Module.cwrap('matte_js_run', 'string', ['string']);

runSource = function(){
    var editor = document.getElementById("editor-text-area");
    var outputArea = document.getElementById("output");
    
    
    outputArea.innerText = MatteCompiler(editor.value);
};

loadExample = function(exampleName) {
    var readTextFile = function (file) {
        var rawFile = new XMLHttpRequest();
        var text;
        rawFile.overrideMimeType("text/plain;");
        rawFile.open("GET", file, false);
        rawFile.addEventListener("load", 
            function () {
                text = rawFile.responseText;
            }, 
            false
        ); 
        rawFile.send(null);
        return text;
    }  
    
    document.getElementById("editor-text-area").value = readTextFile('/test-drive/samples/'+exampleName+'.mt');
    // pushes the draw to prism/live
    document.getElementById("editor-text-area").dispatchEvent(new InputEvent("input"));    
};



