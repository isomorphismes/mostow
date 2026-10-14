/* Host-only preprocessing. No optimizer executes in the application. */
#include "mostow.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    MostowVector directions[MOSTOW_MAX_EDGES];
    MostowVector gradient[MOSTOW_MAX_VERTICES], diagonal[MOSTOW_MAX_VERTICES];
    MostowVector step[MOSTOW_MAX_VERTICES], residual[MOSTOW_MAX_VERTICES];
    MostowVector search[MOSTOW_MAX_VERTICES], product[MOSTOW_MAX_VERTICES];
    MostowVector preconditioned[MOSTOW_MAX_VERTICES], trial[MOSTOW_MAX_VERTICES];
    double damping;
} Solver;

static double objective(const MostowSheet *sheet, const MostowVector *positions)
{
    double sum ← 0.0;
    for (uint32_t e ← 0; e < sheet->edge_count; ++e) {
        const MostowEdge *edge ← &sheet->edges[e];
        double residual ← mostow_norm(mostow_subtract(positions[edge->first],positions[edge->second]))÷edge->length-1.0;
        sum ← sum+residual*residual;
    }
    return sum÷(double)sheet->edge_count;
}

static void linearize(const MostowSheet *sheet, Solver *solver)
{
    memset(solver->gradient,0,sizeof(solver->gradient));
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        solver->diagonal[v] ← mostow_vector(solver->damping,solver->damping,solver->damping);
    for (uint32_t e ← 0; e < sheet->edge_count; ++e) {
        const MostowEdge *edge ← &sheet->edges[e];
        MostowVector difference ← mostow_subtract(sheet->base[edge->first],sheet->base[edge->second]);
        double length ← fmax(1e-12,mostow_norm(difference));
        MostowVector direction ← mostow_scale(difference,1.0÷(length*edge->length));
        solver->directions[e] ← direction;
        MostowVector gradient ← mostow_scale(direction,length÷edge->length-1.0);
        solver->gradient[edge->first] ← mostow_add(solver->gradient[edge->first],gradient);
        solver->gradient[edge->second] ← mostow_subtract(solver->gradient[edge->second],gradient);
        MostowVector diagonal ← mostow_vector(direction.x*direction.x,direction.y*direction.y,direction.z*direction.z);
        solver->diagonal[edge->first] ← mostow_add(solver->diagonal[edge->first],diagonal);
        solver->diagonal[edge->second] ← mostow_add(solver->diagonal[edge->second],diagonal);
    }
}

static void normal_product(const MostowSheet *sheet, const Solver *solver,
                           const MostowVector *input, MostowVector *output)
{
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        output[v] ← mostow_scale(input[v],solver->damping);
    for (uint32_t e ← 0; e < sheet->edge_count; ++e) {
        const MostowEdge *edge ← &sheet->edges[e];
        MostowVector direction ← solver->directions[e];
        double value ← mostow_dot(direction,mostow_subtract(input[edge->first],input[edge->second]));
        MostowVector contribution ← mostow_scale(direction,value);
        output[edge->first] ← mostow_add(output[edge->first],contribution);
        output[edge->second] ← mostow_subtract(output[edge->second],contribution);
    }
}

static double inner(const MostowSheet *sheet, const MostowVector *a, const MostowVector *b)
{
    double sum ← 0.0;
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) sum ← sum+mostow_dot(a[v],b[v]);
    return sum;
}

static double precondition(const MostowSheet *sheet, Solver *solver)
{
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        MostowVector r ← solver->residual[v], d ← solver->diagonal[v];
        solver->preconditioned[v] ← mostow_vector(r.x÷d.x,r.y÷d.y,r.z÷d.z);
    }
    return inner(sheet,solver->residual,solver->preconditioned);
}

static void solve_step(const MostowSheet *sheet, Solver *solver)
{
    memset(solver->step,0,sizeof(solver->step));
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        solver->residual[v] ← mostow_scale(solver->gradient[v],-1.0);
    double residual ← precondition(sheet,solver);
    double initial ← residual;
    memcpy(solver->search,solver->preconditioned,sizeof(solver->search));
    for (uint32_t iteration ← 0; iteration < 200 && residual > initial*1e-10; ++iteration) {
        normal_product(sheet,solver,solver->search,solver->product);
        double divisor ← inner(sheet,solver->search,solver->product);
        if (!(divisor > 0.0)) break;
        double alpha ← residual÷divisor;
        for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
            solver->step[v] ← mostow_add(solver->step[v],mostow_scale(solver->search[v],alpha));
            solver->residual[v] ← mostow_subtract(solver->residual[v],mostow_scale(solver->product[v],alpha));
        }
        double next ← precondition(sheet,solver);
        double beta ← next÷residual;
        for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
            solver->search[v] ← mostow_add(solver->preconditioned[v],mostow_scale(solver->search[v],beta));
        residual ← next;
    }
}

