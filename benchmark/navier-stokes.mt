/**
 * Copyright 2012 the V8 project authors. All rights reserved.
 * Copyright 2009 Oliver Hunt <http://nerget.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
/*
var NavierStokes = new BenchmarkSuite('NavierStokes', 1484000,
                                      [new Benchmark('NavierStokes',
                                                     runNavierStokes,
                                                     setupNavierStokes,
                                                     tearDownNavierStokes)]);
*/
@solver;

@runNavierStokes = ::{
    solver.update();
};

@setupNavierStokes = ::{
    solver = FluidField();
    solver.setResolution(128, 128);
    solver.setIterations(20);
    solver.setDisplayFunction(::{});
    solver.setUICallback(prepareFrame);
    solver.reset();
};

@tearDownNavierStokes = ::{
    solver = empty;
};

@addPoints = ::(field) {
    @n = 64;
    for ([1, n+1],::(i) {
        field.setVelocity(i, i, n, n);
        field.setDensity(i, i, 5);
        field.setVelocity(i, n - i, -1*n, -1*n);
        field.setDensity(i, n - i, 20);
        field.setVelocity(128 - i, n + i, -1*n, -1*n);
        field.setDensity(128 - i, n + i, 30);
    });
};

@framesTillAddingPoints = 0;
@framesBetweenAddingPoints = 5;

@prepareFrame = ::(field) {
    if (framesTillAddingPoints == 0) ::{
        addPoints(field);
        framesTillAddingPoints = framesBetweenAddingPoints;
        framesBetweenAddingPoints = framesBetweenAddingPoints+1;
    }() else ::{
        framesTillAddingPoints = framesTillAddingPoints-1;
    }();
};

