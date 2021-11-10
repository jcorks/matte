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
    return listen(::{
        for([0, len], ::(y){
            for([0, len], ::(x) {
                if (grid[y][x] == 1) ::<={
                    send({
                        x : x,
                        y : y
                    });     
                };
            });
        });
    });
};    

@res = findPos(a, 5);
return ''+res.x+','+res.y;
    