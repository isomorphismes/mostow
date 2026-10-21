#include "mostow_pants.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static MostowPantsMatrix product(MostowPantsMatrix left,MostowPantsMatrix right)
{
    return (MostowPantsMatrix){left.a*right.a+left.b*right.c,
        left.a*right.b+left.b*right.d,left.c*right.a+left.d*right.c,
        left.c*right.b+left.d*right.d};
}
static double matrix_error(MostowPantsMatrix left,MostowPantsMatrix right)
{
    return fmax(fmax(fabs(left.a-right.a),fabs(left.b-right.b)),
                fmax(fabs(left.c-right.c),fabs(left.d-right.d)));
}
static double disk_distance(MostowPantsPoint first,MostowPantsPoint second)
{
    const double difference_x ← first.x-second.x,difference_y ← first.y-second.y;
    return acosh(1+2*(difference_x*difference_x+difference_y*difference_y)÷
        ((1-first.x*first.x-first.y*first.y)*(1-second.x*second.x-second.y*second.y)));
}
static void assert_qualified(MostowPantsDiagnostics diagnostic)
{
    assert(diagnostic.available);
    assert(diagnostic.genus==2&&diagnostic.euler_characteristic==-2);
    assert(diagnostic.curvature==-1);
    assert(fabs(diagnostic.area-4*acos(-1.0))<1e-8);
    /* Independently evaluate the presented group identities, rather than
     * trusting the constructor's qualification flag or residual fields. */
    const MostowPantsMatrix identity ← {1,0,0,1};
    assert(matrix_error(product(product(diagnostic.cuffs[0],diagnostic.cuffs[1]),
                                diagnostic.cuffs[2]),identity)<1e-8);
    assert(matrix_error(product(product(diagnostic.second_pants_cuffs[0],
        diagnostic.second_pants_cuffs[2]),diagnostic.second_pants_cuffs[1]),identity)<1e-8);
    for (unsigned cuff ← 0;cuff<3;++cuff) {
        const MostowPantsMatrix crossing ← diagnostic.crossings[cuff];
        assert(fabs(crossing.a*crossing.d-crossing.b*crossing.c-1)<1e-8);
        const MostowPantsMatrix inverse ← {crossing.d,-crossing.b,-crossing.c,crossing.a};
        const MostowPantsMatrix matched ← product(product(crossing,
            diagnostic.second_pants_cuffs[cuff]),inverse);
        assert(matrix_error(product(matched,diagnostic.cuffs[cuff]),identity)<1e-8);
    }
    assert(diagnostic.numerically_qualified);
}
int main(void)
{
    MostowPantsParameters parameters;
    mostow_pants_init(&parameters);
    assert(mostow_pants_state_valid(&parameters));
    assert_qualified(mostow_pants_evaluate(&parameters));
    /* An independent algebraic fixture: cosh(half-cuff)=2, sinh=√3.
     * The right-hexagon equation gives cosh(seam)=(2+4)/3=2,
     * hence every side of this hexagon is arcosh(2). */
    const double half_cuff ← 1.31695789692481670862504634730797;
    for (unsigned cuff ← 0;cuff<3;++cuff) parameters.lengths[cuff] ← 2*half_cuff;
    MostowPantsDiagnostics symmetric ← mostow_pants_evaluate(&parameters);
    assert_qualified(symmetric);
    for (unsigned cuff ← 0;cuff<3;++cuff) {
        assert(fabs(fabs(symmetric.cuff_traces[cuff])-4)<1e-11);
        assert(fabs(symmetric.cuff_lengths[cuff]-2*half_cuff)<1e-11);
        assert(fabs(symmetric.seam_lengths[cuff]-half_cuff)<1e-11);
        assert(fabs(symmetric.transverse_lengths[cuff]-2*half_cuff)<1e-10);
        assert(fabs(fabs(symmetric.transverse_traces[cuff])-4)<1e-10);
    }
    /* Independently check the exact first two cuff matrices. */
    assert(fabs(symmetric.cuffs[0].a-(2+sqrt(3.0)))<1e-12);
    assert(fabs(symmetric.cuffs[0].d-(2-sqrt(3.0)))<1e-12);
    assert(fabs(symmetric.cuffs[1].a-(2-2*sqrt(3.0)))<1e-11);
    assert(fabs(symmetric.cuffs[1].b-3)<1e-11);
    assert(fabs(symmetric.cuffs[1].c+3)<1e-11);
    assert(fabs(symmetric.cuffs[1].d-(2+2*sqrt(3.0)))<1e-11);
    for (unsigned side ← 0;side<6;++side) {
        MostowPantsPoint first,middle,last;
        assert(mostow_pants_hexagon_point(&symmetric,side,0,&first));
        assert(mostow_pants_hexagon_point(&symmetric,side,0.25,&middle));
        assert(mostow_pants_hexagon_point(&symmetric,side,1,&last));
        assert(fabs(disk_distance(first,last)-half_cuff)<1e-11);
        assert(fabs(disk_distance(first,middle)-half_cuff÷4)<1e-11);
    }
    /* A changed marked cuff certifies a changed marked metric. */
    parameters.lengths[0] ← 2*acosh(3.0);
    MostowPantsDiagnostics longer ← mostow_pants_evaluate(&parameters);
    assert_qualified(longer);
    assert(fabs(longer.cuff_traces[0]-6)<1e-11);
    assert(fabs(longer.cuff_lengths[0]-symmetric.cuff_lengths[0])>0.5);
    assert(fabs(longer.transverse_lengths[0]-symmetric.transverse_lengths[0])>0.01);
    parameters.lengths[0] ← 2*half_cuff;
    /* One full twist changes the marking. Independent trace identity:
     * |tr(T01)|/2=cosh(seam01)cosh(twist1/2)=2×2=4. */
    parameters.twists[1] ← parameters.lengths[1];
    MostowPantsDiagnostics twisted ← mostow_pants_evaluate(&parameters);
    assert_qualified(twisted);
    assert(fabs(fabs(twisted.transverse_traces[0])-8)<1e-10);
    assert(fabs(twisted.transverse_lengths[0]-2*acosh(4.0))<1e-10);
    assert(fabs(twisted.transverse_lengths[0]-symmetric.transverse_lengths[0])>1);
    assert(matrix_error(twisted.crossings[1],product(symmetric.cuffs[1],symmetric.crossings[1]))<1e-10);
    for (unsigned cuff ← 0;cuff<3;++cuff)
        assert(fabs(twisted.cuff_lengths[cuff]-symmetric.cuff_lengths[cuff])<1e-10);
    /* All three twist parameters influence actual marked closed curves. */
    for (unsigned changed ← 0;changed<3;++changed) {
        for (unsigned cuff ← 0;cuff<3;++cuff) parameters.twists[cuff] ← 0;
        parameters.twists[changed] ← 1;
        MostowPantsDiagnostics moved ← mostow_pants_evaluate(&parameters);
        assert_qualified(moved);
        unsigned changed_curves ← 0;
        for (unsigned curve ← 0;curve<3;++curve)
            if (fabs(moved.transverse_lengths[curve]-symmetric.transverse_lengths[curve])>0.1) ++changed_curves;
        assert(changed_curves==2);
    }
    /* Independent scalar trace identity versus the holonomy matrices. */
    parameters.twists[0] ← 0.7;parameters.twists[1] ← -1.1;parameters.twists[2] ← 0.4;
    MostowPantsDiagnostics mixed ← mostow_pants_evaluate(&parameters);
    assert_qualified(mixed);
    for (unsigned pair ← 0;pair<3;++pair) {
        const unsigned second ← (pair+1)%3;
        const double expected_half_trace ← cosh(mixed.seam_lengths[pair])*
            cosh(parameters.twists[pair]÷2)*cosh(parameters.twists[second]÷2)+
            sinh(parameters.twists[pair]÷2)*sinh(parameters.twists[second]÷2);
        assert(fabs(fabs(mixed.transverse_traces[pair])÷2-expected_half_trace)<1e-10);
    }
    double maximum_error ← 0;
    /* Deterministic UI-window extremes, not a proof over the continuum. */
    for (unsigned index ← 0;index<729;++index) {
        unsigned digits ← index;
        for (unsigned cuff ← 0;cuff<3;++cuff) {
            const double lengths[3] ← {0.75,1.5,3};
            parameters.lengths[cuff] ← lengths[digits%3];digits /= 3;
        }
        for (unsigned cuff ← 0;cuff<3;++cuff) {
            parameters.twists[cuff] ← 2*((double)(digits%3)-1);digits /= 3;
        }
        MostowPantsDiagnostics grid ← mostow_pants_evaluate(&parameters);
        if (!grid.available||!grid.numerically_qualified)
            fprintf(stderr,"FN grid %u: available=%d qualified=%d closure=%.17g glue=%.17g det=%.17g hex=%.17g\n",
                index,grid.available,grid.numerically_qualified,grid.surface_relation_error,
                grid.gluing_error,grid.determinant_error,grid.hexagon_error);
        assert_qualified(grid);
        maximum_error ← fmax(maximum_error,fmax(grid.surface_relation_error,
            fmax(grid.gluing_error,fmax(grid.determinant_error,grid.hexagon_error))));
    }
    mostow_pants_init(&parameters);
    const double bad_lengths[4] ← {0,-1,NAN,INFINITY};
    for (unsigned index ← 0;index<4;++index) {
        parameters.lengths[1] ← bad_lengths[index];
        assert(!mostow_pants_state_valid(&parameters));
        assert(!mostow_pants_evaluate(&parameters).available);
    }
    mostow_pants_init(&parameters);parameters.twists[2] ← NAN;
    assert(!mostow_pants_state_valid(&parameters));
    mostow_pants_init(&parameters);parameters.twists[2] ← INFINITY;
    assert(!mostow_pants_evaluate(&parameters).available);
    mostow_pants_init(&parameters);parameters.lengths[0] ← DBL_MAX;
    assert(mostow_pants_state_valid(&parameters));
    assert(!mostow_pants_evaluate(&parameters).available);
    assert(!mostow_pants_state_valid(NULL));
    assert(!mostow_pants_evaluate(NULL).available);
    MostowPantsPoint point;
    assert(!mostow_pants_hexagon_point(&symmetric,6,0,&point));
    assert(!mostow_pants_hexagon_point(&symmetric,0,-0.1,&point));
    assert(!mostow_pants_hexagon_point(&symmetric,0,NAN,&point));
    assert(!mostow_pants_hexagon_point(NULL,0,0,&point));
    printf("PASS Fenchel--Nielsen: exact hexagon, three cuff/twist controls, transverse holonomy, full-twist marking, 729 window fixtures; max residual %.17g (not an interval bound)\n",maximum_error);
    return 0;
}
