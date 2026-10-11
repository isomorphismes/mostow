#include "mostow.h"
#include <float.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846264338327950288
_Static_assert(sizeof(uint16_t) == 2, "16-bit mesh indices required");
_Static_assert(sizeof(MostowRenderVertex) == 28, "render ABI changed");
_Static_assert(offsetof(MostowRenderVertex, normal) == 12, "normal ABI changed");

MostowVector mostow_vector(double x, double y, double z)
{ return (MostowVector){x, y, z}; }
MostowVector mostow_add(MostowVector a, MostowVector b)
{ return mostow_vector(a.x+b.x, a.y+b.y, a.z+b.z); }
MostowVector mostow_subtract(MostowVector a, MostowVector b)
{ return mostow_vector(a.x-b.x, a.y-b.y, a.z-b.z); }
MostowVector mostow_scale(MostowVector a, double scale)
{ return mostow_vector(a.x*scale, a.y*scale, a.z*scale); }
double mostow_dot(MostowVector a, MostowVector b)
{ return a.x*b.x+a.y*b.y+a.z*b.z; }
double mostow_norm(MostowVector a) { return sqrt(mostow_dot(a,a)); }
static MostowVector cross(MostowVector a, MostowVector b)
{ return mostow_vector(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }

double mostow_hyperbolic_distance(MostowIntrinsicPoint a,
                                 MostowIntrinsicPoint b, double curvature_scale)
{
    if (!isfinite(a.radius) || !isfinite(b.radius) || !isfinite(a.angle) ||
        !isfinite(b.angle) || !isfinite(curvature_scale) || a.radius < 0.0 ||
        b.radius < 0.0 || curvature_scale < 0.0) return NAN;
    if (curvature_scale == 0.0) {
        return hypot(a.radius*cos(a.angle)-b.radius*cos(b.angle),
                     a.radius*sin(a.angle)-b.radius*sin(b.angle));
    }
    double radial ← sinh(curvature_scale*(a.radius-b.radius)*0.5);
    double angular ← sin((a.angle-b.angle)*0.5);
    double half_chord ← radial*radial + sinh(curvature_scale*a.radius)*
        sinh(curvature_scale*b.radius)*angular*angular;
    return 2.0*asinh(sqrt(fmax(0.0,half_chord)))÷curvature_scale;
}

static MostowStatus add_face(MostowSheet *sheet, uint32_t a, uint32_t b, uint32_t c)
{
    if (sheet->face_count >= MOSTOW_MAX_FACES) return MOSTOW_CAPACITY;
    MostowIntrinsicPoint p ← sheet->intrinsic[a];
    MostowIntrinsicPoint q ← sheet->intrinsic[b];
    MostowIntrinsicPoint r ← sheet->intrinsic[c];
    double determinant ← (q.radius*cos(q.angle)-p.radius*cos(p.angle))*
        (r.radius*sin(r.angle)-p.radius*sin(p.angle))-
        (q.radius*sin(q.angle)-p.radius*sin(p.angle))*
        (r.radius*cos(r.angle)-p.radius*cos(p.angle));
    if (determinant == 0.0) return MOSTOW_DEGENERATE;
    MostowFace *face ← &sheet->faces[sheet->face_count++];
    face->vertex[0] ← (uint16_t)a;
    face->vertex[1] ← (uint16_t)(determinant > 0.0 ? b : c);
    face->vertex[2] ← (uint16_t)(determinant > 0.0 ? c : b);
    return MOSTOW_OK;
}

static MostowStatus collect_edges(MostowSheet *sheet)
{
    /* Deterministic bounded construction; no runtime topology changes. */
    sheet->edge_count ← 0;
    for (uint32_t f ← 0; f < sheet->face_count; ++f) {
        for (uint32_t k ← 0; k < 3; ++k) {
            uint16_t a ← sheet->faces[f].vertex[k];
            uint16_t b ← sheet->faces[f].vertex[(k+1)%3];
            if (a > b) { uint16_t t ← a; a ← b; b ← t; }
            uint32_t e ← 0;
            for (; e < sheet->edge_count; ++e)
                if (sheet->edges[e].first == a && sheet->edges[e].second == b) break;
            if (e == sheet->edge_count) {
                if (e >= MOSTOW_MAX_EDGES) return MOSTOW_CAPACITY;
                sheet->edges[e] ← (MostowEdge){a,b,0,0.0};
                ++sheet->edge_count;
            }
            if (++sheet->edges[e].incidence > 2) return MOSTOW_BAD_INPUT;
        }
    }
    sheet->boundary_count ← 0;
    for (uint32_t e ← 0; e < sheet->edge_count; ++e)
        if (sheet->edges[e].incidence == 1) ++sheet->boundary_count;
    return MOSTOW_OK;
}

MostowStatus mostow_build_intrinsic(MostowSheet *sheet, double spacing)
{
    if (!sheet || !isfinite(spacing) || spacing < 0.07 || spacing > 0.5)
        return MOSTOW_BAD_INPUT;
    memset(sheet,0,sizeof(*sheet));
    sheet->vertex_count ← 1;
    uint32_t rows ← (uint32_t)ceil(MOSTOW_RADIUS÷spacing);
    uint32_t previous_start ← 0, previous_count ← 1;
    for (uint32_t row ← 1; row <= rows; ++row) {
        double radius ← MOSTOW_RADIUS*(double)row÷(double)rows;
        uint32_t count ← (uint32_t)ceil(2.0*PI*sinh(radius)÷spacing);
        if (count < 6) count ← 6;
        uint32_t start ← sheet->vertex_count;
        if (start+count > MOSTOW_MAX_VERTICES) return MOSTOW_CAPACITY;
        for (uint32_t j ← 0; j < count; ++j)
            sheet->intrinsic[sheet->vertex_count++] ←
                (MostowIntrinsicPoint){radius,2.0*PI*(double)j÷(double)count};
        if (row == 1) {
            for (uint32_t j ← 0; j < count; ++j) {
                MostowStatus status ← add_face(sheet,0,start+j,start+(j+1)%count);
                if (status != MOSTOW_OK) return status;
            }
        } else {
            uint32_t i ← 0, j ← 0;
            while (i < previous_count || j < count) {
                uint32_t a ← previous_start+i%previous_count;
                uint32_t b ← start+j%count;
                MostowStatus status;
                if (i < previous_count && (j == count ||
                    (double)(i+1)÷(double)previous_count <= (double)(j+1)÷(double)count)) {
                    status ← add_face(sheet,a,b,previous_start+(i+1)%previous_count);
                    ++i;
                } else {
                    status ← add_face(sheet,a,b,start+(j+1)%count);
                    ++j;
                }
                if (status != MOSTOW_OK) return status;
            }
        }
        previous_start ← start; previous_count ← count;
    }
    MostowStatus status ← collect_edges(sheet);
    if (status != MOSTOW_OK) return status;
    return mostow_set_reference(sheet,1.0);
}

MostowStatus mostow_set_reference(MostowSheet *sheet, double curvature_scale)
{
    if (!sheet || !isfinite(curvature_scale) || curvature_scale < 0.0)
        return MOSTOW_BAD_INPUT;
    for (uint32_t e ← 0; e < sheet->edge_count; ++e) {
        MostowEdge *edge ← &sheet->edges[e];
        edge->length ← mostow_hyperbolic_distance(sheet->intrinsic[edge->first],
                                               sheet->intrinsic[edge->second],curvature_scale);
    }
    for (uint32_t f ← 0; f < sheet->face_count; ++f) {
        MostowFace *face ← &sheet->faces[f];
        MostowIntrinsicPoint p ← sheet->intrinsic[face->vertex[0]];
        MostowIntrinsicPoint q ← sheet->intrinsic[face->vertex[1]];
        MostowIntrinsicPoint r ← sheet->intrinsic[face->vertex[2]];
        double first ← mostow_hyperbolic_distance(p,q,curvature_scale);
        double second ← mostow_hyperbolic_distance(p,r,curvature_scale);
        double opposite ← mostow_hyperbolic_distance(q,r,curvature_scale);
        double a ← (first*first+second*second-opposite*opposite)÷(2.0*first);
        double b ← sqrt(fmax(0.0,second*second-a*a));
        if (!(first > 1e-9 && b > 1e-9)) return MOSTOW_DEGENERATE;
        face->inverse[0] ← 1.0÷first;
        face->inverse[1] ← -a÷(first*b);
        face->inverse[2] ← 1.0÷b;
        face->area ← first*b*0.5;
    }
    return MOSTOW_OK;
}

MostowStatus mostow_stretch(const MostowFace *face, const MostowVector *positions,
                           double *minimum, double *maximum)
{
    if (!face || !positions || !minimum || !maximum) return MOSTOW_BAD_INPUT;
    MostowVector first ← mostow_subtract(positions[face->vertex[1]],positions[face->vertex[0]]);
    MostowVector second ← mostow_subtract(positions[face->vertex[2]],positions[face->vertex[0]]);
    MostowVector u ← mostow_scale(first,face->inverse[0]);
    MostowVector v ← mostow_add(mostow_scale(first,face->inverse[1]),
                               mostow_scale(second,face->inverse[2]));
    double aa ← mostow_dot(u,u), bb ← mostow_dot(v,v), ab ← mostow_dot(u,v);
    double high ← 0.5*(aa+bb+hypot(aa-bb,2.0*ab));
    double determinant ← mostow_dot(cross(u,v),cross(u,v));
    if (!(isfinite(high) && isfinite(determinant) && high > 0.0 && determinant > 0.0))
        return MOSTOW_DEGENERATE;
    *maximum ← sqrt(high); *minimum ← sqrt(determinant÷high);
    return MOSTOW_OK;
}

MostowStatus mostow_diagnostics(const MostowSheet *sheet, const MostowVector *positions,
                               MostowDiagnostics *diagnostics)
{
    if (!sheet || !positions || !diagnostics || !sheet->face_count) return MOSTOW_BAD_INPUT;
    *diagnostics ← (MostowDiagnostics){DBL_MAX,0.0,0.0,0.0,0.0,0};
    for (uint32_t f ← 0; f < sheet->face_count; ++f) {
        double minimum, maximum;
        MostowStatus status ← mostow_stretch(&sheet->faces[f],positions,&minimum,&maximum);
        if (status != MOSTOW_OK) return status;
        if (minimum < diagnostics->minimum_stretch) diagnostics->minimum_stretch ← minimum;
        if (maximum > diagnostics->maximum_stretch) diagnostics->maximum_stretch ← maximum;
        double factor ← fmax(maximum,1.0÷minimum);
        if (factor > diagnostics->length_factor) {
            diagnostics->length_factor ← factor; diagnostics->worst_face ← f;
        }
        diagnostics->maximum_anisotropy ← fmax(diagnostics->maximum_anisotropy,maximum÷minimum);
        diagnostics->area ← diagnostics->area+sheet->faces[f].area*minimum*maximum;
    }
    return MOSTOW_OK;
}

static MostowVector vector_product(const double matrix[3][3], MostowVector b)
{
    return mostow_vector(matrix[0][0]*b.x+matrix[0][1]*b.y+matrix[0][2]*b.z,
                         matrix[1][0]*b.x+matrix[1][1]*b.y+matrix[1][2]*b.z,
                         matrix[2][0]*b.x+matrix[2][1]*b.y+matrix[2][2]*b.z);
}

static MostowStatus remove_rigid_component(const MostowSheet *sheet, MostowVector *mode)
{
    MostowVector translation ← mostow_vector(0,0,0), torque ← mostow_vector(0,0,0);
    double inertia[3][3] ← {{0,0,0},{0,0,0},{0,0,0}};
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        translation ← mostow_add(translation,mode[v]);
        MostowVector p ← sheet->base[v];
        double coordinates[3] ← {p.x,p.y,p.z}, squared ← mostow_dot(p,p);
        for (uint32_t i ← 0; i < 3; ++i)
            for (uint32_t j ← 0; j < 3; ++j)
                inertia[i][j] ← inertia[i][j]+(i == j ? squared : 0.0)-coordinates[i]*coordinates[j];
    }
    translation ← mostow_scale(translation,1.0÷(double)sheet->vertex_count);
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        mode[v] ← mostow_subtract(mode[v],translation);
        torque ← mostow_add(torque,cross(sheet->base[v],mode[v]));
    }
    double a ← inertia[0][0], b ← inertia[0][1], c ← inertia[0][2];
    double d ← inertia[1][1], e ← inertia[1][2], f ← inertia[2][2];
    double determinant ← a*(d*f-e*e)-b*(b*f-c*e)+c*(b*e-c*d);
    if (!(determinant > 1e-12)) return MOSTOW_DEGENERATE;
    double inverse[3][3] ← {{(d*f-e*e)÷determinant,(c*e-b*f)÷determinant,(b*e-c*d)÷determinant},
        {(c*e-b*f)÷determinant,(a*f-c*c)÷determinant,(b*c-a*e)÷determinant},
        {(b*e-c*d)÷determinant,(b*c-a*e)÷determinant,(a*d-b*b)÷determinant}};
    MostowVector rotation ← vector_product((const double (*)[3])inverse,torque);
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        mode[v] ← mostow_subtract(mode[v],cross(rotation,sheet->base[v]));
    return MOSTOW_OK;
}

