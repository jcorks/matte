//// Test95 
//
// Testing throw/catch as flow control
@a = [
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 1, 0]
];



@findPos =::(grid, len) {
    return [::]{
        [0, len]->for(do:::(y){
            [0, len]->for(do:::(x) {
                if (grid[y][x] == 1) ::<={
                    send(message:{
                        x : x,
                        y : y
                    });     
                };
            });
        });
    };
};    

@res = findPos(grid:a, len:5);
return ''+res.x+','+res.y;
    
