#include "mostow_m004.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    MostowM004Triangulation triangulation=mostow_m004_triangulation();
    MostowM004Topology topology=mostow_m004_check_topology(&triangulation);
    assert(topology.pairings_valid && topology.orientation_consistent);
    assert(topology.edge_neighborhoods_valid && topology.edge_classes==2);
    assert(topology.edge_degree[0]==6 && topology.edge_degree[1]==6);
    assert(topology.ideal_vertices==1 && topology.cusp_is_torus);
    assert(topology.cusp_vertices==4 && topology.cusp_edges==12 && topology.cusp_triangles==8);
    assert(topology.cusp_euler_characteristic==0);
    assert(topology.edge_equations_match && topology.peripheral_cycles_valid && topology.peripheral_equations_match);
    assert(topology.peripheral_basis_valid && topology.manifold_verified);
    /* Reciprocal-pairing, cycle and primitive-basis mutation controls. */
    MostowM004Triangulation bad=triangulation;
    bad.permutation[0][0][0]=1;
    assert(!mostow_m004_check_topology(&bad).manifold_verified);
    bad=triangulation;
    for(unsigned vertex=0;vertex<4;++vertex)bad.permutation[0][0][vertex]=(vertex+1)%4;
    assert(!mostow_m004_check_topology(&bad).pairings_valid);
    bad=triangulation;bad.curve[0][0][2][0]=0;
    assert(!mostow_m004_check_topology(&bad).peripheral_cycles_valid);
    bad=triangulation;
    for(unsigned tetrahedron=0;tetrahedron<2;++tetrahedron)
        for(unsigned vertex=0;vertex<4;++vertex)for(unsigned face=0;face<4;++face)
            bad.curve[0][tetrahedron][vertex][face]*=2;
    MostowM004Topology nonprimitive=mostow_m004_check_topology(&bad);
    assert(nonprimitive.peripheral_cycles_valid && !nonprimitive.peripheral_basis_valid);
    bad=triangulation;
    for(unsigned tetrahedron=0;tetrahedron<2;++tetrahedron)
        for(unsigned vertex=0;vertex<4;++vertex)for(unsigned face=0;face<4;++face)
            bad.curve[1][tetrahedron][vertex][face]=bad.curve[0][tetrahedron][vertex][face];
    assert(!mostow_m004_check_topology(&bad).peripheral_basis_valid);
    bad=triangulation;
    for(unsigned tetrahedron=0;tetrahedron<2;++tetrahedron)
        for(unsigned vertex=0;vertex<4;++vertex)for(unsigned face=0;face<4;++face)
            bad.curve[1][tetrahedron][vertex][face]+=bad.curve[0][tetrahedron][vertex][face];
    MostowM004Topology different_marking=mostow_m004_check_topology(&bad);
    assert(different_marking.peripheral_basis_valid && !different_marking.peripheral_equations_match);
    assert(!mostow_m004_check_topology(NULL).manifold_verified);
    MostowIdealShape shapes[2];
    mostow_m004_complete_shapes(shapes);
    MostowTetrahedronVolume regular=mostow_ideal_tetrahedron_volume(shapes[0]);
    /* Independent SnapPy 3.3.2 high-precision and Bloch-Wigner fixtures. */
    assert(regular.available && !regular.rigorous_certificate);
    assert(fabs(2*regular.volume-2.02988321281930725004240510854904057188)<1e-13);
    for(unsigned angle=0;angle<3;++angle)assert(fabs(regular.angle[angle]-acos(-1.0)÷3)<1e-15);
    assert(regular.series_tail_bound<1e-17 && regular.roundoff_estimate>0);
    MostowTetrahedronVolume right=mostow_ideal_tetrahedron_volume((MostowIdealShape){0,1});
    assert(right.available && fabs(right.volume-0.91596559417721901505460351493238411077)<1e-13);
    MostowTetrahedronVolume nonregular=mostow_ideal_tetrahedron_volume((MostowIdealShape){0.7,1.2});
    assert(nonregular.available && nonregular.volume<regular.volume);
    assert(fabs(nonregular.volume-0.97029202838851009873698930154523175829)<1e-13);
    /* Six cross-ratio representatives preserve the oriented tetrahedron's
     * positive volume where the representative remains in the upper half-plane. */
    MostowTetrahedronVolume cyclic=mostow_ideal_tetrahedron_volume((MostowIdealShape){0.5,0.5});
    assert(cyclic.available && fabs(cyclic.volume-right.volume)<1e-13);
    assert(!mostow_ideal_tetrahedron_volume((MostowIdealShape){0,-1}).available);
    assert(!mostow_ideal_tetrahedron_volume((MostowIdealShape){1,0}).available);
    assert(!mostow_ideal_tetrahedron_volume((MostowIdealShape){NAN,1}).available);
    for(unsigned first=1;first<12;++first)for(unsigned second=1;first+second<12;++second){
        const double alpha=acos(-1.0)*first÷12,beta=acos(-1.0)*second÷12;
        const double radius=sin(beta)÷sin(alpha+beta);
        MostowTetrahedronVolume sample=mostow_ideal_tetrahedron_volume(
            (MostowIdealShape){radius*cos(alpha),radius*sin(alpha)});
        assert(sample.available && sample.volume<=regular.volume+1e-13);
    }
    /* Independent algebraic root check, not the exponent-table evaluator. */
    assert(fabs(shapes[0].real*shapes[0].real-shapes[0].imaginary*shapes[0].imaginary-shapes[0].real+1)<1e-15);
    assert(fabs(2*shapes[0].real*shapes[0].imaginary-shapes[0].imaginary)<1e-15);
    MostowM004Diagnostics complete=mostow_m004_constraints(shapes);
    assert(complete.available && complete.complete_to_tolerance);
    assert(complete.positive_dihedral_branches && !complete.rigorous_certificate);
    assert(complete.tolerance==1e-10 && complete.roundoff_estimate>0);
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
    MostowIdealShape transformed;
    assert(mostow_tetrahedron_boundary_example(shapes[0],0,1,vertices,&transformed));
    assert(hypot(transformed.real-shapes[0].real,transformed.imaginary-shapes[0].imaginary)<1e-14);
    assert(mostow_tetrahedron_boundary_example(shapes[0],0.25,0,vertices,&transformed));
    assert(fabs(transformed.real-0.5)<1e-14);
    assert(fabs(transformed.imaginary-shapes[0].imaginary*0.6)<1e-14);
    assert(mostow_ideal_tetrahedron_volume(transformed).volume<regular.volume-0.01);
    assert(!mostow_tetrahedron_boundary_example(shapes[0],1,0,vertices,&transformed));
    assert(!mostow_tetrahedron_boundary_example(shapes[0],NAN,0,vertices,&transformed));
    MostowM004Experiment experiment;mostow_m004_init(&experiment);
    assert(mostow_m004_state_valid(&experiment));experiment.selected_coordinate=4;
    assert(!mostow_m004_state_valid(&experiment));
    puts("PASS m004: pinned face topology, torus cusp and primitive peripheral basis, independent volume, complete algebraic root, edge-only incomplete witness, log winding, invalid shapes, ideal sphere vertices");
    return 0;
}
