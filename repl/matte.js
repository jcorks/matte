MatteREPL = Module.cwrap('matte_js_repl', 'string', ['string']);

runSource = function(){
    var editor = document.getElementById("editor-text-area");
    var outputArea = document.getElementById("output");
    
    
    outputArea.innerText += MatteREPL(editor.value);
};


var editor = document.getElementById("editor-text-area");
editor.addEventListener("keyup", function(event) {
    if (event.keyCode === 13) {
        event.preventDefault();
        runSource();
        editor.value = '';
        var outputArea = document.getElementById("scrollable");
        outputArea.scrollTo(0, outputArea.scrollHeight);    
    }
})

Module['onRuntimeInitialized'] = function() {
    runSource();
}

