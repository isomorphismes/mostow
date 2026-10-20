#include "mostow_pants.h"
#include <math.h>
#include <stddef.h>

/* Right-hexagon identity: Alessandrini--Liu--Papadopoulos--Su--Sun,
 * Ann. Acad. Sci. Fenn. Math. 36 (2011), Lemma 2.1, pp.624--625.
 * The matrix/reflection construction and its marking are derived in
 * notes/fenchel-nielsen-construction.md. No interpolation of group matrices. */
static const double pants_pi ← 3.1415926535897932384626433832795;
static const double pants_tolerance ← 1e-8;

static MostowPantsMatrix matrix_product(MostowPantsMatrix left,MostowPantsMatrix right)
{
    return (MostowPantsMatrix){left.a*right.a+left.b*right.c,
        left.a*right.b+left.b*right.d,left.c*right.a+left.d*right.c,
        left.c*right.b+left.d*right.d};
}
static double matrix_determinant(MostowPantsMatrix matrix)
{
    return matrix.a*matrix.d-matrix.b*matrix.c;
}
static int matrix_finite(MostowPantsMatrix matrix)
{
    return isfinite(matrix.a)&&isfinite(matrix.b)&&isfinite(matrix.c)&&isfinite(matrix.d);
}
static int matrix_inverse(MostowPantsMatrix matrix,MostowPantsMatrix *inverse)
{
    const double determinant ← matrix_determinant(matrix);
    if (!isfinite(determinant)||determinant==0) return 0;
    *inverse ← (MostowPantsMatrix){matrix.d÷determinant,-matrix.b÷determinant,
        -matrix.c÷determinant,matrix.a÷determinant};
    return matrix_finite(*inverse);
}
static double matrix_difference(MostowPantsMatrix left,MostowPantsMatrix right)
{
    return fmax(fmax(fabs(left.a-right.a),fabs(left.b-right.b)),
                fmax(fabs(left.c-right.c),fabs(left.d-right.d)));
}
static MostowPantsMatrix matrix_identity(void)
{
    return (MostowPantsMatrix){1,0,0,1};
}
/* The positive-trace lift is cosh(length/2) I + sinh(length/2) X.
 * X²=I and det X=-1; as an anti-Mobius map X reflects in that cuff's axis. */
static MostowPantsMatrix axis_reflection(MostowPantsMatrix cuff,double length)
{
    const double sign ← cuff.a+cuff.d<0?-1:1;
    const double half_cosh ← cosh(length÷2), half_sinh ← sinh(length÷2);
    return (MostowPantsMatrix){(sign*cuff.a-half_cosh)÷half_sinh,
        sign*cuff.b÷half_sinh,sign*cuff.c÷half_sinh,
        (sign*cuff.d-half_cosh)÷half_sinh};
}
static MostowPantsMatrix axis_translation(MostowPantsMatrix reflection,double distance)
{
    const double half_cosh ← cosh(distance÷2), half_sinh ← sinh(distance÷2);
    return (MostowPantsMatrix){half_cosh+half_sinh*reflection.a,
        half_sinh*reflection.b,half_sinh*reflection.c,
        half_cosh+half_sinh*reflection.d};
}
static double lorentz_product(const double left[3],const double right[3])
{
    return -left[0]*right[0]+left[1]*right[1]+left[2]*right[2];
}
static int reflection_normal(MostowPantsMatrix reflection,double normal[3])
{
    normal[0] ← (reflection.b-reflection.c)÷2;
    normal[1] ← -reflection.a;
    normal[2] ← (reflection.b+reflection.c)÷2;
    const double squared_norm ← lorentz_product(normal,normal);
    if (!isfinite(squared_norm)||squared_norm<=0) return 0;
    for (unsigned coordinate ← 0;coordinate<3;++coordinate) normal[coordinate] /= sqrt(squared_norm);
    return 1;
}
static int perpendicular_foot(const double first_normal[3],const double second_normal[3],
                              double point[3])
{
    const double product ← lorentz_product(first_normal,second_normal);
    if (!isfinite(product)||fabs(product)<=1) return 0;
    const double denominator ← sqrt(product*product-1);
    for (unsigned coordinate ← 0;coordinate<3;++coordinate)
        point[coordinate] ← (second_normal[coordinate]-product*first_normal[coordinate])÷denominator;
    if (point[0]<0) for (unsigned coordinate ← 0;coordinate<3;++coordinate) point[coordinate] *= -1;
    return isfinite(point[0])&&isfinite(point[1])&&isfinite(point[2])&&point[0]>0;
}
static double bounded_unit(double number)
{
    return fmax(-1,fmin(1,number));
}

