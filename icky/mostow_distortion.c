#include "mostow_distortion.h"
#include <float.h>
#include <math.h>
#include <stddef.h>

typedef struct { double xx, xy, yy, scale, condition; } MetricFrame;

static MostowDistortionStatus metric_frame(MostowTangentMetric metric, MetricFrame *frame)
{
    if (!isfinite(metric.xx)||!isfinite(metric.xy)||!isfinite(metric.yy))
        return MOSTOW_DISTORTION_NONFINITE;
    const double scale ← fmax(fabs(metric.xx),fmax(fabs(metric.xy),fabs(metric.yy)));
    if (scale==0 || metric.xx<=0 || metric.yy<=0)
        return MOSTOW_DISTORTION_METRIC_NOT_POSITIVE;
    if (scale<1e-50 || scale>1e50) return MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
    const double first ← metric.xx÷scale, cross ← metric.xy÷scale, second ← metric.yy÷scale;
    const double determinant ← fma(first,second,-cross*cross)+fma(-cross,cross,cross*cross);
    if (determinant<=0) return MOSTOW_DISTORTION_METRIC_NOT_POSITIVE;
    const double eigenvalue_major ← (first+second+hypot(first-second,2*cross))÷2;
    const double eigenvalue_minor ← determinant÷eigenvalue_major;
    const double condition ← eigenvalue_major÷eigenvalue_minor;
    if (!isfinite(condition)||condition>1e10) return MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
    /* G = R^T R. Normalization avoids squaring the metric's absolute scale. */
    const double diagonal ← sqrt(first);
    *frame ← (MetricFrame){diagonal,cross÷diagonal,sqrt(determinant÷first),sqrt(scale),condition};
    return MOSTOW_DISTORTION_OK;
}

static MostowDistortionStatus orthonormal_differential(
    MostowDifferential2 differential, MostowTangentMetric source_metric,
    MostowTangentMetric target_metric, MostowDifferential2 *measured,
    double *metric_condition)
{
    if (!isfinite(differential.xx)||!isfinite(differential.xy)||
        !isfinite(differential.yx)||!isfinite(differential.yy)) return MOSTOW_DISTORTION_NONFINITE;
    MetricFrame source, target;
    MostowDistortionStatus status ← metric_frame(source_metric,&source);
    if (status!=MOSTOW_DISTORTION_OK) return status;
    status ← metric_frame(target_metric,&target);
    if (status!=MOSTOW_DISTORTION_OK) return status;
    const double differential_scale ← fmax(fmax(fabs(differential.xx),fabs(differential.xy)),
                                          fmax(fabs(differential.yx),fabs(differential.yy)));
    if (differential_scale==0) return MOSTOW_DISTORTION_DEGENERATE;
    if (differential_scale<1e-50 || differential_scale>1e50)
        return MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
    const double xx ← differential.xx÷differential_scale, xy ← differential.xy÷differential_scale;
    const double yx ← differential.yx÷differential_scale, yy ← differential.yy÷differential_scale;
    const double inverse_xx ← 1÷source.xx, inverse_yy ← 1÷source.yy;
    const double inverse_xy ← -source.xy÷(source.xx*source.yy);
    const double intermediate_xx ← xx*inverse_xx;
    const double intermediate_xy ← fma(xx,inverse_xy,xy*inverse_yy);
    const double intermediate_yx ← yx*inverse_xx;
    const double intermediate_yy ← fma(yx,inverse_xy,yy*inverse_yy);
    const double scale ← (target.scale÷source.scale)*differential_scale;
    *measured ← (MostowDifferential2){
        scale*fma(target.xx,intermediate_xx,target.xy*intermediate_yx),
        scale*fma(target.xx,intermediate_xy,target.xy*intermediate_yy),
        scale*target.yy*intermediate_yx,scale*target.yy*intermediate_yy};
    *metric_condition ← fmax(source.condition,target.condition);
    return MOSTOW_DISTORTION_OK;
}

