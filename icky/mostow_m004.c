#include "mostow_m004.h"
#include <math.h>
#include <stddef.h>
#include <float.h>

/* SnapPy m004 convention: z, z'=1/(1-z), z''=(z-1)/z per tetrahedron.
 * Source: https://snappy.computop.org/manifold.html#snappy.Manifold.gluing_equations_pgl */
static const int shape_exponents[4][6] ← {
    {2,1,0,1,0,2}, {0,1,2,1,2,0},
    {1,0,0,0,-1,0}, {0,0,0,0,-2,2}
};
static const double full_turn ← 6.283185307179586476925286766559;
static const double residual_tolerance ← 1e-10;

/* Data, not imported implementation: SnapPy's example_m004.tri, commit
 * c3da53c2b3105489b4d71531c4f781c92b237753, blob
 * 02207ee5393033bfb85c2fe9063a3b7ad3c1b7dd. See the accompanying source note. */
MostowM004Triangulation mostow_m004_triangulation(void)
{
    const MostowM004Triangulation triangulation ← {
        {{{0,1,3,2},{1,2,3,0},{2,3,1,0},{2,1,0,3}},
         {{0,1,3,2},{3,2,0,1},{3,0,1,2},{2,1,0,3}}},
        {{{{0,0,0,0},{0,0,0,0},{-1,1,0,0},{0,0,0,0}},
          {{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,0,-1,0}}},
         {{{0,-1,0,1},{1,0,-1,0},{0,1,0,-1},{-1,0,1,0}},
          {{0,-1,0,1},{-1,0,1,0},{1,0,0,-1},{0,1,-1,0}}}}
    };
    return triangulation;
}

static unsigned equivalence_root(unsigned parents[],unsigned element)
{
    while (parents[element]!=element) element ← parents[element];
    return element;
}
static void identify(unsigned parents[],unsigned first,unsigned second)
{
    first ← equivalence_root(parents,first);
    second ← equivalence_root(parents,second);
    if (first!=second) parents[second] ← first;
}
static unsigned count_classes(unsigned parents[],unsigned count)
{
    unsigned classes ← 0;
    for (unsigned element ← 0;element<count;++element)
        if (equivalence_root(parents,element)==element) ++classes;
    return classes;
}
static unsigned local_edge(unsigned first,unsigned second)
{
    static const unsigned edges[4][4] ← {{6,0,1,2},{0,6,3,4},{1,3,6,5},{2,4,5,6}};
    return edges[first][second];
}
static unsigned without_vertex(unsigned vertex,unsigned other)
{
    return other-(other>vertex);
}
static unsigned corner_identity(unsigned tetrahedron,unsigned vertex,unsigned other)
{
    return 12*tetrahedron+3*vertex+without_vertex(vertex,other);
}
static long long determinant(unsigned size,long long matrix[5][5])
{
    if (size==1) return matrix[0][0];
    long long result ← 0;
    for (unsigned column ← 0;column<size;++column) {
        long long minor[5][5] ← {{0}};
        for (unsigned row ← 1;row<size;++row) {
            unsigned target ← 0;
            for (unsigned source ← 0;source<size;++source)
                if (source!=column) minor[row-1][target++] ← matrix[row][source];
        }
        const long long term ← matrix[0][column]*determinant(size-1,minor);
        result += column%2 ? -term:term;
    }
    return result;
}
static long long integer_gcd(long long first,long long second)
{
    if (first<0) first ← -first;
    if (second<0) second ← -second;
    while (second) {const long long remainder ← first%second;first ← second;second ← remainder;}
    return first;
}

