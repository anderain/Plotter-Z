#ifndef _PZ_SAMPLES_
#define _PZ_SAMPLES_

#if defined(_SH3) || defined(_SH4)
#   ifndef PLATFORM_FX9860
#       define PLATFORM_FX9860
#   endif
#endif

#ifndef FUNC_TYPE_CARTESIAN
#   define FUNC_TYPE_CARTESIAN     0
#endif
#ifndef FUNC_TYPE_PARAMETRIC
#   define FUNC_TYPE_PARAMETRIC    1
#endif

#define SAMPLE_PI 3.14159265f

typedef struct tagPzSample {
    int iFuncType;
    float xMin;
    float xMax;
    float yMin;
    float yMax;
    float zMin;
    float zMax;
    float uMin;
    float uMax;
    float vMin;
    float vMax;
    const char* szName;
    const char* szExpr[3];
} PzSample;

#define U_V_DOES_NOT_MATTER -1,1,-1,1

/*
    <Samples>
    the following lines can be passed as command-line
    argumentsto `pz-sdl` to directly view the effect.
 * ------------------------------------------------------------
    CARTESIAN
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
 * ------------------------------------------------------------
    PARAMETRIC
 * ------------------------------------------------------------
    "5*sin(u)*cos(v);5*sin(u)*sin(v);5*cos(u)" -x -5,5,10 -y -5,5,10 -z -5,5 -u -0,6.2831852,20 -v -0,6.2831852,20 -p
    "(3+3/2*cos(v))*cos(u);(3+3/2*cos(v))*sin(u);3/2*sin(v)" -x -5,5,10 -y -5,5,10 -z -5,5 -u -0,6.2831852,20 -v -0,6.2831852,20 -p
    "(5+v*cos(u/2))*cos(u);(5+v*cos(u/2))*sin(u);v*sin(u/2)" -x -8,8,10 -y -8,8,10 -z -3,3 -u -0,6.2831852,20 -v -3,3,20 -p
    "v*cos(u);v*sin(u);2*u" -x -8,8,10 -y -8,8,10 -z -13,13,10 -u -6.2831852,6.2831852,20 -v 0,8,20 -p
    "(2+cos(u/2)*sin(v)-sin(u/2)*sin(2*v))*cos(u);(2+cos(u/2)*sin(v)-sin(u/2)*sin(2*v))*sin(u);sin(u/2)*sin(v)+cos(u/2)*sin(2*v)" -x -4,4,10 -y -4,4,10 -z -2,2 -u 0,6.2831852,20 -v 0,6.2831852,20 -p
    "cos(u)*sin(v);sin(u)*sin(v);cos(v)+ln(tan(v/2))+u/5" -x -1,1,10 -y -1,1,10 -z -3,3.51327412 -u 0,12.56637,20 -v 0.05,2,20 -p
    "u-u^3/3+u*v^2;v-v^3/3+v*u^2;u^2-v^2" -x -7.5,7.5,10 -y -7.5,7.5,10 -z -4,4 -u -2,2,20 -v -2,2,20 -p
    "2*cos(u)*sin(2*v);2*sin(u)*sin(2*v);4*cos(u)*sin(u)*(cos(v))^2" -x -2,2,10 -y -2,2,10 -z -2,2 -u 0,3.1415926,20 -v 0,3.1415926,20 -p
*/

static const PzSample PlotterZSamples[] = {
#ifdef PLATFORM_FX9860
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -6.0f, 6.0f,
        /* y range */   -6.0f, 6.0f,
        /* z range */   -6.0f, 6.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Dynamic Waves",
        /* expr */      { "sin(x+t/5)+cos(y+t/5)", 0, 0 }
    },
