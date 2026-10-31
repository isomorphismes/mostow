#ifndef MOSTOW_OCTAGON_H
#define MOSTOW_OCTAGON_H
#include <stdint.h>

#define MOSTOW_OCTAGON_SIDES 8u
#define MOSTOW_OCTAGON_NEIGHBORS 9u
#define MOSTOW_OCTAGON_WORD_CAPACITY 32u

/* Hyperboloid convention: <p,q> = p[0]q[0]-p[1]q[1]-p[2]q[2].
 * Points have <p,p>=1 and p[0]>0. Matrices act on column vectors.
 * Disk coordinates use curvature -1 and the open unit disk. */
typedef struct { double x,y; } MostowOctagonPoint;
typedef struct { double entry[3][3]; } MostowOctagonIsometry;
typedef struct {
    double vertex[8][3],midpoint[8][3],outward_normal[8][3];
    MostowOctagonIsometry pairing[8];
    uint32_t partner[8];
} MostowOctagon;
typedef struct {
    int available,valid;
    uint32_t vertex_class[8],edge_class[8];
    uint32_t vertices,edges,faces;
    int euler_characteristic;
    double interior_angle,corner_angle_sum,area,integrated_area;
    double area_error_estimate,side_length,pairing_error,lorentz_error,orientation_error,cycle_error;
} MostowOctagonDiagnostics;
typedef struct {
    MostowOctagonPoint position;
    MostowOctagonIsometry lift;
    uint32_t word_length,total_crossings;
    int32_t word[MOSTOW_OCTAGON_WORD_CAPACITY];
} MostowOctagonNavigator;

/* STAR M1 marking: a b a^-1 b^-1 c d c^-1 d^-1.
 * Pairing[i] maps side i to partner[i], reversing boundary traversal.
 * This is not the opposite-edge Bolza marking. */
void mostow_octagon_init(MostowOctagon *octagon);
MostowOctagonDiagnostics mostow_octagon_diagnostics(const MostowOctagon *octagon);
int mostow_octagon_disk_to_hyperboloid(MostowOctagonPoint point,double output[3]);
int mostow_octagon_hyperboloid_to_disk(const double point[3],MostowOctagonPoint *output);
int mostow_octagon_apply(MostowOctagonIsometry isometry,MostowOctagonPoint input,
                        MostowOctagonPoint *output);
/* Geodesic arclength interpolation along a cut side, 0<=fraction<=1. */
int mostow_octagon_side_point(const MostowOctagon *octagon,uint32_t side,
                             double fraction,MostowOctagonPoint *output);
int mostow_octagon_contains(const MostowOctagon *octagon,MostowOctagonPoint point);
/* Center tile plus its eight side-neighbors; matrices, not words, identify
 * these cells. Finite display only, not an enumeration of the cover. */
int mostow_octagon_neighbor(const MostowOctagon *octagon,uint32_t tile,
                           MostowOctagonIsometry *output);
void mostow_octagon_navigator_init(MostowOctagonNavigator *navigator);
int mostow_octagon_navigator_valid(const MostowOctagonNavigator *navigator);
/* Desired position is in the navigator's CURRENT tile coordinates. Transport
 * through identified sides until inside; lift records the continuous cover
 * position. At most 32 crossings per move. Adjacent inverse letters cancel;
 * this is NOT a canonical solution of the surface-group word problem.
 * Returns 1 on success. Invalid input, exhausted word capacity or numerical
 * range failure returns 0 and leaves the navigator unchanged. */
int mostow_octagon_move(const MostowOctagon *octagon,MostowOctagonNavigator *navigator,
                       MostowOctagonPoint desired_position);
#endif