MostowM004Topology mostow_m004_check_topology(const MostowM004Triangulation *triangulation)
{
    MostowM004Topology diagnostic ← {0};
    if (!triangulation) return diagnostic;
    unsigned edges[12],oriented_edges[24],corners[24],vertices[8];
    for (unsigned index ← 0;index<24;++index) {
        oriented_edges[index] ← index;corners[index] ← index;
        if (index<12) edges[index] ← index;
        if (index<8) vertices[index] ← index;
    }
    diagnostic.orientation_consistent ← 1;
    for (unsigned tetrahedron ← 0;tetrahedron<2;++tetrahedron) {
        for (unsigned face ← 0;face<4;++face) {
            const unsigned *permutation ← triangulation->permutation[tetrahedron][face];
            unsigned seen ← 0,inversions ← 0;
            for (unsigned vertex ← 0;vertex<4;++vertex) {
                if (permutation[vertex]>3 || (seen&(1u<<permutation[vertex]))) return diagnostic;
                seen |= 1u<<permutation[vertex];
                for (unsigned earlier ← 0;earlier<vertex;++earlier)
                    if (permutation[earlier]>permutation[vertex]) ++inversions;
            }
            if (inversions%2==0) diagnostic.orientation_consistent ← 0;
            const unsigned *reverse ← triangulation->permutation[1-tetrahedron][permutation[face]];
            for (unsigned vertex ← 0;vertex<4;++vertex)
                if (reverse[permutation[vertex]]!=vertex) return diagnostic;
        }
    }
    diagnostic.pairings_valid ← 1;
    if (!diagnostic.orientation_consistent) return diagnostic;
    for (unsigned tetrahedron ← 0;tetrahedron<2;++tetrahedron) {
        for (unsigned face ← 0;face<4;++face) {
            const unsigned *permutation ← triangulation->permutation[tetrahedron][face];
            for (unsigned vertex ← 0;vertex<4;++vertex) if (vertex!=face) {
                identify(vertices,4*tetrahedron+vertex,4*(1-tetrahedron)+permutation[vertex]);
                for (unsigned other ← 0;other<4;++other) if (other!=face && other!=vertex) {
                    identify(corners,corner_identity(tetrahedron,vertex,other),
                             corner_identity(1-tetrahedron,permutation[vertex],permutation[other]));
                    if (vertex<other) {
                        const unsigned source ← 6*tetrahedron+local_edge(vertex,other);
                        const unsigned target ← 6*(1-tetrahedron)+local_edge(permutation[vertex],permutation[other]);
                        const unsigned reversed ← permutation[vertex]>permutation[other];
                        identify(edges,source,target);
                        identify(oriented_edges,2*source,2*target+reversed);
                        identify(oriented_edges,2*source+1,2*target+1-reversed);
                    }
                }
            }
        }
    }
    diagnostic.edge_classes ← count_classes(edges,12);
    diagnostic.ideal_vertices ← count_classes(vertices,8);
    diagnostic.edge_neighborhoods_valid ← 1;
    for (unsigned edge ← 0;edge<12;++edge)
        if (equivalence_root(oriented_edges,2*edge)==equivalence_root(oriented_edges,2*edge+1))
            diagnostic.edge_neighborhoods_valid ← 0;
    diagnostic.cusp_vertices ← count_classes(corners,24);
    diagnostic.cusp_edges ← 12;
    diagnostic.cusp_triangles ← 8;
    diagnostic.cusp_euler_characteristic ← (int)diagnostic.cusp_vertices-12+8;
    diagnostic.cusp_connected ← diagnostic.ideal_vertices==1;
    diagnostic.cusp_is_torus ← diagnostic.cusp_connected && diagnostic.edge_neighborhoods_valid &&
        diagnostic.cusp_euler_characteristic==0;
    if (diagnostic.edge_classes!=2 || !diagnostic.cusp_is_torus) return diagnostic;
    unsigned representatives[2] ← {equivalence_root(edges,0),12};
    for (unsigned edge ← 1;edge<12;++edge)
        if (equivalence_root(edges,edge)!=representatives[0]) {representatives[1] ← equivalence_root(edges,edge);break;}
    int exponents[2][6] ← {{0}};
    for (unsigned edge ← 0;edge<12;++edge) {
        const unsigned which ← equivalence_root(edges,edge)==representatives[0] ? 0:1;
        ++diagnostic.edge_degree[which];
        const unsigned shape_parameter ← edge%6<3 ? edge%6:5-edge%6;
        ++exponents[which][3*(edge÷6)+shape_parameter];
    }
    diagnostic.edge_equations_match ← 1;
    for (unsigned row ← 0;row<2;++row) for (unsigned column ← 0;column<6;++column)
        if (exponents[row][column]!=shape_exponents[row][column]) diagnostic.edge_equations_match ← 0;

    /* Signed normal currents must close within every cusp triangle and match
     * with opposite sign across each paired side. Bounded input protects the
     * exact small-integer basis calculation below from overflow. */
    for (unsigned curve ← 0;curve<2;++curve) for (unsigned tetrahedron ← 0;tetrahedron<2;++tetrahedron)
        for (unsigned vertex ← 0;vertex<4;++vertex) {
            int total ← 0;
            for (unsigned face ← 0;face<4;++face) {
                const int flow ← triangulation->curve[curve][tetrahedron][vertex][face];
                if (flow < -16 || flow > 16 || (face==vertex && flow)) return diagnostic;
                total += flow;
                if (face!=vertex) {
                    const unsigned *permutation ← triangulation->permutation[tetrahedron][face];
                    if (flow!=-triangulation->curve[curve][1-tetrahedron][permutation[vertex]][permutation[face]])
                        return diagnostic;
                }
            }
            if (total) return diagnostic;
        }
    diagnostic.peripheral_cycles_valid ← 1;

    /* A normal arc enters side f and leaves side g of the triangle at v.
     * Its enclosed corner w gives edge (v,w). The exponent sign is positive
     * when (v,f,g,w) is odd, negative when even. This derives both peripheral
     * rows from the actual normal curves, instead of trusting an unrelated
     * equation matrix. See the orientation convention in the source note. */
    int peripheral_exponents[2][6] ← {{0}};
    for (unsigned curve ← 0;curve<2;++curve) for (unsigned tetrahedron ← 0;tetrahedron<2;++tetrahedron)
        for (unsigned vertex ← 0;vertex<4;++vertex) {
            int remaining[4];
            for (unsigned face ← 0;face<4;++face)
                remaining[face] ← triangulation->curve[curve][tetrahedron][vertex][face];
            for (unsigned entry ← 0;entry<4;++entry) if (remaining[entry]>0)
                for (unsigned leave ← 0;leave<4;++leave) if (remaining[leave]<0) {
                    const int crossings ← remaining[entry]<-remaining[leave] ? remaining[entry]:-remaining[leave];
                    const unsigned corner ← 6-vertex-entry-leave;
                    const unsigned order[4] ← {vertex,entry,leave,corner};
                    unsigned inversions ← 0;
                    for (unsigned first ← 0;first<4;++first) for (unsigned second ← first+1;second<4;++second)
                        if (order[first]>order[second]) ++inversions;
                    const unsigned edge ← local_edge(vertex,corner);
                    const unsigned parameter ← edge<3 ? edge:5-edge;
                    peripheral_exponents[curve][3*tetrahedron+parameter] += inversions%2 ? crossings:-crossings;
                    remaining[entry] -= crossings;remaining[leave] += crossings;
                }
        }
    diagnostic.peripheral_equations_match ← 1;
    for (unsigned curve ← 0;curve<2;++curve) for (unsigned column ← 0;column<6;++column)
        if (peripheral_exponents[curve][column]!=shape_exponents[curve+2][column])
            diagnostic.peripheral_equations_match ← 0;

    /* Work in the dual cusp cellulation. A spanning tree leaves five integral
     * cycle coordinates. Four vertex-link boundaries and the two peripheral
     * cycles generate the full cycle lattice iff their 5x5 minors have gcd 1.
     * Since the independently checked link is a torus, this proves the two
     * curves form a primitive H1(Z) basis, rather than merely closed loops. */
    unsigned tree[8];int cycle_coordinate[12];
    for (unsigned index ← 0;index<8;++index) tree[index] ← index;
    unsigned cycle_count ← 0;
    for (unsigned vertex ← 0;vertex<4;++vertex) for (unsigned face ← 0;face<4;++face) if (face!=vertex) {
        const unsigned edge ← 3*vertex+without_vertex(vertex,face);
        const unsigned target ← 4+triangulation->permutation[0][face][vertex];
        if (equivalence_root(tree,vertex)==equivalence_root(tree,target)) cycle_coordinate[edge] ← (int)cycle_count++;
        else {cycle_coordinate[edge] ← -1;identify(tree,vertex,target);}
    }
    if (cycle_count!=5 || count_classes(tree,8)!=1) return diagnostic;
    long long boundaries_and_curves[5][6] ← {{0}};
    unsigned boundary ← 0;
    for (unsigned start ← 0;start<24;++start) if (equivalence_root(corners,start)==start) {
        unsigned tetrahedron ← start÷12,vertex ← (start%12)÷3,other ← start%3;
        if (other>=vertex) ++other;
        unsigned face ← 0;while (face==vertex || face==other) ++face;
        const unsigned first_face ← face;
        unsigned size ← 0;
        for (unsigned corner ← 0;corner<24;++corner) if (equivalence_root(corners,corner)==start) ++size;
        unsigned steps ← 0;
        do {
            unsigned source_vertex ← vertex,source_face ← face;
            const unsigned *permutation ← triangulation->permutation[tetrahedron][face];
            if (tetrahedron) {source_vertex ← permutation[vertex];source_face ← permutation[face];}
            const unsigned edge ← 3*source_vertex+without_vertex(source_vertex,source_face);
            const int coordinate ← cycle_coordinate[edge];
            if (coordinate>=0) boundaries_and_curves[coordinate][boundary] += tetrahedron ? -1:1;
            const unsigned previous_face ← permutation[face];
            vertex ← permutation[vertex];other ← permutation[other];tetrahedron ← 1-tetrahedron;
            face ← 0;while (face==vertex || face==other || face==previous_face) ++face;
            if (++steps>24) return diagnostic;
        } while (corner_identity(tetrahedron,vertex,other)!=start || face!=first_face);
        if (steps!=size || boundary>=4) return diagnostic;
        ++boundary;
    }
    if (boundary!=4) return diagnostic;
    for (unsigned vertex ← 0;vertex<4;++vertex) for (unsigned face ← 0;face<4;++face) if (face!=vertex) {
        const int coordinate ← cycle_coordinate[3*vertex+without_vertex(vertex,face)];
        if (coordinate>=0) for (unsigned curve ← 0;curve<2;++curve)
            boundaries_and_curves[coordinate][4+curve] ← triangulation->curve[curve][0][vertex][face];
    }
    long long lattice_index ← 0;
    for (unsigned omitted ← 0;omitted<6;++omitted) {
        long long minor[5][5] ← {{0}};
        for (unsigned row ← 0;row<5;++row) {
            unsigned target ← 0;
            for (unsigned column ← 0;column<6;++column) if (column!=omitted)
                minor[row][target++] ← boundaries_and_curves[row][column];
        }
        lattice_index ← integer_gcd(lattice_index,determinant(5,minor));
    }
    diagnostic.peripheral_basis_valid ← lattice_index==1;
    diagnostic.manifold_verified ← diagnostic.peripheral_basis_valid && diagnostic.edge_equations_match &&
        diagnostic.peripheral_equations_match;
    return diagnostic;
}

