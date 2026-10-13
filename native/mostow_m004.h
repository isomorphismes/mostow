#ifndef MOSTOW_M004_H
#define MOSTOW_M004_H

typedef struct { double real, imaginary; } MostowIdealShape;
typedef struct {
    MostowIdealShape trial[2];
    int enforce_completeness;
    unsigned selected_coordinate;
} MostowM004Experiment;
typedef struct {
    int available;
    double residual[4]; /* edges, meridian, longitude; logarithmic norm */
    int complete_to_tolerance;
} MostowM004Diagnostics;

void mostow_m004_init(MostowM004Experiment *experiment);
int mostow_m004_state_valid(const MostowM004Experiment *experiment);
void mostow_m004_complete_shapes(MostowIdealShape shapes[2]);
void mostow_m004_edge_only_shapes(MostowIdealShape shapes[2]);
MostowM004Diagnostics mostow_m004_constraints(const MostowIdealShape shapes[2]);
/* Normalized ideal boundary vertices: infinity, 0, 1, z, on the Klein sphere. */
int mostow_m004_ideal_vertices(MostowIdealShape shape, double vertices[4][3]);

#endif
