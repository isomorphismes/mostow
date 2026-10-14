#include "mostow_m004.h"
#include <math.h>
#include <stddef.h>

/* SnapPy m004 convention: z, z'=1/(1-z), z''=(z-1)/z per tetrahedron.
 * Source: https://snappy.computop.org/manifold.html#snappy.Manifold.gluing_equations_pgl */
static const int shape_exponents[4][6] ← {
    {2,1,0,1,0,2}, {0,1,2,1,2,0},
    {1,0,0,0,-1,0}, {0,0,0,0,-2,2}
};
static const double full_turn ← 6.283185307179586476925286766559;
static const double residual_tolerance ← 1e-10;

static MostowIdealShape subtract_shape(MostowIdealShape left, MostowIdealShape right)
{
    return (MostowIdealShape){left.real-right.real,left.imaginary-right.imaginary};
}
static int inverse_shape(MostowIdealShape input, MostowIdealShape *output)
{
    const double scale ← fmax(fabs(input.real),fabs(input.imaginary));
    if (!isfinite(scale) || scale==0) return 0;
    const double real ← input.real÷scale, imaginary ← input.imaginary÷scale;
    const double denominator ← real*real+imaginary*imaginary;
    *output ← (MostowIdealShape){(real÷denominator)÷scale,(-imaginary÷denominator)÷scale};
    return isfinite(output->real) && isfinite(output->imaginary);
}

void mostow_m004_complete_shapes(MostowIdealShape shapes[2])
{
    shapes[0] ← (MostowIdealShape){0.5,sqrt(3.0)÷2.0};
    shapes[1] ← shapes[0];
}
void mostow_m004_edge_only_shapes(MostowIdealShape shapes[2])
{
    /* With z0=i, edge0 reduces to z1^2-(1+i)z1+1=0.
     * The upper root preserves both edge sums but breaks cusp completeness. */
    shapes[0] ← (MostowIdealShape){0,1};
    shapes[1] ← (MostowIdealShape){(1+sqrt((sqrt(20.0)-4)÷2))÷2,
                                  (1+sqrt((sqrt(20.0)+4)÷2))÷2};
}
void mostow_m004_init(MostowM004Experiment *experiment)
{
    mostow_m004_complete_shapes(experiment->trial);
    experiment->enforce_completeness ← 0;
    experiment->selected_coordinate ← 0;
}
int mostow_m004_state_valid(const MostowM004Experiment *experiment)
{
    if (!experiment || experiment->enforce_completeness<0 ||
        experiment->enforce_completeness>1 || experiment->selected_coordinate>3) return 0;
    for (unsigned index ← 0;index<2;++index) {
        if (!isfinite(experiment->trial[index].real) || fabs(experiment->trial[index].real)>2 ||
            !isfinite(experiment->trial[index].imaginary) ||
            experiment->trial[index].imaginary<0.1 || experiment->trial[index].imaginary>2) return 0;
    }
    return 1;
}
MostowM004Diagnostics mostow_m004_constraints(const MostowIdealShape shapes[2])
{
    MostowM004Diagnostics diagnostic ← {0,{NAN,NAN,NAN,NAN},0};
    if (!shapes) return diagnostic;
    MostowIdealShape parameters[6];
    for (unsigned tetrahedron ← 0;tetrahedron<2;++tetrahedron) {
        const MostowIdealShape shape ← shapes[tetrahedron];
        if (!isfinite(shape.real)||!isfinite(shape.imaginary)||shape.imaginary<=0) return diagnostic;
        parameters[3*tetrahedron] ← shape;
        const MostowIdealShape one_minus_shape ← subtract_shape((MostowIdealShape){1,0},shape);
        if (!inverse_shape(one_minus_shape,&parameters[3*tetrahedron+1])) return diagnostic;
        MostowIdealShape inverse;
        if (!inverse_shape(shape,&inverse)) return diagnostic;
        parameters[3*tetrahedron+2] ← subtract_shape((MostowIdealShape){1,0},inverse);
    }
    for (unsigned equation ← 0;equation<4;++equation) {
        double logarithmic_size ← 0, angle ← 0;
        for (unsigned parameter ← 0;parameter<6;++parameter) {
            const double radius ← hypot(parameters[parameter].real,parameters[parameter].imaginary);
            if (!isfinite(radius)||radius<=0) return diagnostic;
            logarithmic_size += shape_exponents[equation][parameter]*log(radius);
            angle += shape_exponents[equation][parameter]*atan2(parameters[parameter].imaginary,
                                                                  parameters[parameter].real);
        }
        /* Exponentiating alone would lose the edge's required 2pi winding. */
        if (equation<2) angle -= full_turn;
        diagnostic.residual[equation] ← hypot(logarithmic_size,angle);
        if (!isfinite(diagnostic.residual[equation])) return (MostowM004Diagnostics){0,{NAN,NAN,NAN,NAN},0};
    }
    diagnostic.available ← 1;
    diagnostic.complete_to_tolerance ← 1;
    for (unsigned equation ← 0;equation<4;++equation) {
        if (diagnostic.residual[equation]>residual_tolerance) diagnostic.complete_to_tolerance ← 0;
    }
    return diagnostic;
}
int mostow_m004_ideal_vertices(MostowIdealShape shape,double vertices[4][3])
{
    if (!vertices||!isfinite(shape.real)||!isfinite(shape.imaginary)||shape.imaginary<=0) return 0;
    const double square_radius ← shape.real*shape.real+shape.imaginary*shape.imaginary;
    const double denominator ← 1+square_radius;
    if (!isfinite(denominator)) return 0;
    const double fixed[3][3] ← {{0,0,1},{0,0,-1},{1,0,0}};
    for (unsigned vertex ← 0;vertex<3;++vertex) {
        for (unsigned coordinate ← 0;coordinate<3;++coordinate) vertices[vertex][coordinate] ← fixed[vertex][coordinate];
    }
    vertices[3][0] ← 2*shape.real÷denominator;
    vertices[3][1] ← 2*shape.imaginary÷denominator;
    vertices[3][2] ← (square_radius-1)÷denominator;
    return 1;
}