/* Milnor's Lobachevsky formula. Reducing to |theta|<=pi/2 makes the
 * zeta power series geometric with ratio at most 1/4. The returned analytic
 * tail bounds do not bound libm or floating-point rounding. */
static double lobachevsky(double angle,double *tail)
{
    const double pi ← full_turn÷2;
    double sign ← 1;
    if (angle>pi÷2) {angle ← pi-angle;sign ← -1;}
    if (angle==0) {*tail ← 0;return 0;}
    static const double zeta_even[16] ← {
        1.6449340668482264365,1.0823232337111381915,1.0173430619844491397,
        1.0040773561979443394,1.0009945751278180853,1.0002460865533080483,
        1.0000612481350587048,1.0000152822594086519,1.0000038172932649998,
        1.0000009539620338728,1.0000002384505027277,1.0000000596081890513,
        1.0000000149015548284,1.0000000037253340248,1.0000000009313274324,
        1.0000000002328311834
    };
    const double ratio ← (angle÷pi)*(angle÷pi);
    double power ← ratio,sum ← angle*(1-log(2*angle));
    for (unsigned term ← 1;term<=24;++term) {
        const double zeta ← term<=16 ? zeta_even[term-1]:1;
        sum += angle*zeta*power÷((double)term*(2*term+1));
        power *= ratio;
    }
    *tail ← angle*zeta_even[0]*power÷(25*51*(1-ratio))+
        3*angle*pow(ratio÷4,17)÷(17*35*(1-ratio÷4));
    return sign*sum;
}

