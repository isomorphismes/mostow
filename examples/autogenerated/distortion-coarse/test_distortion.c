#include "mostow_distortion.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static const MostowTangentMetric euclidean ← {1,0,1};
static const char *case_name ← "UNSET";
static void close_number(double actual,double expected)
{
    if (!isfinite(actual)||fabs(actual-expected)>=2e-12*fmax(1,fabs(expected)))
        fprintf(stderr,"FAIL %s: actual %.17g expected %.17g\n",case_name,actual,expected);
    assert(isfinite(actual));
    assert(fabs(actual-expected)<2e-12*fmax(1,fabs(expected)));
}
static void axes(MostowDifferential2 differential,double major,double minor,int orientation)
{
    const MostowDistortionDiagnostics result ← mostow_distortion_measure(differential,euclidean,euclidean);
    assert(result.status==MOSTOW_DISTORTION_OK);
    close_number(result.sigma_major,major); close_number(result.sigma_minor,minor);
    close_number(result.dilatation,major÷minor);
    close_number(result.area_scale,major*minor);
    close_number(result.signed_area_scale,orientation*major*minor);
    assert(result.orientation==orientation);
    assert(result.relative_roundoff_estimate>0 && result.relative_roundoff_estimate<1e-4);
}
static void unavailable(MostowDifferential2 differential,MostowTangentMetric source,
                        MostowTangentMetric target,MostowDistortionStatus expected)
{
    const MostowDistortionDiagnostics result ← mostow_distortion_measure(differential,source,target);
    assert(result.status==expected);
    assert(isnan(result.sigma_major)&&isnan(result.sigma_minor)&&isnan(result.dilatation));
    assert(result.orientation==0&&!result.conformal_to_tolerance&&!result.anticonformal_to_tolerance);
}
int main(void)
{
    const MostowDifferential2 identity ← {1,0,0,1};
    case_name ← "IDENTITY";
    axes(identity,1,1,1);
    const MostowDifferential2 rational_rotation ← {0.6,-0.8,0.8,0.6};
    axes(rational_rotation,1,1,1);
    assert(mostow_distortion_measure(rational_rotation,euclidean,euclidean).conformal_to_tolerance);
    axes((MostowDifferential2){3,0,0,3},3,3,1);
    case_name ← "DIAGONAL_STRETCH";
    axes((MostowDifferential2){2,0,0,0.5},2,0.5,1);
    const double golden_ratio ← (1+sqrt(5.0))÷2;
    const MostowDifferential2 shear ← {1,1,0,1};
    axes(shear,golden_ratio,1÷golden_ratio,1);
    assert(!mostow_distortion_measure(shear,euclidean,euclidean).conformal_to_tolerance);
    const MostowDifferential2 reflection ← {1,0,0,-1};
    case_name ← "REFLECTION";
    axes(reflection,1,1,-1);
    const MostowDistortionDiagnostics reversed ← mostow_distortion_measure(reflection,euclidean,euclidean);
    assert(!reversed.conformal_to_tolerance&&reversed.anticonformal_to_tolerance);

    /* Independent coordinate identity: R^T R=G and J=R imply J is an
     * isometry from G to Euclidean space, including off-diagonal G. */
    const MostowTangentMetric skew_metric ← {4,2,2};
    case_name ← "METRIC_BASIS";
    const MostowDifferential2 upper_factor ← {2,1,0,1};
    const MostowDistortionDiagnostics changed_basis ← mostow_distortion_measure(upper_factor,skew_metric,euclidean);
    if (changed_basis.status!=MOSTOW_DISTORTION_OK||!changed_basis.conformal_to_tolerance)
        fprintf(stderr,"FAIL METRIC_BASIS: coordinate isometry was not conformal\n");
    assert(changed_basis.status==MOSTOW_DISTORTION_OK&&changed_basis.conformal_to_tolerance);
    close_number(changed_basis.sigma_major,1); close_number(changed_basis.sigma_minor,1);
    const MostowDistortionDiagnostics same_metric ← mostow_distortion_measure(identity,skew_metric,skew_metric);
    assert(same_metric.status==MOSTOW_DISTORTION_OK&&same_metric.conformal_to_tolerance);
    close_number(same_metric.sigma_major,1); close_number(same_metric.sigma_minor,1);
    const MostowDistortionDiagnostics doubled_metric ← mostow_distortion_measure(identity,skew_metric,
                                                                                 (MostowTangentMetric){8,4,4});
    assert(doubled_metric.status==MOSTOW_DISTORTION_OK);
    close_number(doubled_metric.sigma_major,sqrt(2.0));close_number(doubled_metric.sigma_minor,sqrt(2.0));
    close_number(doubled_metric.area_scale,2);

    /* Here orthonormal M=diag(5,7) J diag(1/2,1/3), calculated
     * independently from the coordinates, not by the kernel helper. */
    const MostowDifferential2 general ← {1,2,3,5};
    case_name ← "GENERAL_METRIC";
    const MostowTangentMetric source ← {4,0,9},target ← {25,0,49};
    const MostowDistortionDiagnostics measured ← mostow_distortion_measure(general,source,target);
    assert(measured.status==MOSTOW_DISTORTION_OK);
    close_number(measured.area_scale,35.0÷6.0); assert(measured.orientation==-1);
    const double square_norm ← 2.5*2.5+(10.0÷3)*(10.0÷3)+10.5*10.5+(35.0÷3)*(35.0÷3);
    close_number(measured.sigma_major*measured.sigma_major+measured.sigma_minor*measured.sigma_minor,square_norm);
    MostowTangentPoint circle,image;
    case_name ← "GENERAL_METRIC_IMAGE";
    assert(mostow_distortion_sample(general,source,target,0,&circle,&image)==MOSTOW_DISTORTION_OK);
    close_number(circle.x,1); close_number(circle.y,0); close_number(image.x,2.5);close_number(image.y,10.5);
    assert(mostow_distortion_sample(general,source,target,acos(-1.0)÷2,&circle,&image)==MOSTOW_DISTORTION_OK);
    close_number(image.x,10.0÷3);close_number(image.y,35.0÷3);
    for (unsigned sample ← 0;sample<257;++sample) {
        const double angle ← 2*acos(-1.0)*sample÷256;
        assert(mostow_distortion_sample(shear,euclidean,euclidean,angle,&circle,&image)==MOSTOW_DISTORTION_OK);
        close_number(hypot(circle.x,circle.y),1);
        close_number(image.x,circle.x+circle.y);close_number(image.y,circle.y);
        assert(hypot(image.x,image.y)>=1÷golden_ratio-1e-12);
        assert(hypot(image.x,image.y)<=golden_ratio+1e-12);
    }
    unavailable((MostowDifferential2){0,0,0,0},euclidean,euclidean,MOSTOW_DISTORTION_DEGENERATE);
    unavailable((MostowDifferential2){1,2,2,4},euclidean,euclidean,MOSTOW_DISTORTION_DEGENERATE);
    unavailable(identity,(MostowTangentMetric){1,1,1},euclidean,MOSTOW_DISTORTION_METRIC_NOT_POSITIVE);
    unavailable(identity,euclidean,(MostowTangentMetric){1,2,1},MOSTOW_DISTORTION_METRIC_NOT_POSITIVE);
    unavailable(identity,(MostowTangentMetric){-1,0,-1},euclidean,MOSTOW_DISTORTION_METRIC_NOT_POSITIVE);
    unavailable(identity,(MostowTangentMetric){1,0,1e-12},euclidean,MOSTOW_DISTORTION_CONDITION_UNSUPPORTED);
    unavailable((MostowDifferential2){1,0,0,1e-10},euclidean,euclidean,MOSTOW_DISTORTION_CONDITION_UNSUPPORTED);
    unavailable(identity,(MostowTangentMetric){1e60,0,1e60},euclidean,MOSTOW_DISTORTION_CONDITION_UNSUPPORTED);
    unavailable((MostowDifferential2){NAN,0,0,1},euclidean,euclidean,MOSTOW_DISTORTION_NONFINITE);
    unavailable(identity,euclidean,(MostowTangentMetric){1,INFINITY,1},MOSTOW_DISTORTION_NONFINITE);
    assert(mostow_distortion_sample(identity,euclidean,euclidean,0,NULL,&image)==MOSTOW_DISTORTION_NULL_OUTPUT);
    circle ← (MostowTangentPoint){17,23}; image ← circle;
    assert(mostow_distortion_sample(identity,euclidean,euclidean,NAN,&circle,&image)==MOSTOW_DISTORTION_NONFINITE);
    close_number(circle.x,17);close_number(image.y,23);

    const MostowCoarseConstants constants ← mostow_integer_inclusion_constants();
    case_name ← "COARSE_CONSTANTS";
    close_number(constants.multiplicative,1);close_number(constants.additive,0);close_number(constants.coarse_surjectivity,0.5);
    for (int first ← -30;first<=30;++first) {
        for (int second ← -30;second<=30;++second) {
            double distance;
            assert(mostow_integer_inclusion_distance(first,second,&distance));
            close_number(distance,fabs((double)first-second));
        }
        for (int part ← 0;part<=16;++part) {
            const double real ← first+part÷16.0;
            const MostowIntegerWitness witness ← mostow_integer_inclusion_witness(real);
            if (!witness.available||witness.distance>0.5)
                fprintf(stderr,"FAIL NEAREST_INTEGER: real %.17g integer %lld distance %.17g\n",
                        real,(long long)witness.integer,witness.distance);
            assert(witness.available&&witness.distance<=0.5);
            close_number(witness.distance,fabs(real-witness.integer));
            /* Half-integers force B >= 1/2, a negative control for density. */
            if (part==8) { close_number(witness.distance,0.5); assert(witness.distance>0.499); }
        }
    }
    const MostowIntegerWitness negative_tie ← mostow_integer_inclusion_witness(-0.5);
    assert(negative_tie.available&&negative_tie.integer==-1&&negative_tie.distance==0.5);
    assert(mostow_integer_inclusion_witness(0x1p50).available);
    assert(mostow_integer_inclusion_witness(-0x1p50).available);
    assert(!mostow_integer_inclusion_witness(0x1p51).available);
    assert(!mostow_integer_inclusion_witness(INFINITY).available);
    assert(!mostow_integer_inclusion_witness(NAN).available);
    double longest;
    assert(mostow_integer_inclusion_distance(INT64_C(-1125899906842624),INT64_C(1125899906842624),&longest));
    close_number(longest,0x1p51);
    assert(!mostow_integer_inclusion_distance(INT64_MAX,0,&longest));
    assert(!mostow_integer_inclusion_distance(0,0,NULL));
    puts("PASS metric distortion: exact spectra, metric frames, orientation, semantic ellipse, refusal controls; integer inclusion: sharp global constants and bounded witnesses");
    return 0;
}