// Code from Oliver Hunt (http://nerget.com/fluidSim/pressure.js) starts here.
@FluidField = ::(canvas){
    @this = {};
    @addFields=::(x, s, dt)
    {
        for([0, size], ::(i){x[i] = x[i] + dt*s[i];});
    };

    @set_bnd = ::(b, x){
        (match(b) {
            (1): ::{
                for ([1, width+1], ::(i){
                    x[i] =  x[i + rowSize];
                    x[i + (height+1) *rowSize] = x[i + height * rowSize];
                });

                for ([1, height+1], ::(j){
                    x[j * rowSize] = -1*x[1 + j * rowSize];
                    x[(width + 1) + j * rowSize] = -1*x[width + j * rowSize];
                });
            },

            (2): ::{
                for ([1, width+1], ::(i){
                    x[i] = -1*x[i + rowSize];
                    x[i + (height + 1) * rowSize] = -1*x[i + height * rowSize];
                });

                for ([1, height+1], ::(j){
                    x[j * rowSize] =  x[1 + j * rowSize];
                    x[(width + 1) + j * rowSize] =  x[width + j * rowSize];
                });
            },

            default: ::{
                for ([1, width+1], ::(i){
                    x[i] =  x[i + rowSize];
                    x[i + (height + 1) * rowSize] = x[i + height * rowSize];
                });

                for ([1, height+1], ::(j){
                    x[j * rowSize] =  x[1 + j * rowSize];
                    x[(width + 1) + j * rowSize] =  x[width + j * rowSize];
                });
            }
        })();
        @maxEdge = (height + 1) * rowSize;
        x[0]                 = 0.5 * (x[1] + x[rowSize]);
        x[maxEdge]           = 0.5 * (x[1 + maxEdge] + x[height * rowSize]);
        x[(width+1)]         = 0.5 * (x[width] + x[(width + 1) + rowSize]);
        x[(width+1)+maxEdge] = 0.5 * (x[width + maxEdge] + x[(width + 1) + height * rowSize]);
    };

    @lin_solve = ::(b, x, x0, a, c)
    {
        if (a == 0 && c == 1) ::{
            for ([1, height+1], ::(j){
                @currentRow = j * rowSize;
                currentRow = currentRow+1;
                for ([1, width+1], ::(i){
                    x[currentRow] = x0[currentRow];
                    currentRow = currentRow+1;
                });
            });
            set_bnd(b, x);
        }() else ::{
            @invC = 1 / c;
            for ([0, iterations], ::(k){
                for ([1, height-1], ::(j) {
                    @lastRow = (j - 1) * rowSize;
                    @currentRow = j * rowSize;
                    @nextRow = (j + 1) * rowSize;
                    @lastX = x[currentRow];
                    currentRow = currentRow+1;
                    for ([1, width+1], ::(i) {
                        currentRow = currentRow+1;
                        lastRow = lastRow+1;
                        nextRow = nextRow+1;
                        lastX = x[currentRow] = (x0[currentRow] + a*(lastX+x[currentRow]+x[lastRow]+x[nextRow])) * invC;
                    });
                });
                set_bnd(b, x);
            });
        }();
    };

    @diffuse = ::(b, x, x0, dt)
    {
        @a = 0;
        lin_solve(b, x, x0, a, 1 + 4*a);
    };

    @lin_solve2 = ::(x, x0, y, y0, a, c)
    {
        if (a == 0 && c == 1) ::{
            for([1,height+1], ::(j) {
                @currentRow = j * rowSize;
                currentRow = currentRow+1;
                for([0, width], ::(i) {
                    x[currentRow] = x0[currentRow];
                    y[currentRow] = y0[currentRow];
                    currentRow = currentRow+1;
                });
            });
            set_bnd(1, x);
            set_bnd(2, y);
        }() else ::{
            @invC = 1/c;
            for ([0,iterations], ::(k) {
                for ([1, height+1], ::(j) {
                    @lastRow = (j - 1) * rowSize;
                    @currentRow = j * rowSize;
                    @nextRow = (j + 1) * rowSize;
                    @lastX = x[currentRow];
                    @lastY = y[currentRow];
                    currentRow = currentRow+1;
                    for ([1,width+1], ::(i) {
                        lastX = x[currentRow] = (x0[currentRow] + a * (lastX + x[currentRow] + x[lastRow] + x[nextRow])) * invC;
                        currentRow = currentRow+1;
                        lastRow = lastRow+1;
                        nextRow = nextRow+1;                        
                        lastY = y[currentRow] = (y0[currentRow] + a * (lastY + y[currentRow] + y[lastRow] + y[nextRow])) * invC;
                    });
                });
                set_bnd(1, x);
                set_bnd(2, y);
            });
        }();
    };

    @diffuse2 = ::(x, x0, y, y0, dt)
    {
        @a = 0;
        lin_solve2(x, x0, y, y0, a, 1 + 4 * a);
    };

    @advect = ::(b, d, d0, u, v, dt)
    {
        @Wdt0 = dt * width;
        @Hdt0 = dt * height;
        @Wp5 = width + 0.5;
        @Hp5 = height + 0.5;
        for([1,height+1], ::(j) {
            @pos = j * rowSize;
            for ([1,width+1], ::(i) {
                pos = pos+1;
                @x = i - Wdt0 * u[pos];
                @y = j - Hdt0 * v[pos];

                
                if (x < 0.5) ::{
                    x = 0.5;
                }();
    
                if (x > Wp5) ::{
                    x = Wp5;
                }();

                @i0 = x | 0;
                @i1 = i0 + 1;
                if (y < 0.5) ::{
                    y = 0.5;
                }();
                if (y > Hp5) ::{
                    y = Hp5;
                }();
                @j0 = y | 0;
                @j1 = j0 + 1;
                @s1 = x - i0;
                @s0 = 1 - s1;
                @t1 = y - j0;
                @t0 = 1 - t1;
                @row1 = j0 * rowSize;
                @row2 = j1 * rowSize;
                d[pos] = s0 * (t0 * d0[i0 + row1] + t1 * d0[i0 + row2]) + s1 * (t0 * d0[i1 + row1] + t1 * d0[i1 + row2]);
            });
        });
        set_bnd(b, d);
    };

    @project = ::(u, v, p, div) 
    {
        @h = -0.5 / introspect(width * height).sqrt();
        for ([1,height+1], ::(j) {
            @row = j * rowSize;
            @previousRow = (j - 1) * rowSize;
            @prevValue = row - 1;
            @currentRow = row;
            @nextValue = row + 1;
            @nextRow = (j + 1) * rowSize;
            for ([1,width+1], ::(i) {
                currentRow = currentRow+1;
                nextValue = nextValue+1;
                prevValue = prevValue+1;
                nextRow = nextRow+1;
                previousRow = previousRow+1;
                div[currentRow] = h * (u[nextValue] - u[prevValue] + v[nextRow] - v[previousRow]);
                p[currentRow] = 0;
            });
        });
        set_bnd(0, div);
        set_bnd(0, p);

        lin_solve(0, p, div, 1, 4 );
        @wScale = 0.5 * width;
        @hScale = 0.5 * height;
        for ([1,height+1], ::(j) {
            @prevPos = j * rowSize - 1;
            @currentPos = j * rowSize;
            @nextPos = j * rowSize + 1;
            @prevRow = (j - 1) * rowSize;
            @currentRow = j * rowSize;
            @nextRow = (j + 1) * rowSize;

            for ([1,width+1], ::(i) {
                currentPos = currentPos+1;
                nextRow = nextRow+1;
                prevRow = prevRow+1;
                u[currentPos] = u[currentPos] - wScale * (p[nextPos] - p[prevPos]);
                nextRow = nextRow+1;
                prevRow = prevRow+1;
                v[currentPos] = v[currentPos]-  hScale * (p[nextRow] - p[prevRow]);
            });
        });
        set_bnd(1, u);
        set_bnd(2, v);
    };

    @dens_step = ::(x, x0, u, v, dt)
    {
        addFields(x, x0, dt);
        diffuse(0, x0, x, dt );
        advect(0, x, x0, u, v, dt );
    };

    @vel_step = ::(u, v, u0, v0, dt)
    {
        @temp;
        addFields(u, u0, dt );
        addFields(v, v0, dt );
        temp = u0; u0 = u; u = temp;
        temp = v0; v0 = v; v = temp;
        diffuse2(u,u0,v,v0, dt);
        project(u, v, u0, v0);
        temp = u0; u0 = u; u = temp;
        temp = v0; v0 = v; v = temp;
        advect(1, u, u0, u0, v0, dt);
        advect(2, v, v0, u0, v0, dt);
        project(u, v, u0, v0 );
    };
    @uiCallback = ::(d,u,v) {};

    @Field = ::(dens, u, v) {
        @this = {};
        // Just exposing the fields here rather than using accessors is a measurable win during display (maybe 5%)
        // but makes the code ugly.
        this.setDensity = ::(x, y, d) {
             dens[(x + 1) + (y + 1) * rowSize] = d;
        };
        this.getDensity = ::(x, y) {
             return dens[(x + 1) + (y + 1) * rowSize];
        };
        this.setVelocity = ::(x, y, xv, yv) {
             u[(x + 1) + (y + 1) * rowSize] = xv;
             v[(x + 1) + (y + 1) * rowSize] = yv;
        };
        this.getXVelocity = ::(x, y) {
             return u[(x + 1) + (y + 1) * rowSize];
        };
        this.getYVelocity = ::(x, y) {
             return v[(x + 1) + (y + 1) * rowSize];
        };
        this.width = :: { return width; };
        this.height = :: { return height; };
        return this;
    };
    @queryUI = ::(d, u, v)
    {
        for ([0, size], ::(i) {
            u[i] = v[i] = d[i] = 0.0;
        });
        uiCallback(Field(d, u, v));
    };

    this.update = ::{
        queryUI(dens_prev, u_prev, v_prev);
        vel_step(u, v, u_prev, v_prev, dt);
        dens_step(dens, dens_prev, u, v, dt);
        displayFunc(Field(dens, u, v));
    };
    this.setDisplayFunction = ::(func) {
        displayFunc = func;
    };

    this.iterations = ::() { return iterations; };
    this.setIterations = ::(iters) {
        if (iters > 0 && iters <= 100) ::{
           iterations = iters;
        }();
    };
    this.setUICallback = ::(callback) {
        uiCallback = callback;
    };
    @iterations = 10;
    @visc = 0.5;
    @dt = 0.1;
    @dens;
    @dens_prev;
    @u;
    @u_prev;
    @v;
    @v_prev;
    @width = 0;
    @height = 0;
    @rowSize;
    @size;
    @displayFunc;

    @reset = ::{
        rowSize = width + 2;
        size = (width+2)*(height+2);
        dens = [];
        dens_prev = [];
        u = [];
        u_prev = [];
        v = [];
        v_prev = [];
        for ([0, size+1], ::(i) {
            dens_prev[i] = 0;
            u_prev[i] = 0;
            v_prev[i] = 0;
            dens[i] = 0;
            u[i] = 0;
            v[i] = 0;
        });
    };
    this.reset = reset;
    this.setResolution = ::(hRes, wRes)
    {
        @res = wRes * hRes;
        if (res > 0 && res < 1000000 && (wRes != width || hRes != height)) ::{
            width = wRes;
            height = hRes;
            reset();
            return true;
        }();
        return false;
    };
    this.setResolution(64, 64);
    return this;
};


setupNavierStokes();
for([0, 100], ::(i){
    print('Iteration ' + i + '\n');
    runNavierStokes();
});
tearDownNavierStokes();