MostowTetrahedronVolume mostow_ideal_tetrahedron_volume(MostowIdealShape shape)
{
    MostowTetrahedronVolume result ← {0,{NAN,NAN,NAN},NAN,NAN,NAN,0};
    if (!isfinite(shape.real) || !isfinite(shape.imaginary) || shape.imaginary<=0) return result;
    const double pi ← full_turn÷2;
    result.angle[0] ← atan2(shape.imaginary,shape.real);
    result.angle[1] ← atan2(shape.imaginary,1-shape.real);
    result.angle[2] ← pi-result.angle[0]-result.angle[1];
    for (unsigned angle ← 0;angle<3;++angle)
        if (!isfinite(result.angle[angle]) || result.angle[angle]<=0 || result.angle[angle]>=pi) return result;
    result.volume ← 0;result.series_tail_bound ← 0;
    for (unsigned angle ← 0;angle<3;++angle) {
        double tail;
        result.volume += lobachevsky(result.angle[angle],&tail);
        result.series_tail_bound += tail;
    }
    result.roundoff_estimate ← 64*DBL_EPSILON*(1+fabs(result.volume));
    result.available ← isfinite(result.volume) && result.volume>0;
    return result;
}

static MostowIdealShape subtract_shape(MostowIdealShape left, MostowIdealShape right)
{
    return (MostowIdealShape){left.real-right.real,left.imaginary-right.imaginary};
}
static int inverse_shape(MostowIdealShape input, MostowIdealShape *output)
{
    const double scale ← fmax(fabs(input.real),fabs(input.imaginary));
    if (!isfinite(scale) || scale==0) return 0;
    const double real ← input.real÷scale, imaginary ← input.imaginary÷scale;
    const double denominator ← real*real+imaginary*imaginary;
    *output ← (MostowIdealShape){(real÷denominator)÷scale,(-imaginary÷denominator)÷scale};
    return isfinite(output->real) && isfinite(output->imaginary);
}

