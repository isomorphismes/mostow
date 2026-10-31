#ifndef MOSTOW_DISTORTION_H
#define MOSTOW_DISTORTION_H
#include <stdint.h>

/* Coefficients refer to explicitly oriented coordinate bases. */
typedef struct { double xx, xy, yy; } MostowTangentMetric;
typedef struct { double xx, xy, yx, yy; } MostowDifferential2;
typedef struct { double x, y; } MostowTangentPoint;
typedef enum {
    MOSTOW_DISTORTION_OK,
    MOSTOW_DISTORTION_NONFINITE,
    MOSTOW_DISTORTION_METRIC_NOT_POSITIVE,
    MOSTOW_DISTORTION_DEGENERATE,
    MOSTOW_DISTORTION_CONDITION_UNSUPPORTED,
    MOSTOW_DISTORTION_NULL_OUTPUT
} MostowDistortionStatus;
typedef struct {
    MostowDistortionStatus status;
    double sigma_major, sigma_minor, dilatation;
    double area_scale, signed_area_scale;
    int orientation; /* +1 preserves, -1 reverses; zero on unavailable data. */
    int conformal_to_tolerance; /* Preserving orientation AND nearly equal axes. */
    int anticonformal_to_tolerance;
    double relative_roundoff_estimate; /* Diagnostic estimate, not a rigorous bound. */
} MostowDistortionDiagnostics;
typedef struct { double multiplicative, additive, coarse_surjectivity; } MostowCoarseConstants;
typedef struct { int available; int64_t integer; double distance; } MostowIntegerWitness;

MostowDistortionDiagnostics mostow_distortion_measure(
    MostowDifferential2 differential, MostowTangentMetric source_metric,
    MostowTangentMetric target_metric);
/* Points use each tangent plane's metric-orthonormal display frame. */
MostowDistortionStatus mostow_distortion_sample(
    MostowDifferential2 differential, MostowTangentMetric source_metric,
    MostowTangentMetric target_metric, double angle,
    MostowTangentPoint *source_circle, MostowTangentPoint *target_ellipse);
const char *mostow_distortion_status_name(MostowDistortionStatus status);
MostowCoarseConstants mostow_integer_inclusion_constants(void);
/* Computed witnesses cover |real| <= 2^50. This is not the global proof. */
MostowIntegerWitness mostow_integer_inclusion_witness(double real);
/* Exact binary64 integer distances for endpoints in [-2^50,2^50]. */
int mostow_integer_inclusion_distance(int64_t first, int64_t second, double *distance);
#endif
