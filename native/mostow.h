#ifndef MOSTOW_H
#define MOSTOW_H
#include <stddef.h>
#include <stdint.h>

#define MOSTOW_MAX_VERTICES 4096u
#define MOSTOW_MAX_FACES 8192u
#define MOSTOW_MAX_EDGES 12288u
#define MOSTOW_MODE_COUNT 3u
#define MOSTOW_RADIUS 2.0
#define MOSTOW_SPACING 0.14
#define MOSTOW_PUBLIC_LENGTH_BOUND 1.10

typedef struct { double x, y, z; } MostowVector;
typedef struct { double radius, angle; } MostowIntrinsicPoint;
typedef struct { uint16_t vertex[3]; double inverse[3]; double area; } MostowFace;
typedef struct { uint16_t first, second, incidence; double length; } MostowEdge;
typedef struct { float position[3], normal[3], radius; } MostowRenderVertex;
typedef struct {
    uint32_t vertex_count, face_count, edge_count, boundary_count;
    MostowIntrinsicPoint intrinsic[MOSTOW_MAX_VERTICES];
    MostowFace faces[MOSTOW_MAX_FACES];
    MostowEdge edges[MOSTOW_MAX_EDGES];
    MostowVector base[MOSTOW_MAX_VERTICES];
    MostowVector modes[MOSTOW_MODE_COUNT][MOSTOW_MAX_VERTICES];
    double certified_minimum, certified_maximum, mode_derivative_bound;
    double arithmetic_derivative_bound;
} MostowSheet;
typedef struct {
    double minimum_stretch, maximum_stretch, length_factor, maximum_anisotropy;
    double area;
    uint32_t worst_face;
} MostowDiagnostics;
typedef enum {
    MOSTOW_OK, MOSTOW_BAD_INPUT, MOSTOW_CAPACITY, MOSTOW_DEGENERATE,
    MOSTOW_UNCERTIFIED
} MostowStatus;

MostowVector mostow_vector(double x, double y, double z);
MostowVector mostow_add(MostowVector a, MostowVector b);
MostowVector mostow_subtract(MostowVector a, MostowVector b);
MostowVector mostow_scale(MostowVector a, double scale);
double mostow_dot(MostowVector a, MostowVector b);
double mostow_norm(MostowVector a);
double mostow_hyperbolic_distance(MostowIntrinsicPoint a,
                                 MostowIntrinsicPoint b, double curvature_scale);
MostowStatus mostow_build_intrinsic(MostowSheet *sheet, double spacing);
MostowStatus mostow_set_reference(MostowSheet *sheet, double curvature_scale);
MostowStatus mostow_stretch(const MostowFace *face, const MostowVector *positions,
                           double *minimum, double *maximum);
MostowStatus mostow_diagnostics(const MostowSheet *sheet,
                               const MostowVector *positions,
                               MostowDiagnostics *diagnostics);
MostowStatus mostow_prepare_modes(MostowSheet *sheet);
MostowStatus mostow_sheet_init(MostowSheet *sheet);
MostowStatus mostow_sample(const MostowSheet *sheet, double active_seconds,
                          MostowRenderVertex *vertices, size_t vertex_capacity,
                          MostowDiagnostics *diagnostics);
#endif
