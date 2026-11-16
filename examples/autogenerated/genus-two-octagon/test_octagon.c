#include "mostow_octagon.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static double disk_distance(MostowOctagonPoint first,MostowOctagonPoint second)
{
    const double squared_difference ← (first.x-second.x)*(first.x-second.x)+
                                      (first.y-second.y)*(first.y-second.y);
    return acosh(1+2*squared_difference÷((1-first.x*first.x-first.y*first.y)*
                                        (1-second.x*second.x-second.y*second.y)));
}
static int near_point(MostowOctagonPoint first,MostowOctagonPoint second)
{
    return hypot(first.x-second.x,first.y-second.y)<1e-10;
}
int main(void)
{
    const double pi ← acos(-1.0);
    MostowOctagon octagon;
    mostow_octagon_init(&octagon);
    const MostowOctagonDiagnostics diagnostic ← mostow_octagon_diagnostics(&octagon);
    assert(diagnostic.available && diagnostic.valid);
    assert(diagnostic.vertices==1 && diagnostic.edges==4 && diagnostic.faces==1);
    assert(diagnostic.euler_characteristic==-2);
    for (uint32_t side ← 0;side<8;++side) {
        assert(diagnostic.vertex_class[side]==0);
        assert(octagon.partner[octagon.partner[side]]==side);
    }
    /* Independent right-triangle identity and the Poincare conformal angle.
     * cosh(circumradius)=(1+sqrt(2))^2 implies disk radius 2^(-1/4).
     * Area quadrature integrates the metric density, not Gauss--Bonnet. */
    MostowOctagonPoint vertex;
    assert(mostow_octagon_hyperboloid_to_disk(octagon.vertex[0],&vertex));
    assert(fabs(hypot(vertex.x,vertex.y)-pow(2.0,-0.25))<1e-14);
    assert(fabs(diagnostic.interior_angle-pi÷4)<1e-12);
    assert(fabs(diagnostic.corner_angle_sum-2*pi)<1e-11);
    assert(fabs(diagnostic.area-4*pi)<1e-11);
    assert(fabs(diagnostic.integrated_area-4*pi)<1e-8);
    assert(diagnostic.area_error_estimate<1e-8);
    assert(fabs(diagnostic.side_length-2*acosh(1+sqrt(2.0)))<1e-12);
    assert(diagnostic.pairing_error<1e-10);
    assert(diagnostic.lorentz_error<1e-10);
    assert(diagnostic.orientation_error<1e-10);
    assert(diagnostic.cycle_error<1e-7);
    assert(mostow_octagon_contains(&octagon,(MostowOctagonPoint){0,0}));
    assert(!mostow_octagon_contains(&octagon,(MostowOctagonPoint){0.99,0}));
    assert(!mostow_octagon_contains(&octagon,(MostowOctagonPoint){NAN,0}));
    assert(!mostow_octagon_disk_to_hyperboloid((MostowOctagonPoint){1,0},octagon.midpoint[0]));

    for (uint32_t side ← 0;side<8;++side) {
        MostowOctagonPoint start,middle,end,mapped,target;
        assert(mostow_octagon_side_point(&octagon,side,0,&start));
        assert(mostow_octagon_side_point(&octagon,side,0.5,&middle));
        assert(mostow_octagon_side_point(&octagon,side,1,&end));
        assert(fabs(disk_distance(start,middle)-diagnostic.side_length÷2)<1e-12);
        assert(fabs(disk_distance(start,end)-diagnostic.side_length)<1e-12);
        for (uint32_t sample ← 0;sample<=8;++sample) {
            const double fraction ← sample÷8.0;
            assert(mostow_octagon_side_point(&octagon,side,fraction,&start));
            assert(mostow_octagon_apply(octagon.pairing[side],start,&mapped));
            assert(mostow_octagon_side_point(&octagon,octagon.partner[side],1-fraction,&target));
            assert(near_point(mapped,target));
        }
        /* Cross each actual side, then return through its paired side.
         * The cover lift must remain continuous across the coordinate jump. */
        MostowOctagonNavigator navigator;
        mostow_octagon_navigator_init(&navigator);
        const double angle ← (side+0.5)*pi÷4;
        const double inradius ← acosh(1+sqrt(2.0));
        const double outside_radius ← tanh((inradius+0.02)÷2);
        const double inside_radius ← tanh((inradius-0.02)÷2);
        const MostowOctagonPoint outside ← {outside_radius*cos(angle),outside_radius*sin(angle)};
        const MostowOctagonPoint inside ← {inside_radius*cos(angle),inside_radius*sin(angle)};
        assert(!mostow_octagon_contains(&octagon,outside));
        assert(mostow_octagon_move(&octagon,&navigator,outside));
        assert(navigator.word_length==1 && navigator.total_crossings==1);
        assert(mostow_octagon_contains(&octagon,navigator.position));
        assert(mostow_octagon_apply(navigator.lift,navigator.position,&mapped));
        assert(near_point(mapped,outside));
        assert(mostow_octagon_apply(octagon.pairing[side],inside,&target));
        assert(mostow_octagon_move(&octagon,&navigator,target));
        assert(navigator.word_length==0 && navigator.total_crossings==2);
        assert(near_point(navigator.position,inside));
        assert(mostow_octagon_navigator_valid(&navigator));
    }
    /* First-ring tiles are compared by actual actions on three independent
     * points. Merely distinct word spellings would not establish this. */
    MostowOctagonIsometry tiles[9];
    for (uint32_t tile ← 0;tile<9;++tile) {
        assert(mostow_octagon_neighbor(&octagon,tile,&tiles[tile]));
        for (uint32_t previous ← 0;previous<tile;++previous) {
            double difference ← 0;
            for (uint32_t row ← 0;row<3;++row)
                for (uint32_t column ← 0;column<3;++column)
                    difference ← fmax(difference,fabs(tiles[tile].entry[row][column]-tiles[previous].entry[row][column]));
            assert(difference>0.01);
        }
    }
    assert(!mostow_octagon_neighbor(&octagon,9,&tiles[0]));
    /* Known-bad data witnesses: topology, isometry, angle and transport are
     * separately observed. These fail if their constraints are discarded. */
    MostowOctagon broken ← octagon;
    broken.partner[0] ← 0;
    assert(!mostow_octagon_diagnostics(&broken).available);
    broken ← octagon;
    broken.pairing[0] ← tiles[0];
    assert(!mostow_octagon_diagnostics(&broken).valid);
    MostowOctagon identity_pairings ← octagon;
    for (uint32_t side ← 0;side<8;++side) identity_pairings.pairing[side] ← tiles[0];
    /* Identity maps are Lorentz isometries and have identity cycle product.
     * Only the actual endpoint-identification condition rejects this lie. */
    assert(!mostow_octagon_diagnostics(&identity_pairings).valid);
    MostowOctagon reflected_pairings ← octagon;
    for (uint32_t side ← 0;side<8;++side) {
        const double *normal ← octagon.outward_normal[octagon.partner[side]];
        /* Reflect the image in its target geodesic. Endpoints stay fixed,
         * but normal orientation reverses and surface orientation fails. */
        for (uint32_t column ← 0;column<3;++column) {
            const double normal_component ← normal[0]*octagon.pairing[side].entry[0][column]-
                normal[1]*octagon.pairing[side].entry[1][column]-normal[2]*octagon.pairing[side].entry[2][column];
            for (uint32_t row ← 0;row<3;++row)
                reflected_pairings.pairing[side].entry[row][column] += 2*normal_component*normal[row];
        }
    }
    const MostowOctagonDiagnostics reflection_diagnostic ← mostow_octagon_diagnostics(&reflected_pairings);
    assert(reflection_diagnostic.available && !reflection_diagnostic.valid);
    assert(reflection_diagnostic.orientation_error>1);
    broken ← octagon;
    broken.vertex[0][1] *= 0.9;
    assert(!mostow_octagon_diagnostics(&broken).valid);
    MostowOctagonNavigator navigator;
    mostow_octagon_navigator_init(&navigator);
    const MostowOctagonNavigator saved ← navigator;
    assert(!mostow_octagon_move(&octagon,&navigator,(MostowOctagonPoint){1,0}));
    assert(memcmp(&navigator,&saved,sizeof navigator)==0);
    navigator.lift.entry[0][0] ← 2;
    assert(!mostow_octagon_navigator_valid(&navigator));
    navigator ← saved;
    navigator.word_length ← 33;
    assert(!mostow_octagon_navigator_valid(&navigator));
    navigator ← saved;
    navigator.word_length ← 1;
    navigator.total_crossings ← 1;
    navigator.word[0] ← 5;
    assert(!mostow_octagon_navigator_valid(&navigator));
    navigator ← saved;
    navigator.total_crossings ← 1;
    assert(!mostow_octagon_navigator_valid(&navigator));
    printf("PASS octagon: V=%u E=%u F=%u chi=%d, area=%.12g integral=%.12g estimate=%.3g, pairing=%.3g Lorentz=%.3g cycle=%.3g; eight crossing/reverse and corrupt-state controls\n",
           diagnostic.vertices,diagnostic.edges,diagnostic.faces,diagnostic.euler_characteristic,
           diagnostic.area,diagnostic.integrated_area,diagnostic.area_error_estimate,
           diagnostic.pairing_error,diagnostic.lorentz_error,diagnostic.cycle_error);
    return 0;
}
