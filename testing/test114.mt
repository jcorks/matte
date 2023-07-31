//// Test 114 
// "From the wild"
return ::<= {
    @hints = {
        NEUTRAL: 10
    }
    @CANVAS_HEIGHT = 40;
    @CANVAS_WIDTH = 40;
    @canvas = [];
    @canvasColors = [];
    @penx = 0;
    @peny = 0;
    @penColor = hints.NEUTRAL;
    @onCommit;
    @debugLines = [];
    
    @savestates = [];
    
   
    for(0, CANVAS_HEIGHT) ::(index) {
        @:line = [];
        @:lineColor = [];
        
        for(0, CANVAS_WIDTH) ::(ch) {
            line->push(value:' ');
            lineColor->push(value:hints.NEUTRAL);
        }
        
        canvas->push(value:line);
        canvasColors->push(value:lineColor);
    }
    return canvasColors[10][20];
}

