var editor = document.getElementById("texteditor");
var editor_locked = false;
editor.addEventListener("DOMCharacterDataModified", 
    function() {
        if (editor_locked) return;
        editor_locked = true;
        var sel = window.getSelection();
        var range = sel.getRangeAt(0);
        
        var editorTag = ':::::@' + Math.random() + '@:::::';
        var editorTagNode = document.createTextNode(editorTag);
        range.insertNode(editorTagNode);
        
        console.log(range);
                        


        
        var tagLocation = {};
        
        try {
            tagLocation = ApplyMatteColoringPre(editor, editorTag);        
        } catch(e) {
            console.log(e);
        }
        if (tagLocation.element != null) {
            console.log('index: ' + tagLocation.index);
            console.log(tagLocation.element.innerText);                            

            
            var newRange = document.createRange();


            newRange.setStart(tagLocation.element, tagLocation.index);
            newRange.setEnd(tagLocation.element, tagLocation.index);

            sel.removeAllRanges();        
            sel.addRange(newRange);
        } else {
            console.log('COULD NOT FIND TAGLOCATION');
        }
        editor_locked = false;
    }
);