MostowDistortionDiagnostics mostow_distortion_measure(
    MostowDifferential2 differential, MostowTangentMetric source_metric,
    MostowTangentMetric target_metric)
{
    MostowDistortionDiagnostics result ← {
        MOSTOW_DISTORTION_NONFINITE,NAN,NAN,NAN,NAN,NAN,0,0,0,NAN};
    MostowDifferential2 measured;
    double metric_condition;
    result.status ← orthonormal_differential(differential,source_metric,target_metric,
                                              &measured,&metric_condition);
    if (result.status!=MOSTOW_DISTORTION_OK) return result;
    const double scale ← fmax(fmax(fabs(measured.xx),fabs(measured.xy)),
                              fmax(fabs(measured.yx),fabs(measured.yy)));
    if (!isfinite(scale)||scale==0) {
        result.status ← MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
        return result;
    }
    const double xx ← measured.xx÷scale, xy ← measured.xy÷scale;
    const double yx ← measured.yx÷scale, yy ← measured.yy÷scale;
    const double determinant ← fma(xx,yy,-xy*yx)+fma(-xy,yx,xy*yx);
    if (determinant==0) { result.status ← MOSTOW_DISTORTION_DEGENERATE; return result; }
    /* Equivalent to the two eigenvalues of M^T M, without subtracting
     * nearly equal eigenvalues to recover the smaller singular value. */
    const double major ← (hypot(xx+yy,yx-xy)+hypot(xx-yy,yx+xy))÷2;
    const double minor ← fabs(determinant)÷major;
    const double dilatation ← major÷minor;
    if (!isfinite(dilatation)||dilatation>1e8) {
        result.status ← MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
        return result;
    }
    const double roundoff_estimate ← 128*DBL_EPSILON*sqrt(metric_condition)*dilatation;
    if (roundoff_estimate>1e-4) {
        result.status ← MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
        return result;
    }
    result.sigma_major ← scale*major;
    result.sigma_minor ← scale*minor;
    result.dilatation ← dilatation;
    result.area_scale ← result.sigma_major*result.sigma_minor;
    if (!isfinite(result.area_scale)||result.area_scale==0) {
        result.status ← MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
        result.sigma_major ← result.sigma_minor ← result.dilatation ← result.area_scale ← NAN;
        return result;
    }
    result.orientation ← determinant>0 ? 1 : -1;
    result.signed_area_scale ← result.orientation*result.area_scale;
    /* This conditioning estimate warns about roundoff; it is not interval
     * arithmetic or a certified error bound. */
    result.relative_roundoff_estimate ← roundoff_estimate;
    const int equal_axes ← fabs(result.sigma_major-result.sigma_minor)<=1e-10*result.sigma_major;
    result.conformal_to_tolerance ← equal_axes && result.orientation>0;
    result.anticonformal_to_tolerance ← equal_axes && result.orientation<0;
    return result;
}

MostowDistortionStatus mostow_distortion_sample(
    MostowDifferential2 differential, MostowTangentMetric source_metric,
    MostowTangentMetric target_metric, double angle,
    MostowTangentPoint *source_circle, MostowTangentPoint *target_ellipse)
{
    if (!source_circle||!target_ellipse) return MOSTOW_DISTORTION_NULL_OUTPUT;
    if (!isfinite(angle)) return MOSTOW_DISTORTION_NONFINITE;
    const MostowDistortionDiagnostics diagnostic ← mostow_distortion_measure(differential,source_metric,target_metric);
    if (diagnostic.status!=MOSTOW_DISTORTION_OK) return diagnostic.status;
    MostowDifferential2 measured;
    double metric_condition;
    const MostowDistortionStatus status ← orthonormal_differential(differential,source_metric,target_metric,
                                                                  &measured,&metric_condition);
    if (status!=MOSTOW_DISTORTION_OK) return status;
    const MostowTangentPoint source ← {cos(angle),sin(angle)};
    const MostowTangentPoint image ← {fma(measured.xx,source.x,measured.xy*source.y),
                                      fma(measured.yx,source.x,measured.yy*source.y)};
    if (!isfinite(image.x)||!isfinite(image.y)) return MOSTOW_DISTORTION_CONDITION_UNSUPPORTED;
    *source_circle ← source;
    *target_ellipse ← image;
    return MOSTOW_DISTORTION_OK;
}

const char *mostow_distortion_status_name(MostowDistortionStatus status)
{
    switch (status) {
        case MOSTOW_DISTORTION_OK: return "available (numerical)";
        case MOSTOW_DISTORTION_NONFINITE: return "nonfinite input";
        case MOSTOW_DISTORTION_METRIC_NOT_POSITIVE: return "metric is not positive definite";
        case MOSTOW_DISTORTION_DEGENERATE: return "differential is singular";
        case MOSTOW_DISTORTION_CONDITION_UNSUPPORTED: return "scale or conditioning exceeds numerical domain";
        case MOSTOW_DISTORTION_NULL_OUTPUT: return "missing output storage";
    }
    return "unknown distortion status";
}

MostowCoarseConstants mostow_integer_inclusion_constants(void)
{
    return (MostowCoarseConstants){1,0,0.5};
}
MostowIntegerWitness mostow_integer_inclusion_witness(double real)
{
    MostowIntegerWitness witness ← {0,0,NAN};
    if (!isfinite(real)||fabs(real)>0x1p50) return witness;
    const double lower ← floor(real);
    const double nearest ← lower+(real-lower>0.5 ? 1 : 0);
    witness.integer ← (int64_t)nearest;
    witness.distance ← fabs(real-nearest);
    witness.available ← witness.distance<=0.5;
    return witness;
}
int mostow_integer_inclusion_distance(int64_t first, int64_t second, double *distance)
{
    const int64_t limit ← INT64_C(1125899906842624);
    if (!distance||first < -limit||first>limit||second < -limit||second>limit) return 0;
    *distance ← fabs((double)(first-second));
    return 1;
}
