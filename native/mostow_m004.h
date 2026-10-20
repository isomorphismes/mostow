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
    double logarithmic_size[4], angle_error[4];
    double tolerance, roundoff_estimate;
    int positive_dihedral_branches, rigorous_certificate;
} MostowM004Diagnostics;

/* Fixed two-tetrahedron census model. Face f is opposite vertex f.
 * permutation[t][f][v] is the image of vertex v in tetrahedron 1-t.
 * curve[c][t][v][f] is signed cusp-normal flow; c=0 meridian, c=1 longitude. */
typedef struct {
    unsigned permutation[2][4][4];
    int curve[2][2][4][4];
} MostowM004Triangulation;
typedef struct {
    int pairings_valid, orientation_consistent, edge_neighborhoods_valid;
    unsigned edge_classes, edge_degree[2], ideal_vertices;
    unsigned cusp_vertices, cusp_edges, cusp_triangles;
    int cusp_euler_characteristic, cusp_connected, cusp_is_torus;
    int edge_equations_match, peripheral_cycles_valid, peripheral_basis_valid;
    int peripheral_equations_match;
    int manifold_verified; /* exact combinatorial checks, not geometric certification */
} MostowM004Topology;
typedef struct {
    int available;
    double angle[3], volume;
    double series_tail_bound, roundoff_estimate;
    int rigorous_certificate; /* always false: system libm is not interval arithmetic */
} MostowTetrahedronVolume;

MostowM004Triangulation mostow_m004_triangulation(void);
MostowM004Topology mostow_m004_check_topology(const MostowM004Triangulation *triangulation);
MostowTetrahedronVolume mostow_ideal_tetrahedron_volume(MostowIdealShape shape);
/* Real-linear shear, then optionally (2w+1)/(w+1), on the Riemann sphere.
 * |shear|<1 preserves orientation; shear=0 is conformal. The cross-ratio is
 * independently recomputed from homogeneous transformed boundary points. */
int mostow_tetrahedron_boundary_example(MostowIdealShape shape,double shear,int mobius,
                                       double vertices[4][3],MostowIdealShape *cross_ratio);

void mostow_m004_init(MostowM004Experiment *experiment);
int mostow_m004_state_valid(const MostowM004Experiment *experiment);
void mostow_m004_complete_shapes(MostowIdealShape shapes[2]);
void mostow_m004_edge_only_shapes(MostowIdealShape shapes[2]);
MostowM004Diagnostics mostow_m004_constraints(const MostowIdealShape shapes[2]);
/* Normalized ideal boundary vertices: infinity, 0, 1, z, on the Klein sphere. */
int mostow_m004_ideal_vertices(MostowIdealShape shape, double vertices[4][3]);

#endif