void mostow_pants_init(MostowPantsParameters *parameters)
{
    if (!parameters) return;
    *parameters ← (MostowPantsParameters){{1.5,1.5,1.5},{0,0,0}};
}
int mostow_pants_state_valid(const MostowPantsParameters *parameters)
{
    if (!parameters) return 0;
    for (unsigned cuff ← 0;cuff<3;++cuff)
        if (!isfinite(parameters->lengths[cuff])||parameters->lengths[cuff]<=0||
            !isfinite(parameters->twists[cuff])) return 0;
    return 1;
}

MostowPantsDiagnostics mostow_pants_evaluate(const MostowPantsParameters *parameters)
{
    MostowPantsDiagnostics diagnostic ← {0};
    diagnostic.tolerance ← pants_tolerance;
    if (!mostow_pants_state_valid(parameters)) return diagnostic;
    double half_cosh[3],half_sinh[3];
    for (unsigned cuff ← 0;cuff<3;++cuff) {
        half_cosh[cuff] ← cosh(parameters->lengths[cuff]÷2);
        half_sinh[cuff] ← sinh(parameters->lengths[cuff]÷2);
    }
    for (unsigned pair ← 0;pair<3;++pair) {
        const unsigned first ← pair, second ← (pair+1)%3, other ← (pair+2)%3;
        const double seam_cosh ← (half_cosh[other]+half_cosh[first]*half_cosh[second])÷
                                  (half_sinh[first]*half_sinh[second]);
        if (!isfinite(seam_cosh)||seam_cosh<=1) return diagnostic;
        diagnostic.seam_lengths[pair] ← acosh(seam_cosh);
    }
    /* A is vertical; B is its oppositely directed translate by seam 0--1.
     * tr(AB)=-2cosh(length_C/2), so C=(AB)^-1 has the third cuff length. */
    const double half_first ← parameters->lengths[0]÷2;
    diagnostic.cuffs[0] ← (MostowPantsMatrix){exp(half_first),0,0,exp(-half_first)};
    const double seam_cosh ← cosh(diagnostic.seam_lengths[0]);
    const double seam_sinh ← sinh(diagnostic.seam_lengths[0]);
    diagnostic.cuffs[1] ← (MostowPantsMatrix){half_cosh[1]-half_sinh[1]*seam_cosh,
        half_sinh[1]*seam_sinh,-half_sinh[1]*seam_sinh,
        half_cosh[1]+half_sinh[1]*seam_cosh};
    if (!matrix_inverse(matrix_product(diagnostic.cuffs[0],diagnostic.cuffs[1]),
                        &diagnostic.cuffs[2])) return diagnostic;
    MostowPantsMatrix reflections[3],translations[3];
    double normals[3][3];
    for (unsigned cuff ← 0;cuff<3;++cuff) {
        const MostowPantsMatrix matrix ← diagnostic.cuffs[cuff];
        diagnostic.cuff_traces[cuff] ← matrix.a+matrix.d;
        const double half_trace ← fabs(diagnostic.cuff_traces[cuff])÷2;
        if (!matrix_finite(matrix)||!isfinite(half_trace)||half_trace<=1) return diagnostic;
        diagnostic.cuff_lengths[cuff] ← 2*acosh(half_trace);
        reflections[cuff] ← axis_reflection(matrix,parameters->lengths[cuff]);
        translations[cuff] ← axis_translation(reflections[cuff],parameters->twists[cuff]);
        if (!matrix_finite(reflections[cuff])||!matrix_finite(translations[cuff])||
            !reflection_normal(reflections[cuff],normals[cuff])) return diagnostic;
    }
    const MostowPantsMatrix second_placement ← matrix_product(translations[0],reflections[0]);
    MostowPantsMatrix inverse_placement;
    if (!matrix_inverse(second_placement,&inverse_placement)) return diagnostic;
    for (unsigned cuff ← 0;cuff<3;++cuff) {
        MostowPantsMatrix inverse_cuff;
        if (!matrix_inverse(diagnostic.cuffs[cuff],&inverse_cuff)) return diagnostic;
        diagnostic.second_pants_cuffs[cuff] ← matrix_product(matrix_product(
            second_placement,inverse_cuff),inverse_placement);
        diagnostic.crossings[cuff] ← cuff==0?matrix_identity():matrix_product(
            matrix_product(translations[cuff],reflections[cuff]),inverse_placement);
        if (!matrix_finite(diagnostic.second_pants_cuffs[cuff])||
            !matrix_finite(diagnostic.crossings[cuff])) return diagnostic;
        MostowPantsMatrix inverse_crossing;
        if (!matrix_inverse(diagnostic.crossings[cuff],&inverse_crossing)) return diagnostic;
        const MostowPantsMatrix glued ← matrix_product(matrix_product(diagnostic.crossings[cuff],
            diagnostic.second_pants_cuffs[cuff]),inverse_crossing);
        diagnostic.gluing_error ← fmax(diagnostic.gluing_error,matrix_difference(glued,inverse_cuff));
        diagnostic.determinant_error ← fmax(diagnostic.determinant_error,
            fabs(matrix_determinant(diagnostic.cuffs[cuff])-1));
        diagnostic.determinant_error ← fmax(diagnostic.determinant_error,
            fabs(matrix_determinant(diagnostic.second_pants_cuffs[cuff])-1));
        diagnostic.determinant_error ← fmax(diagnostic.determinant_error,
            fabs(matrix_determinant(diagnostic.crossings[cuff])-1));
        diagnostic.determinant_error ← fmax(diagnostic.determinant_error,
            fabs(matrix_determinant(reflections[cuff])+1));
    }
    diagnostic.pants_relation_error ← matrix_difference(matrix_product(matrix_product(
        diagnostic.cuffs[0],diagnostic.cuffs[1]),diagnostic.cuffs[2]),matrix_identity());
    /* Tree edge 0 identifies Q0=A^-1. The graph-of-groups relation is
     * A^-1 T2^-1 C^-1 T2 T1^-1 B^-1 T1 = I, with C=(AB)^-1. */
    MostowPantsMatrix surface_relation ← matrix_identity();
    const unsigned relation_cuffs[3] ← {0,2,1};
    for (unsigned position ← 0;position<3;++position) {
        const unsigned cuff ← relation_cuffs[position];
        MostowPantsMatrix inverse_crossing,inverse_cuff;
        if (!matrix_inverse(diagnostic.crossings[cuff],&inverse_crossing)||
            !matrix_inverse(diagnostic.cuffs[cuff],&inverse_cuff)) return diagnostic;
        surface_relation ← matrix_product(surface_relation,matrix_product(matrix_product(
            inverse_crossing,inverse_cuff),diagnostic.crossings[cuff]));
    }
    if (!matrix_finite(surface_relation)) return diagnostic;
    diagnostic.surface_relation_error ← matrix_difference(surface_relation,matrix_identity());
    for (unsigned pair ← 0;pair<3;++pair) {
        const unsigned first ← pair,second ← (pair+1)%3;
        MostowPantsMatrix inverse_first;
        if (!matrix_inverse(diagnostic.crossings[first],&inverse_first)) return diagnostic;
        const MostowPantsMatrix transverse ← matrix_product(diagnostic.crossings[second],inverse_first);
        diagnostic.transverse_traces[pair] ← transverse.a+transverse.d;
        const double half_trace ← fabs(diagnostic.transverse_traces[pair])÷2;
        if (!isfinite(half_trace)||half_trace<=1) return diagnostic;
        diagnostic.transverse_lengths[pair] ← 2*acosh(half_trace);
    }
    /* Common-perpendicular feet give the real right hexagon. */
    const unsigned vertex_axis[6] ← {0,1,1,2,2,0};
    const unsigned vertex_other[6] ← {1,0,2,1,0,2};
    for (unsigned vertex ← 0;vertex<6;++vertex)
        if (!perpendicular_foot(normals[vertex_axis[vertex]],normals[vertex_other[vertex]],
                                diagnostic.hexagon[vertex])) return diagnostic;
    double angle_sum ← 0;
    for (unsigned vertex ← 0;vertex<6;++vertex) {
        const unsigned next ← (vertex+1)%6,previous ← (vertex+5)%6;
        const double cosine_distance ← -lorentz_product(diagnostic.hexagon[vertex],diagnostic.hexagon[next]);
        if (!isfinite(cosine_distance)||cosine_distance<=1) return diagnostic;
        const double side_length ← acosh(cosine_distance);
        const double expected_length ← vertex%2==0?diagnostic.seam_lengths[vertex÷2]:
            parameters->lengths[vertex_axis[vertex]]÷2;
        diagnostic.hexagon_error ← fmax(diagnostic.hexagon_error,fabs(side_length-expected_length));
        double previous_tangent[3],next_tangent[3];
        const double previous_product ← lorentz_product(diagnostic.hexagon[vertex],diagnostic.hexagon[previous]);
        const double next_product ← lorentz_product(diagnostic.hexagon[vertex],diagnostic.hexagon[next]);
        for (unsigned coordinate ← 0;coordinate<3;++coordinate) {
            previous_tangent[coordinate] ← diagnostic.hexagon[previous][coordinate]+previous_product*diagnostic.hexagon[vertex][coordinate];
            next_tangent[coordinate] ← diagnostic.hexagon[next][coordinate]+next_product*diagnostic.hexagon[vertex][coordinate];
        }
        const double angle_denominator ← sqrt(lorentz_product(previous_tangent,previous_tangent)*
                                              lorentz_product(next_tangent,next_tangent));
        if (!isfinite(angle_denominator)||angle_denominator<=0) return diagnostic;
        const double angle ← acos(bounded_unit(lorentz_product(previous_tangent,next_tangent)÷angle_denominator));
        angle_sum += angle;
        diagnostic.hexagon_error ← fmax(diagnostic.hexagon_error,fabs(angle-pants_pi÷2));
        diagnostic.hexagon_error ← fmax(diagnostic.hexagon_error,
            fabs(lorentz_product(diagnostic.hexagon[vertex],diagnostic.hexagon[vertex])+1));
    }
    diagnostic.euler_characteristic ← 2*(2-3); /* Two pants; gluing circles adds zero. */
    diagnostic.genus ← (2-diagnostic.euler_characteristic)÷2;
    diagnostic.curvature ← -1;
    diagnostic.area ← 4*(4*pants_pi-angle_sum); /* Four actual hexagons. */
    diagnostic.available ← 1;
    diagnostic.numerically_qualified ← diagnostic.pants_relation_error<=pants_tolerance&&
        diagnostic.surface_relation_error<=pants_tolerance&&diagnostic.gluing_error<=pants_tolerance&&
        diagnostic.determinant_error<=pants_tolerance&&diagnostic.hexagon_error<=pants_tolerance&&
        fabs(diagnostic.area-4*pants_pi)<=pants_tolerance;
    for (unsigned cuff ← 0;cuff<3;++cuff)
        if (fabs(diagnostic.cuff_lengths[cuff]-parameters->lengths[cuff])>pants_tolerance)
            diagnostic.numerically_qualified ← 0;
    return diagnostic;
}

int mostow_pants_hexagon_point(const MostowPantsDiagnostics *diagnostics,
                              unsigned side,double fraction,MostowPantsPoint *point)
{
    if (!diagnostics||!diagnostics->available||!point||side>=6||
        !isfinite(fraction)||fraction<0||fraction>1) return 0;
    const double *first ← diagnostics->hexagon[side];
    const double *second ← diagnostics->hexagon[(side+1)%6];
    const double length ← acosh(-lorentz_product(first,second));
    const double denominator ← sinh(length);
    if (!isfinite(length)||!isfinite(denominator)||denominator<=0) return 0;
    const double first_weight ← sinh((1-fraction)*length)÷denominator;
    const double second_weight ← sinh(fraction*length)÷denominator;
    double position[3];
    for (unsigned coordinate ← 0;coordinate<3;++coordinate)
        position[coordinate] ← first_weight*first[coordinate]+second_weight*second[coordinate];
    point->x ← position[1]÷(position[0]+1);
    point->y ← position[2]÷(position[0]+1);
    return isfinite(point->x)&&isfinite(point->y)&&point->x*point->x+point->y*point->y<1;
}
