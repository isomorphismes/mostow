#ifndef MOSTOW_PANTS_H
#define MOSTOW_PANTS_H

/* A fixed marking: two pants joined along cuffs 0,1,2. Twists are signed
 * hyperbolic arclengths, not angles and not reduced modulo a cuff length. */
typedef struct { double lengths[3], twists[3]; } MostowPantsParameters;
typedef struct { double a,b,c,d; } MostowPantsMatrix;
typedef struct { double x,y; } MostowPantsPoint;
typedef struct {
    int available, numerically_qualified;
    int genus, euler_characteristic;
    double curvature, area;
    double cuff_lengths[3], cuff_traces[3];
    /* Pair order for seams and transverse loops: 0--1, 1--2, 2--0. */
    double seam_lengths[3], transverse_lengths[3], transverse_traces[3];
    /* One right hexagon, vertices on -t²+x²+y²=-1, t>0, in boundary order.
     * Alternating edges are seams and half cuffs. Two hexagons make a pant. */
    double hexagon[6][3];
    MostowPantsMatrix cuffs[3], second_pants_cuffs[3], crossings[3];
    double pants_relation_error, surface_relation_error, gluing_error;
    double determinant_error, hexagon_error, tolerance;
} MostowPantsDiagnostics;

void mostow_pants_init(MostowPantsParameters *parameters);
int mostow_pants_state_valid(const MostowPantsParameters *parameters);
MostowPantsDiagnostics mostow_pants_evaluate(const MostowPantsParameters *parameters);
/* Geodesic arclength interpolation of one hexagon side, projected to the disk. */
int mostow_pants_hexagon_point(const MostowPantsDiagnostics *diagnostics,
                              unsigned side,double fraction,MostowPantsPoint *point);

#endif
