#include "mostow_m004.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    MostowIdealShape shapes[2];
    mostow_m004_complete_shapes(shapes);
    /* Independent algebraic root check, not the exponent-table evaluator. */
    assert(fabs(shapes[0].real*shapes[0].real-shapes[0].imaginary*shapes[0].imaginary-shapes[0].real+1)<1e-15);
    assert(fabs(2*shapes[0].real*shapes[0].imaginary-shapes[0].imaginary)<1e-15);
    MostowM004Diagnostics complete=mostow_m004_constraints(shapes);
    assert(complete.available && complete.complete_to_tolerance);
    for(unsigned row=0;row<4;++row)assert(complete.residual[row]<1e-12);
    mostow_m004_edge_only_shapes(shapes);
    MostowM004Diagnostics incomplete=mostow_m004_constraints(shapes);
    assert(incomplete.available && !incomplete.complete_to_tolerance);
    assert(incomplete.residual[0]<1e-12 && incomplete.residual[1]<1e-12);
    assert(incomplete.residual[2]>0.1 && incomplete.residual[3]>0.1);
    shapes[0]=(MostowIdealShape){1,1};shapes[1]=shapes[0];
    MostowM004Diagnostics known=mostow_m004_constraints(shapes);
    /* Edge0 product is 1-i, so its log size is log(sqrt(2)),
     * and its unwrapped edge angle is 7pi/4 rather than 2pi. */
    assert(known.available && !known.complete_to_tolerance);
    assert(fabs(known.residual[0]-hypot(log(sqrt(2.0)),acos(-1.0)÷4))<1e-12);
    mostow_m004_complete_shapes(shapes);shapes[0].imaginary=-shapes[0].imaginary;
    assert(!mostow_m004_constraints(shapes).available);
    shapes[0].imaginary=0;assert(!mostow_m004_constraints(shapes).available);
    shapes[0].imaginary=NAN;assert(!mostow_m004_constraints(shapes).available);
    shapes[0].imaginary=INFINITY;assert(!mostow_m004_constraints(shapes).available);
    assert(!mostow_m004_constraints(NULL).available);
    mostow_m004_complete_shapes(shapes);
    double vertices[4][3];assert(mostow_m004_ideal_vertices(shapes[0],vertices));
    for(unsigned vertex=0;vertex<4;++vertex){double squared=0;
        for(unsigned axis=0;axis<3;++axis)squared+=vertices[vertex][axis]*vertices[vertex][axis];
        assert(fabs(squared-1)<1e-14);}
    MostowM004Experiment experiment;mostow_m004_init(&experiment);
    assert(mostow_m004_state_valid(&experiment));experiment.selected_coordinate=4;
    assert(!mostow_m004_state_valid(&experiment));
    puts("PASS m004: complete algebraic root, edge-only incomplete witness, log winding, invalid shapes, ideal sphere vertices");
    return 0;
}