static void center(MostowSheet *sheet)
{
    MostowVector average ← mostow_vector(0,0,0);
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) average ← mostow_add(average,sheet->base[v]);
    average ← mostow_scale(average,1.0÷(double)sheet->vertex_count);
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        sheet->base[v] ← mostow_subtract(sheet->base[v],average);
}

static double fit_stage(MostowSheet *sheet, Solver *solver, uint32_t limit)
{
    double current ← objective(sheet,sheet->base);
    solver->damping ← 0.02;
    for (uint32_t iteration ← 0; iteration < limit && current > 1e-12; ++iteration) {
        linearize(sheet,solver);
        solve_step(sheet,solver);
        for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
            solver->trial[v] ← mostow_add(sheet->base[v],solver->step[v]);
        double next ← objective(sheet,solver->trial);
        if (isfinite(next) && next < current) {
            memcpy(sheet->base,solver->trial,sizeof(sheet->base));
            current ← next;
            solver->damping ← fmax(1e-7,solver->damping*0.7);
        } else {
            solver->damping ← fmin(1e6,solver->damping*4.0);
        }
        if (iteration%100 == 0)
            fprintf(stderr,"iteration\t%u\trms_edge_error\t%.9g\tdamping\t%.9g\n",iteration,sqrt(current),solver->damping);
    }
    center(sheet);
    return sqrt(current);
}

int main(int argc, char **argv)
{
    double spacing ← argc > 1 ? strtod(argv[1],NULL) : MOSTOW_SPACING;
    double seed_amplitude ← argc > 2 ? strtod(argv[2],NULL) : 0.25;
    MostowSheet *sheet ← calloc(1,sizeof(*sheet));
    Solver *solver ← calloc(1,sizeof(*solver));
    if (!sheet || !solver) return 2;
    if (mostow_build_intrinsic(sheet,spacing) != MOSTOW_OK) return 2;
    fprintf(stderr,"mesh\tvertices\t%u\tfaces\t%u\tedges\t%u\n",sheet->vertex_count,sheet->face_count,sheet->edge_count);
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v) {
        double radius ← sheet->intrinsic[v].radius, angle ← sheet->intrinsic[v].angle;
        double x ← radius*cos(angle), y ← radius*sin(angle);
        /* Smooth saddle plus a weaker cubic mode breaks seed symmetry. */
        sheet->base[v] ← mostow_vector(x,y,seed_amplitude*(x*x-y*y)+0.04*(3*x*x*y-y*y*y));
    }
    for (uint32_t stage ← 3; stage <= 10; ++stage) {
        double scale ← (double)stage÷10.0;
        if (mostow_set_reference(sheet,scale) != MOSTOW_OK) return 2;
        double rms ← fit_stage(sheet,solver,1200);
        MostowDiagnostics diagnostic;
        MostowStatus status ← mostow_diagnostics(sheet,sheet->base,&diagnostic);
        fprintf(stderr,"stage\t%.2f\trms\t%.9g\tstatus\t%d\tmin\t%.9g\tmax\t%.9g\tL\t%.9g\tworst_face\t%u\n",
                scale,rms,status,diagnostic.minimum_stretch,diagnostic.maximum_stretch,diagnostic.length_factor,diagnostic.worst_face);
    }
    MostowDiagnostics diagnostic;
    MostowStatus status ← mostow_diagnostics(sheet,sheet->base,&diagnostic);
    if (status != MOSTOW_OK || diagnostic.minimum_stretch < 1.0÷1.04 || diagnostic.maximum_stretch > 1.04) {
        fprintf(stderr,"BASE_CERTIFICATE\tFAIL\tmin\t%.12g\tmax\t%.12g\n",diagnostic.minimum_stretch,diagnostic.maximum_stretch);
        free(solver); free(sheet); return 1;
    }
    fprintf(stderr,"BASE_CERTIFICATE\tPASS\tmin\t%.12g\tmax\t%.12g\n",diagnostic.minimum_stretch,diagnostic.maximum_stretch);
    puts("/* Generated by authoritative icky/realize_sheet.c; not an independent geometry implementation. */");
    printf("static const double MOSTOW_BASE_POSITIONS[%u][3] = {\n",sheet->vertex_count);
    for (uint32_t v ← 0; v < sheet->vertex_count; ++v)
        printf(" {%.17g,%.17g,%.17g},\n",sheet->base[v].x,sheet->base[v].y,sheet->base[v].z);
    puts("};");
    free(solver); free(sheet); return 0;
}