#endif
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -3.0f, 3.0f,
        /* y range */   -3.0f, 3.0f,
        /* z range */   -7.5f, 8.5f,
        U_V_DOES_NOT_MATTER,
        /* name */      "The MATLAB Peaks",
        /* expr */      { "3*(1-x)^2*exp(-x^2-(y+1)^2)-10*(x/5-x^3-y^5)*exp(-x^2-y^2)-exp(-(x+1)^2-y^2)/3", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -4.0f, 4.0f,
        /* y range */   -4.0f, 4.0f,
        /* z range */   -0.0f, 15.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Ackley Function",
        /* expr */      { "-20*exp(-1/5*sqr(1/2*(x^2+y^2)))-exp(1/2*(cos(2*pi*x)+cos(2*pi*y)))+exp(1)+20", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -1.0f, 1.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Asymmetric Ripple Surface",
        /* expr */      { "exp(-abs(2*x/3))*cos(sqr(x^2+y^2))", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -6.0f, 6.0f,
        /* y range */   -6.0f, 6.0f,
        /* z range */   -3.0f, 3.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Concentric Ripple Function",
        /* expr */      { "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -2.0f, 2.0f,
        /* y range */   -1.0f, 3.0f,
        /* z range */   -1.0f, 10.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Rosenbrock Function",
        /* expr */      { "((1-x^2)+100*(y-x^2)^2)/100", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -5.0f, 5.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Saddle Surface",
        /* expr */      { "(-x^2+y^2)/5", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -5.0f, 5.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Egg Carton Surface",
        /* expr */      { "sin(x)+cos(y)", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -2.0f, 4.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Logarithmic Sink",
        /* expr */      { "ln(x^2+y^2+1/20)", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_CARTESIAN,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -2.0f, 4.0f,
        U_V_DOES_NOT_MATTER,
        /* name */      "Absolute Sine Grid",
        /* expr */      { "abs(sin(x))*abs(sin(y))", 0, 0 }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -5.0f, 5.0f,
        /* u range */   0, 2 * SAMPLE_PI,
        /* v range */   0, 2 * SAMPLE_PI,
        /* name */      "Sphere",
        /* expr */ {
            "5*sin(u)*cos(v)",
            "5*sin(u)*sin(v)",
            "5*cos(u)"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -5.0f, 5.0f,
        /* y range */   -5.0f, 5.0f,
        /* z range */   -5.0f, 5.0f,
        /* u range */   0, 2 * SAMPLE_PI,
        /* v range */   0, 2 * SAMPLE_PI,
        /* name */      "Torus",
        /* expr */ {
            "(3+3/2*cos(v))*cos(u)",
            "(3+3/2*cos(v))*sin(u)",
            "3/2*sin(v)"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -8.0f, 8.0f,
        /* y range */   -8.0f, 8.0f,
        /* z range */   -3.0f, 3.0f,
        /* u range */   0, 2 * SAMPLE_PI,
        /* v range */   -1.0f, 1.0f,
        /* name */      "Mobius Strip",
        /* expr */ {
            "(5+v*cos(u/2))*cos(u)",
            "(5+v*cos(u/2))*sin(u)",
            "v*sin(u/2)"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -8.0f, 8.0f,
        /* y range */   -8.0f, 8.0f,
        /* z range */   -13.0f, 13.0f,
        /* u range */   -2 * SAMPLE_PI, 2 * SAMPLE_PI,
        /* v range */   0, 8,
        /* name */      "Helicoid",
        /* expr */ {
            "v*cos(u)",
            "v*sin(u)",
            "2*u"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -4.0f, 4.0f,
        /* y range */   -4.0f, 4.0f,
        /* z range */   -2.0f, 2.0f,
        /* u range */   0, 2 * SAMPLE_PI,
        /* v range */   0, 2 * SAMPLE_PI,
        /* name */      "Figure-8 Klein Bottle",
        /* expr */ {
            "(2+cos(u/2)*sin(v)-sin(u/2)*sin(2*v))*cos(u)",
            "(2+cos(u/2)*sin(v)-sin(u/2)*sin(2*v))*sin(u)",
            "sin(u/2)*sin(v)+cos(u/2)*sin(2*v)"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -1.0f, 1.0f,
        /* y range */   -1.0f, 1.0f,
        /* z range */   -3.0f, (1.0f + 4.0f * SAMPLE_PI / 5.0f),
        /* u range */   0, 12.56637,
        /* v range */   0.05, 2,
        /* name */      "Dini's Surface",
        /* expr */ {
            "cos(u)*sin(v)",
            "sin(u)*sin(v)",
            "cos(v)+ln(tan(v/2))+u/5"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -7.5f, 7.5f,
        /* y range */   -7.5f, 7.5f,
        /* z range */   -4.0f, 4.0f,
        /* u range */   -2, 2,
        /* v range */   -2, 2,
        /* name */      "Enneper Surface",
        /* expr */ {
            "u-u^3/3+u*v^2",
            "v-v^3/3+v*u^2",
            "u^2-v^2"
        }
    },
    {
        /* Type */      FUNC_TYPE_PARAMETRIC,
        /* x range */   -2.0f, 2.0f,
        /* y range */   -2.0f, 2.0f,
        /* z range */   -2.0f, 2.0f,
        /* u range */   0, SAMPLE_PI,
        /* v range */   0, SAMPLE_PI,
        /* name */      "Steiner's Roman Surface",
        /* expr */ {
            "2*cos(u)*sin(2*v)",
            "2*sin(u)*sin(2*v)",
            "4*cos(u)*sin(u)*(cos(v))^2"
        }
    }
};

#endif