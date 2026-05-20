#ifndef _PZ_SAMPLES_
#define _PZ_SAMPLES_

typedef struct tagPzSample {
    float xMin;
    float xMax;
    float yMin;
    float yMax;
    float zMin;
    float zMax;
    char* szExpr;
} PzSample;

/*
    <Samples>
    the following lines can be passed as command-line
    argumentsto `pz-sdl` to directly view the effect.
 * ------------------------------------------------------------
    "3*(1-x)^2*exp(-x^2-(y+1)^2)-10*(x/5-x^3-y^5)*exp(-x^2-y^2)-exp(-(x+1)^2-y^2)/3" -x -3,3,40 -y -3,3,40 -z -7.5,8.5
    "exp(-abs(2*x/3))*cos(sqr(x^2+y^2))" -x -5,5,20 -y -5,5,20 -z -1,1
    "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))" -x -6,6,20 -y -6,6,20 -z -3,3
    "(-x^2+y^2)/5" -x -5,5,20 -y -5,5,20 -z -5,5
    "sin(x)+cos(y)" -x -5,5,20 -y -5,5,20 -z -5,5
    "ln(x^2+y^2+1/20)" -x -5,5,20 -y -5,5,20 -z -2,4
    "abs(sin(x))*abs(sin(y))" -x -5,5,20 -y -5,5,20 -z -2,4
    "((1-x^2)+100*(y-x^2)^2)/100" -x -2,2,20 -y -1,3,20 -z -1,10
    "-20*exp(-1/5*sqr(1/2*(x^2+y^2)))-exp(1/2*(cos(2*pi*x)+cos(2*pi*y)))+exp(1)+20" -x -4,4,30 -y -4,4,30 -z -0,15
*/

static const PzSample PlotterZSamples[] = {
    { /* x range */ -3.0f, 3.0f, /* y range */ -3.0f, 3.0f, /* z range */ -7.5f, 8.5f, /* expr */ "3*(1-x)^2*exp(-x^2-(y+1)^2)-10*(x/5-x^3-y^5)*exp(-x^2-y^2)-exp(-(x+1)^2-y^2)/3" },
    { /* x range */ -4.0f, 4.0f, /* y range */ -4.0f, 4.0f, /* z range */ -0.0f, 15.0f, /* expr */ "-20*exp(-1/5*sqr(1/2*(x^2+y^2)))-exp(1/2*(cos(2*pi*x)+cos(2*pi*y)))+exp(1)+20" },
    { /* x range */ -5.0f, 5.0f, /* y range */ -5.0f, 5.0f, /* z range */ -1.0f, 1.0f, /* expr */ "exp(-abs(2*x/3))*cos(sqr(x^2+y^2))" },
    { /* x range */ -6.0f, 6.0f, /* y range */ -6.0f, 6.0f, /* z range */ -3.0f, 3.0f, /* expr */ "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))" },
    { /* x range */ -2.0f, 2.0f, /* y range */ -1.0f, 3.0f, /* z range */ -1.0f, 10.0f, /* expr */ "((1-x^2)+100*(y-x^2)^2)/100" },
    { /* x range */ -5.0f, 5.0f, /* y range */ -5.0f, 5.0f, /* z range */ -5.0f, 5.0f, /* expr */ "(-x^2+y^2)/5" },
    { /* x range */ -5.0f, 5.0f, /* y range */ -5.0f, 5.0f, /* z range */ -5.0f, 5.0f, /* expr */ "sin(x)+cos(y)" },
    { /* x range */ -5.0f, 5.0f, /* y range */ -5.0f, 5.0f, /* z range */ -2.0f, 4.0f, /* expr */ "ln(x^2+y^2+1/20)" },
    { /* x range */ -5.0f, 5.0f, /* y range */ -5.0f, 5.0f, /* z range */ -2.0f, 4.0f, /* expr */ "abs(sin(x))*abs(sin(y))" }
};

#endif