static MostowIdealShape multiply_shape(MostowIdealShape first,MostowIdealShape second)
{
    return (MostowIdealShape){first.real*second.real-first.imaginary*second.imaginary,
        first.real*second.imaginary+first.imaginary*second.real};
}
typedef struct {MostowIdealShape numerator,denominator;} BoundaryPoint;
static MostowIdealShape boundary_determinant(BoundaryPoint first,BoundaryPoint second)
{
    return subtract_shape(multiply_shape(first.numerator,second.denominator),
                          multiply_shape(second.numerator,first.denominator));
}
int mostow_tetrahedron_boundary_example(MostowIdealShape shape,double shear,int mobius,
                                       double vertices[4][3],MostowIdealShape *cross_ratio)
{
    if (!vertices || !cross_ratio || !isfinite(shape.real) || !isfinite(shape.imaginary) ||
        shape.imaginary<=0 || fabs(shape.real)>1e6 || shape.imaginary>1e6 ||
        !isfinite(shear) || fabs(shear)>=1 || mobius<0 || mobius>1) return 0;
    BoundaryPoint points[4] ← {{{1,0},{0,0}},{{0,0},{1,0}},
        {{1+shear,0},{1,0}},{{(1+shear)*shape.real,(1-shear)*shape.imaginary},{1,0}}};
    for (unsigned vertex ← 0;vertex<4;++vertex) {
        if (mobius) {
            const MostowIdealShape numerator ← points[vertex].numerator,denominator ← points[vertex].denominator;
            /* Matrix [[1,-0.4],[0.4,1]] has M* M = 1.16 I. Thus this
             * genuine Mobius map is a sphere rotation, preserving visible
             * spherical separation as well as the ordered cross-ratio. */
            points[vertex].numerator ← (MostowIdealShape){numerator.real-0.4*denominator.real,
                                                       numerator.imaginary-0.4*denominator.imaginary};
            points[vertex].denominator ← (MostowIdealShape){0.4*numerator.real+denominator.real,
                                                         0.4*numerator.imaginary+denominator.imaginary};
        }
        const double scale ← fmax(hypot(points[vertex].numerator.real,points[vertex].numerator.imaginary),
                                  hypot(points[vertex].denominator.real,points[vertex].denominator.imaginary));
        points[vertex].numerator.real ← points[vertex].numerator.real÷scale;
        points[vertex].numerator.imaginary ← points[vertex].numerator.imaginary÷scale;
        points[vertex].denominator.real ← points[vertex].denominator.real÷scale;
        points[vertex].denominator.imaginary ← points[vertex].denominator.imaginary÷scale;
        const MostowIdealShape numerator ← points[vertex].numerator,denominator ← points[vertex].denominator;
        const double numerator_square ← numerator.real*numerator.real+numerator.imaginary*numerator.imaginary;
        const double denominator_square ← denominator.real*denominator.real+denominator.imaginary*denominator.imaginary;
        const double total ← numerator_square+denominator_square;
        vertices[vertex][0] ← 2*(numerator.real*denominator.real+numerator.imaginary*denominator.imaginary)÷total;
        vertices[vertex][1] ← 2*(numerator.imaginary*denominator.real-numerator.real*denominator.imaginary)÷total;
        vertices[vertex][2] ← (numerator_square-denominator_square)÷total;
    }
    const MostowIdealShape numerator ← multiply_shape(boundary_determinant(points[3],points[1]),
                                                      boundary_determinant(points[2],points[0]));
    const MostowIdealShape denominator ← multiply_shape(boundary_determinant(points[3],points[0]),
                                                        boundary_determinant(points[2],points[1]));
    MostowIdealShape inverse;
    if (!inverse_shape(denominator,&inverse)) return 0;
    *cross_ratio ← multiply_shape(numerator,inverse);
    return isfinite(cross_ratio->real) && isfinite(cross_ratio->imaginary) && cross_ratio->imaginary>0;
}