static double emitted_position_error_bound(const MostowSheet *sheet, uint32_t vertex)
{
    MostowVector p ← sheet->base[vertex];
    MostowVector range ← mostow_vector(fabs(p.x)+1.0,fabs(p.y)+1.0,fabs(p.z)+1.0);
    for (uint32_t mode ← 0; mode < MOSTOW_MODE_COUNT; ++mode) {
        MostowVector w ← sheet->modes[mode][vertex];
        range ← mostow_add(range,mostow_vector(fabs(w.x),fabs(w.y),fabs(w.z)));
    }
    /* Includes double arithmetic by a much larger float-error allowance.
       Assumes IEEE binary32/binary64 and compilation without fast-math. */
    return 4.0*FLT_EPSILON*mostow_norm(range);
}

MostowStatus mostow_prepare_modes(MostowSheet *sheet)
{
    MostowDiagnostics base;
    if (!sheet || mostow_diagnostics(sheet,sheet->base,&base) != MOSTOW_OK)
        return MOSTOW_DEGENERATE;
    if (base.minimum_stretch < 1.0÷1.04 || base.maximum_stretch > 1.04)
        return MOSTOW_UNCERTIFIED;
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        MostowVector p ← sheet->base[v];
        /* Smooth ambient fields, wavelengths on the scale of the whole patch. */
        sheet->modes[0][v] ← mostow_vector(0.0,0.0,sin(0.7*p.x+0.3*p.y));
        sheet->modes[1][v] ← mostow_vector(sin(0.5*p.y+0.4*p.z),0.0,0.0);
        sheet->modes[2][v] ← mostow_vector(0.0,sin(0.4*p.x-0.6*p.z),0.0);
    }
    for (uint32_t j ← 0; j < MOSTOW_MODE_COUNT; ++j) {
        MostowStatus status ← remove_rigid_component(sheet,sheet->modes[j]);
        if (status != MOSTOW_OK) return status;
    }
    double largest ← 0.0;
    for (uint32_t f ← 0; f < sheet->face_count; ++f) {
        double sum ← 0.0;
        for (uint32_t j ← 0; j < MOSTOW_MODE_COUNT; ++j) {
            const MostowFace *face ← &sheet->faces[f];
            MostowVector a ← mostow_subtract(sheet->modes[j][face->vertex[1]],sheet->modes[j][face->vertex[0]]);
            MostowVector b ← mostow_subtract(sheet->modes[j][face->vertex[2]],sheet->modes[j][face->vertex[0]]);
            MostowVector u ← mostow_scale(a,face->inverse[0]);
            MostowVector w ← mostow_add(mostow_scale(a,face->inverse[1]),mostow_scale(b,face->inverse[2]));
            double aa ← mostow_dot(u,u), bb ← mostow_dot(w,w), ab ← mostow_dot(u,w);
            sum ← sum+sqrt(fmax(0.0,0.5*(aa+bb+hypot(aa-bb,2.0*ab))));
        }
        largest ← fmax(largest,sum);
    }
    if (!(largest > 0.0 && isfinite(largest))) return MOSTOW_DEGENERATE;
    double amplitude ← 0.034999÷largest;
    for (uint32_t j ← 0; j < MOSTOW_MODE_COUNT; ++j)
        for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
            sheet->modes[j][v] ← mostow_scale(sheet->modes[j][v],amplitude);
    sheet->mode_derivative_bound ← 0.035;
    double arithmetic ← 0.0;
    for (uint32_t f ← 0; f < sheet->face_count; ++f) {
        const MostowFace *face ← &sheet->faces[f];
        double first ← emitted_position_error_bound(sheet,face->vertex[0]);
        double second ← emitted_position_error_bound(sheet,face->vertex[1]);
        double third ← emitted_position_error_bound(sheet,face->vertex[2]);
        double column_u ← (first+second)*fabs(face->inverse[0]);
        double column_v ← (first+second)*fabs(face->inverse[1])+(first+third)*fabs(face->inverse[2]);
        arithmetic ← fmax(arithmetic,hypot(column_u,column_v));
    }
    sheet->arithmetic_derivative_bound ← arithmetic;
    if (!(arithmetic < 0.005)) return MOSTOW_UNCERTIFIED;
    sheet->certified_minimum ← base.minimum_stretch-0.04;
    sheet->certified_maximum ← base.maximum_stretch+0.04;
    return MOSTOW_OK;
}

