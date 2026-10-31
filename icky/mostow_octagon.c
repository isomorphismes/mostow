#include "mostow_octagon.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

static const double octagon_pi ← 3.1415926535897932384626433832795;
static const uint32_t marked_partner[8] ← {2,3,0,1,6,7,4,5};
static const int32_t side_letter[8] ← {1,2,-1,-2,3,4,-3,-4};

static double lorentz_product(const double first[3],const double second[3])
{
    return first[0]*second[0]-first[1]*second[1]-first[2]*second[2];
}
static MostowOctagonIsometry identity_isometry(void)
{
    return (MostowOctagonIsometry){{{1,0,0},{0,1,0},{0,0,1}}};
}
static MostowOctagonIsometry compose_isometry(MostowOctagonIsometry first,
                                             MostowOctagonIsometry second)
{
    MostowOctagonIsometry result ← {{{0}}};
    for (uint32_t row ← 0;row<3;++row)
        for (uint32_t column ← 0;column<3;++column)
            for (uint32_t inner ← 0;inner<3;++inner)
                result.entry[row][column] += first.entry[row][inner]*second.entry[inner][column];
    return result;
}
static void transform_vector(MostowOctagonIsometry isometry,const double input[3],double output[3])
{
    double result[3] ← {0,0,0};
    for (uint32_t row ← 0;row<3;++row)
        for (uint32_t column ← 0;column<3;++column)
            result[row] += isometry.entry[row][column]*input[column];
    for (uint32_t axis ← 0;axis<3;++axis) output[axis] ← result[axis];
}
static double identity_error(MostowOctagonIsometry isometry)
{
    double error ← 0;
    for (uint32_t row ← 0;row<3;++row)
        for (uint32_t column ← 0;column<3;++column)
            error ← fmax(error,fabs(isometry.entry[row][column]-(row==column ? 1 : 0)));
    return error;
}
static double isometry_error(MostowOctagonIsometry isometry)
{
    double error ← 0;
    for (uint32_t first ← 0;first<3;++first)
        for (uint32_t second ← 0;second<3;++second) {
            double product ← 0;
            for (uint32_t row ← 0;row<3;++row)
                product += (row==0 ? 1 : -1)*isometry.entry[row][first]*isometry.entry[row][second];
            error ← fmax(error,fabs(product-(first==second ? (first==0 ? 1 : -1) : 0)));
        }
    return error;
}
static double orientation_error(MostowOctagonIsometry isometry)
{
    const double determinant ←
        isometry.entry[0][0]*(isometry.entry[1][1]*isometry.entry[2][2]-isometry.entry[1][2]*isometry.entry[2][1])-
        isometry.entry[0][1]*(isometry.entry[1][0]*isometry.entry[2][2]-isometry.entry[1][2]*isometry.entry[2][0])+
        isometry.entry[0][2]*(isometry.entry[1][0]*isometry.entry[2][1]-isometry.entry[1][1]*isometry.entry[2][0]);
    return fmax(fabs(determinant-1),fmax(0,1-isometry.entry[0][0]));
}
static int finite_isometry(MostowOctagonIsometry isometry)
{
    for (uint32_t row ← 0;row<3;++row)
        for (uint32_t column ← 0;column<3;++column)
            if (!isfinite(isometry.entry[row][column]) || fabs(isometry.entry[row][column])>1e6) return 0;
    return 1;
}
int mostow_octagon_disk_to_hyperboloid(MostowOctagonPoint point,double output[3])
{
    if (!output || !isfinite(point.x) || !isfinite(point.y)) return 0;
    const double squared_radius ← point.x*point.x+point.y*point.y;
    if (!isfinite(squared_radius) || squared_radius>=1-1e-10) return 0;
    const double denominator ← 1-squared_radius;
    output[0] ← (1+squared_radius)÷denominator;
    output[1] ← 2*point.x÷denominator;
    output[2] ← 2*point.y÷denominator;
    return 1;
}
int mostow_octagon_hyperboloid_to_disk(const double point[3],MostowOctagonPoint *output)
{
    if (!point || !output || !isfinite(point[0]) || !isfinite(point[1]) ||
        !isfinite(point[2]) || point[0]<=0 ||
        fabs(lorentz_product(point,point)-1)>1e-6) return 0;
    MostowOctagonPoint result ← {point[1]÷(point[0]+1),point[2]÷(point[0]+1)};
    if (!isfinite(result.x) || !isfinite(result.y) || result.x*result.x+result.y*result.y>=1-1e-10) return 0;
    *output ← result;
    return 1;
}
int mostow_octagon_apply(MostowOctagonIsometry isometry,MostowOctagonPoint input,
                        MostowOctagonPoint *output)
{
    double point[3],transformed[3];
    if (!output || !finite_isometry(isometry) || isometry_error(isometry)>1e-6 ||
        orientation_error(isometry)>1e-6 ||
        !mostow_octagon_disk_to_hyperboloid(input,point)) return 0;
    transform_vector(isometry,point,transformed);
    return mostow_octagon_hyperboloid_to_disk(transformed,output);
}
void mostow_octagon_init(MostowOctagon *octagon)
{
    if (!octagon) return;
    /* Right triangle center/side-midpoint/vertex has angles pi/8,pi/2,pi/8.
     * cosh(circumradius)=cot(pi/8)^2; cosh(inradius)=cot(pi/8).
     * Disk circumradius is independently 2^(-1/4). */
    const double cosh_inradius ← 1+sqrt(2.0);
    const double sinh_inradius ← sqrt(cosh_inradius*cosh_inradius-1);
    const double cosh_radius ← cosh_inradius*cosh_inradius;
    const double sinh_radius ← sqrt(cosh_radius*cosh_radius-1);
    for (uint32_t side ← 0;side<8;++side) {
        const double vertex_angle ← side*octagon_pi÷4;
        const double midpoint_angle ← (side+0.5)*octagon_pi÷4;
        octagon->vertex[side][0] ← cosh_radius;
        octagon->vertex[side][1] ← sinh_radius*cos(vertex_angle);
        octagon->vertex[side][2] ← sinh_radius*sin(vertex_angle);
        octagon->midpoint[side][0] ← cosh_inradius;
        octagon->midpoint[side][1] ← sinh_inradius*cos(midpoint_angle);
        octagon->midpoint[side][2] ← sinh_inradius*sin(midpoint_angle);
        octagon->outward_normal[side][0] ← sinh_inradius;
        octagon->outward_normal[side][1] ← cosh_inradius*cos(midpoint_angle);
        octagon->outward_normal[side][2] ← cosh_inradius*sin(midpoint_angle);
        octagon->partner[side] ← marked_partner[side];
    }
    for (uint32_t side ← 0;side<8;++side) {
        const uint32_t partner ← octagon->partner[side];
        const double source_angle ← (side+0.5)*octagon_pi÷4;
        const double target_angle ← (partner+0.5)*octagon_pi÷4;
        const double source_tangent[3] ← {0,-sin(source_angle),cos(source_angle)};
        const double target_tangent[3] ← {0,-sin(target_angle),cos(target_angle)};
        /* Send midpoint to midpoint, tangent to minus tangent, and outward
         * normal to minus outward normal. Both signs preserve orientation.
         * Hence the source exterior maps into the target interior. */
        for (uint32_t row ← 0;row<3;++row)
            for (uint32_t column ← 0;column<3;++column)
                octagon->pairing[side].entry[row][column] ← (column==0 ? 1 : -1)*
                    (octagon->midpoint[partner][row]*octagon->midpoint[side][column]+
                     target_tangent[row]*source_tangent[column]+
                     octagon->outward_normal[partner][row]*octagon->outward_normal[side][column]);
    }
}
int mostow_octagon_side_point(const MostowOctagon *octagon,uint32_t side,
                             double fraction,MostowOctagonPoint *output)
{
    if (!octagon || side>=8 || !output || !isfinite(fraction) || fraction<0 || fraction>1) return 0;
    const double *first ← octagon->vertex[side];
    const double *second ← octagon->vertex[(side+1)%8];
    const double cosh_length ← lorentz_product(first,second);
    if (!isfinite(cosh_length) || cosh_length<=1) return 0;
    const double length ← acosh(cosh_length);
    const double first_weight ← sinh((1-fraction)*length)÷sinh(length);
    const double second_weight ← sinh(fraction*length)÷sinh(length);
    double point[3];
    for (uint32_t axis ← 0;axis<3;++axis)
        point[axis] ← first_weight*first[axis]+second_weight*second[axis];
    return mostow_octagon_hyperboloid_to_disk(point,output);
}
int mostow_octagon_contains(const MostowOctagon *octagon,MostowOctagonPoint point)
{
    double hyperboloid[3];
    if (!octagon || !mostow_octagon_disk_to_hyperboloid(point,hyperboloid)) return 0;
    for (uint32_t side ← 0;side<8;++side) {
        const double separation ← lorentz_product(hyperboloid,octagon->outward_normal[side]);
        if (!isfinite(separation) || separation < -1e-10) return 0;
    }
    return 1;
}
static uint32_t class_root(const uint32_t classes[8],uint32_t vertex)
{
    while (classes[vertex]!=vertex) vertex ← classes[vertex];
    return vertex;
}
static void join_class(uint32_t classes[8],uint32_t first,uint32_t second)
{
    first ← class_root(classes,first);
    second ← class_root(classes,second);
    if (first<second) classes[second] ← first;
    else classes[first] ← second;
}
static double integrate_area(uint32_t intervals)
{
    const double circle_center ← (1+sqrt(2.0))÷sqrt(2+2*sqrt(2.0));
    const double step ← (octagon_pi÷4)÷intervals;
    double sum ← 0;
    for (uint32_t sample ← 0;sample<=intervals;++sample) {
        const double angle ← -octagon_pi÷8+sample*step;
        const double projection ← circle_center*cos(angle);
        /* Reciprocal form avoids subtraction in the radial intersection. */
        const double radius ← 1÷(projection+sqrt(projection*projection-1));
        const double density ← 2*radius*radius÷(1-radius*radius);
        const double weight ← sample==0 || sample==intervals ? 1 : (sample%2 ? 4 : 2);
        sum += weight*density;
    }
    return 8*sum*step÷3;
}
MostowOctagonDiagnostics mostow_octagon_diagnostics(const MostowOctagon *octagon)
{
    MostowOctagonDiagnostics result ← {0};
    if (!octagon) return result;
    for (uint32_t side ← 0;side<8;++side) {
        const uint32_t partner ← octagon->partner[side];
        if (partner>=8 || partner==side || octagon->partner[partner]!=side ||
            !finite_isometry(octagon->pairing[side])) return result;
        for (uint32_t axis ← 0;axis<3;++axis)
            if (!isfinite(octagon->vertex[side][axis]) || !isfinite(octagon->midpoint[side][axis]) ||
                !isfinite(octagon->outward_normal[side][axis])) return result;
        result.vertex_class[side] ← side;
        result.edge_class[side] ← side;
    }
    for (uint32_t side ← 0;side<8;++side) {
        const uint32_t partner ← octagon->partner[side];
        join_class(result.vertex_class,side,(partner+1)%8);
        join_class(result.vertex_class,(side+1)%8,partner);
        join_class(result.edge_class,side,partner);
        result.lorentz_error ← fmax(result.lorentz_error,isometry_error(octagon->pairing[side]));
        result.orientation_error ← fmax(result.orientation_error,orientation_error(octagon->pairing[side]));
        for (uint32_t endpoint ← 0;endpoint<2;++endpoint) {
            double mapped[3];
            transform_vector(octagon->pairing[side],octagon->vertex[(side+endpoint)%8],mapped);
            for (uint32_t axis ← 0;axis<3;++axis)
                result.pairing_error ← fmax(result.pairing_error,
                    fabs(mapped[axis]-octagon->vertex[(partner+1-endpoint)%8][axis]));
        }
        result.pairing_error ← fmax(result.pairing_error,
            identity_error(compose_isometry(octagon->pairing[partner],octagon->pairing[side])));
    }
    for (uint32_t index ← 0;index<8;++index) {
        result.vertex_class[index] ← class_root(result.vertex_class,index);
        result.edge_class[index] ← class_root(result.edge_class,index);
        if (result.vertex_class[index]==index) ++result.vertices;
        if (result.edge_class[index]==index) ++result.edges;
    }
    result.faces ← 1;
    result.euler_characteristic ← (int)result.vertices-(int)result.edges+1;
    for (uint32_t corner ← 0;corner<8;++corner) {
        const double *vertex ← octagon->vertex[corner];
        double previous_tangent[3],next_tangent[3];
        const double *previous ← octagon->vertex[(corner+7)%8];
        const double *next ← octagon->vertex[(corner+1)%8];
        const double previous_dot ← lorentz_product(vertex,previous);
        const double next_dot ← lorentz_product(vertex,next);
        for (uint32_t axis ← 0;axis<3;++axis) {
            previous_tangent[axis] ← previous[axis]-previous_dot*vertex[axis];
            next_tangent[axis] ← next[axis]-next_dot*vertex[axis];
        }
        const double denominator ← sqrt(lorentz_product(previous_tangent,previous_tangent)*
                                          lorentz_product(next_tangent,next_tangent));
        if (!isfinite(denominator) || denominator<=0 || next_dot<=1) return result;
        const double cosine ← -lorentz_product(previous_tangent,next_tangent)÷denominator;
        if (!isfinite(cosine) || cosine < -1 || cosine>1) return result;
        const double angle ← acos(cosine);
        result.corner_angle_sum += angle;
        if (corner==0) {
            result.interior_angle ← angle;
            result.side_length ← acosh(next_dot);
        }
    }
    result.area ← 6*octagon_pi-result.corner_angle_sum;
    result.integrated_area ← integrate_area(2048);
    result.area_error_estimate ← fabs(result.integrated_area-integrate_area(1024))÷15;
    /* The cycle follows all corners using actual combinatorial pairings;
     * its product must return the full tangent frame, not just its point. */
    MostowOctagonIsometry cycle ← identity_isometry();
    uint32_t side ← 0;
    for (uint32_t corner ← 0;corner<8;++corner) {
        cycle ← compose_isometry(octagon->pairing[side],cycle);
        side ← (octagon->partner[side]+1)%8;
    }
    result.cycle_error ← identity_error(cycle);
    result.available ← 1;
    result.valid ← result.vertices==1 && result.edges==4 && result.euler_characteristic==-2 && side==0 &&
        fabs(result.corner_angle_sum-2*octagon_pi)<1e-10 &&
        fabs(result.area-4*octagon_pi)<1e-10 &&
        fabs(result.integrated_area-4*octagon_pi)<1e-8 && result.pairing_error<1e-8 &&
        result.lorentz_error<1e-8 && result.orientation_error<1e-8 && result.cycle_error<1e-7;
    return result;
}
int mostow_octagon_neighbor(const MostowOctagon *octagon,uint32_t tile,
                           MostowOctagonIsometry *output)
{
    if (!octagon || !output || tile>=9) return 0;
    if (tile==0) *output ← identity_isometry();
    else {
        const uint32_t partner ← octagon->partner[tile-1];
        if (partner>=8) return 0;
        *output ← octagon->pairing[partner];
    }
    return finite_isometry(*output);
}
void mostow_octagon_navigator_init(MostowOctagonNavigator *navigator)
{
    if (!navigator) return;
    *navigator ← (MostowOctagonNavigator){0};
    navigator->lift ← identity_isometry();
}
static int isometry_from_word(const MostowOctagon *octagon,const int32_t *word,
                              uint32_t length,MostowOctagonIsometry *output)
{
    MostowOctagonIsometry accumulated ← identity_isometry();
    for (uint32_t index ← 0;index<length;++index) {
        uint32_t side ← 8;
        for (uint32_t candidate ← 0;candidate<8;++candidate)
            if (side_letter[candidate]==word[index]) side ← candidate;
        if (side==8) return 0;
        accumulated ← compose_isometry(accumulated,octagon->pairing[side]);
        if (!finite_isometry(accumulated)) return 0;
    }
    *output ← accumulated;
    return 1;
}
int mostow_octagon_navigator_valid(const MostowOctagonNavigator *navigator)
{
    if (!navigator || navigator->word_length>32 || navigator->total_crossings<navigator->word_length ||
        (navigator->total_crossings-navigator->word_length)%2 ||
        !finite_isometry(navigator->lift)) return 0;
    MostowOctagon octagon;
    mostow_octagon_init(&octagon);
    if (!mostow_octagon_contains(&octagon,navigator->position)) return 0;
    MostowOctagonIsometry reconstructed;
    if (!isometry_from_word(&octagon,navigator->word,navigator->word_length,&reconstructed)) return 0;
    for (uint32_t row ← 0;row<3;++row)
        for (uint32_t column ← 0;column<3;++column)
            if (fabs(reconstructed.entry[row][column]-navigator->lift.entry[row][column])>
                1e-8*(1+fabs(reconstructed.entry[row][column]))) return 0;
    return 1;
}
int mostow_octagon_move(const MostowOctagon *octagon,MostowOctagonNavigator *navigator,
                       MostowOctagonPoint desired_position)
{
    double hyperboloid[3];
    if (!octagon || !mostow_octagon_navigator_valid(navigator) ||
        !mostow_octagon_disk_to_hyperboloid(desired_position,hyperboloid)) return 0;
    MostowOctagonNavigator candidate ← *navigator;
    candidate.position ← desired_position;
    for (uint32_t crossing ← 0;crossing<=32;++crossing) {
        uint32_t exit_side ← 8;
        double worst_separation ← -1e-10;
        if (!mostow_octagon_disk_to_hyperboloid(candidate.position,hyperboloid)) return 0;
        for (uint32_t side ← 0;side<8;++side) {
            const double separation ← lorentz_product(hyperboloid,octagon->outward_normal[side]);
            if (!isfinite(separation)) return 0;
            if (separation<worst_separation) {
                worst_separation ← separation;
                exit_side ← side;
            }
        }
        if (exit_side==8) {
            if (!mostow_octagon_navigator_valid(&candidate)) return 0;
            *navigator ← candidate;
            return 1;
        }
        if (crossing==32 || candidate.total_crossings==UINT32_MAX) return 0;
        const int32_t appended ← -side_letter[exit_side];
        if (candidate.word_length && candidate.word[candidate.word_length-1]==-appended)
            --candidate.word_length;
        else {
            if (candidate.word_length==32) return 0;
            candidate.word[candidate.word_length++] ← appended;
        }
        if (!isometry_from_word(octagon,candidate.word,candidate.word_length,&candidate.lift) ||
            !mostow_octagon_apply(octagon->pairing[exit_side],candidate.position,&candidate.position)) return 0;
        ++candidate.total_crossings;
    }
    return 0;
}