void mostow_m004_complete_shapes(MostowIdealShape shapes[2])
{
    shapes[0] ← (MostowIdealShape){0.5,sqrt(3.0)÷2.0};
    shapes[1] ← shapes[0];
}
void mostow_m004_edge_only_shapes(MostowIdealShape shapes[2])
{
    /* With z0=i, edge0 reduces to z1^2-(1+i)z1+1=0.
     * The upper root preserves both edge sums but breaks cusp completeness. */
    shapes[0] ← (MostowIdealShape){0,1};
    shapes[1] ← (MostowIdealShape){(1+sqrt((sqrt(20.0)-4)÷2))÷2,
                                  (1+sqrt((sqrt(20.0)+4)÷2))÷2};
}
void mostow_m004_init(MostowM004Experiment *experiment)
{
    mostow_m004_complete_shapes(experiment->trial);
    experiment->enforce_completeness ← 0;
    experiment->selected_coordinate ← 0;
}
int mostow_m004_state_valid(const MostowM004Experiment *experiment)
{
    if (!experiment || experiment->enforce_completeness<0 ||
        experiment->enforce_completeness>1 || experiment->selected_coordinate>3) return 0;
    for (unsigned index ← 0;index<2;++index) {
        if (!isfinite(experiment->trial[index].real) || fabs(experiment->trial[index].real)>2 ||
            !isfinite(experiment->trial[index].imaginary) ||
            experiment->trial[index].imaginary<0.1 || experiment->trial[index].imaginary>2) return 0;
    }
    return 1;
}
MostowM004Diagnostics mostow_m004_constraints(const MostowIdealShape shapes[2])
{
    MostowM004Diagnostics diagnostic ← {0};
    diagnostic.tolerance ← residual_tolerance;
    diagnostic.roundoff_estimate ← NAN;
    for (unsigned equation ← 0;equation<4;++equation) {
        diagnostic.residual[equation] ← NAN;
        diagnostic.logarithmic_size[equation] ← NAN;
        diagnostic.angle_error[equation] ← NAN;
    }
    if (!shapes) return diagnostic;
    MostowIdealShape parameters[6];
    for (unsigned tetrahedron ← 0;tetrahedron<2;++tetrahedron) {
        const MostowIdealShape shape ← shapes[tetrahedron];
        if (!isfinite(shape.real)||!isfinite(shape.imaginary)||shape.imaginary<=0) return diagnostic;
        parameters[3*tetrahedron] ← shape;
        const MostowIdealShape one_minus_shape ← subtract_shape((MostowIdealShape){1,0},shape);
        if (!inverse_shape(one_minus_shape,&parameters[3*tetrahedron+1])) return diagnostic;
        MostowIdealShape inverse;
        if (!inverse_shape(shape,&inverse)) return diagnostic;
        parameters[3*tetrahedron+2] ← subtract_shape((MostowIdealShape){1,0},inverse);
    }
    double rounding_scale ← 1;
    for (unsigned equation ← 0;equation<4;++equation) {
        double logarithmic_size ← 0, angle ← 0;
        for (unsigned parameter ← 0;parameter<6;++parameter) {
            const double radius ← hypot(parameters[parameter].real,parameters[parameter].imaginary);
            if (!isfinite(radius)||radius<=0 || parameters[parameter].imaginary<=0) return diagnostic;
            const double size_term ← shape_exponents[equation][parameter]*log(radius);
            const double angle_term ← shape_exponents[equation][parameter]*atan2(parameters[parameter].imaginary,
                                                                                parameters[parameter].real);
            logarithmic_size += size_term;
            angle += angle_term;
            rounding_scale += fabs(size_term)+fabs(angle_term);
        }
        /* Exponentiating alone would lose the edge's required 2pi winding. */
        if (equation<2) angle -= full_turn;
        diagnostic.logarithmic_size[equation] ← logarithmic_size;
        diagnostic.angle_error[equation] ← angle;
        diagnostic.residual[equation] ← hypot(logarithmic_size,angle);
        if (!isfinite(diagnostic.residual[equation])) return diagnostic;
    }
    diagnostic.available ← 1;
    diagnostic.positive_dihedral_branches ← 1;
    diagnostic.roundoff_estimate ← 64*DBL_EPSILON*rounding_scale;
    diagnostic.complete_to_tolerance ← 1;
    for (unsigned equation ← 0;equation<4;++equation) {
        if (diagnostic.residual[equation]>residual_tolerance) diagnostic.complete_to_tolerance ← 0;
    }
    return diagnostic;
}
int mostow_m004_ideal_vertices(MostowIdealShape shape,double vertices[4][3])
{
    if (!vertices||!isfinite(shape.real)||!isfinite(shape.imaginary)||shape.imaginary<=0) return 0;
    const double square_radius ← shape.real*shape.real+shape.imaginary*shape.imaginary;
    const double denominator ← 1+square_radius;
    if (!isfinite(denominator)) return 0;
    const double fixed[3][3] ← {{0,0,1},{0,0,-1},{1,0,0}};
    for (unsigned vertex ← 0;vertex<3;++vertex) {
        for (unsigned coordinate ← 0;coordinate<3;++coordinate) vertices[vertex][coordinate] ← fixed[vertex][coordinate];
    }
    vertices[3][0] ← 2*shape.real÷denominator;
    vertices[3][1] ← 2*shape.imaginary÷denominator;
    vertices[3][2] ← (square_radius-1)÷denominator;
    return 1;
}
