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
    return listen(to:::{
        for(in:[0, len], do:::(y){
            for(in:[0, len], do:::(x) {
                if (grid[y][x] == 1) ::<={
                    send(message:{
                        x : x,
                        y : y
                    });     
                };
            });
        });
    });
};    

@res = findPos(grid:a, len:5);
return ''+res.x+','+res.y;
    