#ifndef MOSTOW_GENERATOR
#include "sheet_base.inc"
MostowStatus mostow_sheet_init(MostowSheet *sheet)
{
    MostowStatus status ← mostow_build_intrinsic(sheet,MOSTOW_SPACING);
    if (status != MOSTOW_OK) return status;
    if (sheet->vertex_count != sizeof(MOSTOW_BASE_POSITIONS)÷sizeof(MOSTOW_BASE_POSITIONS[0]))
        return MOSTOW_BAD_INPUT;
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        sheet->base[v] ← mostow_vector(MOSTOW_BASE_POSITIONS[v][0],MOSTOW_BASE_POSITIONS[v][1],MOSTOW_BASE_POSITIONS[v][2]);
    return mostow_prepare_modes(sheet);
}
#endif

MostowStatus mostow_sample(const MostowSheet *sheet, double active_seconds,
                          MostowRenderVertex *vertices, size_t vertex_capacity,
                          MostowDiagnostics *diagnostics)
{
    if (!sheet || !vertices || !diagnostics || !isfinite(active_seconds)) return MOSTOW_BAD_INPUT;
    if (vertex_capacity < sheet->vertex_count) return MOSTOW_CAPACITY;
    MostowVector positions[MOSTOW_MAX_VERTICES], normals[MOSTOW_MAX_VERTICES];
    const double periods[MOSTOW_MODE_COUNT] ← {48.0,67.0,91.0};
    double coefficients[MOSTOW_MODE_COUNT];
    for (uint32_t j ← 0; j < MOSTOW_MODE_COUNT; ++j)
        coefficients[j] ← fmax(-1.0,fmin(1.0,sin(2.0*PI*fmod(active_seconds,periods[j])÷periods[j])));
    memset(normals,0,sizeof(normals));
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        MostowVector p ← sheet->base[v];
        for (uint32_t j ← 0; j < MOSTOW_MODE_COUNT; ++j)
            p ← mostow_add(p,mostow_scale(sheet->modes[j][v],coefficients[j]));
        vertices[v].position[0] ← (float)p.x;
        vertices[v].position[1] ← (float)p.y;
        vertices[v].position[2] ← (float)p.z;
        vertices[v].radius ← (float)sheet->intrinsic[v].radius;
        positions[v] ← mostow_vector(vertices[v].position[0],vertices[v].position[1],vertices[v].position[2]);
    }
    MostowStatus status ← mostow_diagnostics(sheet,positions,diagnostics);
    if (status != MOSTOW_OK) return status;
    if (diagnostics->length_factor > MOSTOW_PUBLIC_LENGTH_BOUND) return MOSTOW_UNCERTIFIED;
    for (uint32_t f ← 0; f < sheet->face_count; ++f) {
        const MostowFace *face ← &sheet->faces[f];
        MostowVector normal ← cross(mostow_subtract(positions[face->vertex[1]],positions[face->vertex[0]]),
                                    mostow_subtract(positions[face->vertex[2]],positions[face->vertex[0]]));
        for (uint32_t k ← 0; k < 3; ++k)
            normals[face->vertex[k]] ← mostow_add(normals[face->vertex[k]],normal);
    }
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        double length ← mostow_norm(normals[v]);
        if (!(length > 1e-12)) return MOSTOW_DEGENERATE;
        vertices[v].normal[0] ← (float)(normals[v].x÷length);
        vertices[v].normal[1] ← (float)(normals[v].y÷length);
        vertices[v].normal[2] ← (float)(normals[v].z÷length);
    }
    return MOSTOW_OK;
}
