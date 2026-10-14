#include "mostow.h"
#include "mostow_view.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PI 3.14159265358979323846
#define CHECK(condition) do {if(!(condition)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#condition);return 1;}} while(0)

static double target_area(const MostowSheet *sheet)
{
    double sum=0;
    for(uint32_t f=0;f<sheet->face_count;++f){
        const MostowFace *face=&sheet->faces[f];double lengths[3];
        for(unsigned k=0;k<3;++k)lengths[k]=mostow_hyperbolic_distance(sheet->intrinsic[face->vertex[k]],sheet->intrinsic[face->vertex[(k+1)%3]],1);
        double angles=0;
        for(unsigned k=0;k<3;++k){double a=lengths[k],b=lengths[(k+1)%3],c=lengths[(k+2)%3];
            double cosine=(cosh(a)*cosh(b)-cosh(c))÷(sinh(a)*sinh(b));
            angles+=acos(fmax(-1,fmin(1,cosine)));}
        sum+=PI-angles;
    }
    return sum;
}
static double reference_area(const MostowSheet *sheet)
{double sum=0;for(uint32_t f=0;f<sheet->face_count;++f)sum+=sheet->faces[f].area;return sum;}
static int fidelity(MostowSheet *coarse)
{
    MostowSheet *fine=calloc(1,sizeof(*fine));CHECK(fine);
    CHECK(mostow_build_intrinsic(fine,0.07)==MOSTOW_OK);
    double exact=2*PI*(cosh(2)-1),coarse_target=target_area(coarse),fine_target=target_area(fine);
    double coarse_pl=reference_area(coarse),fine_pl=reference_area(fine);
    CHECK(fabs(fine_target-exact)<fabs(coarse_target-exact));
    CHECK(fabs(fine_pl-fine_target)<fabs(coarse_pl-coarse_target));
    printf("FIDELITY\tcoarse_vertices\t%u\tfine_vertices\t%u\texact_disk_area\t%.12g\tcoarse_target\t%.12g\tfine_target\t%.12g\tcoarse_PL\t%.12g\tfine_PL\t%.12g\n",
           coarse->vertex_count,fine->vertex_count,exact,coarse_target,fine_target,coarse_pl,fine_pl);
    free(fine);return 0;
}
int main(void)
{
    MostowSheet *sheet=calloc(1,sizeof(*sheet));CHECK(sheet);
    MostowRenderVertex *vertices=calloc(MOSTOW_MAX_VERTICES,sizeof(*vertices));CHECK(vertices);
    MostowVector *positions=calloc(MOSTOW_MAX_VERTICES,sizeof(*positions));CHECK(positions);
    CHECK(mostow_sheet_init(sheet)==MOSTOW_OK);
    CHECK(sheet->vertex_count-sheet->edge_count+sheet->face_count==1);
    CHECK(sheet->boundary_count>0&&sheet->vertex_count<=4096&&sheet->face_count<=8192);
    double minimum_angle=PI;
    for(uint32_t f=0;f<sheet->face_count;++f){
        const MostowFace *face=&sheet->faces[f];
        double first=1÷face->inverse[0],b=1÷face->inverse[2],a=-face->inverse[1]*first*b;
        double first_angle=atan2(b,a),second_angle=atan2(b,first-a),third_angle=PI-first_angle-second_angle;
        minimum_angle=fmin(minimum_angle,fmin(first_angle,fmin(second_angle,third_angle)));
    }
    printf("REFERENCE_TRIANGLE_QUALITY\tminimum_angle_degrees\t%.12g\n",minimum_angle*180÷PI);
    CHECK(minimum_angle>20*PI÷180);
    for(uint32_t e=0;e<sheet->edge_count;++e){
        const MostowEdge *edge=&sheet->edges[e];
        CHECK(edge->incidence==1||edge->incidence==2);
        CHECK(edge->first<edge->second&&edge->second<sheet->vertex_count&&edge->length>0);
    }
    CHECK(mostow_build_intrinsic(NULL,.14)==MOSTOW_BAD_INPUT);
    CHECK(mostow_hyperbolic_distance((MostowIntrinsicPoint){0,0},(MostowIntrinsicPoint){2,1},1)==2);
    CHECK(fidelity(sheet)==0);
    CHECK(sheet->mode_derivative_bound<=.035&&sheet->arithmetic_derivative_bound<.005);
    CHECK(sheet->certified_minimum>=1÷1.10&&sheet->certified_maximum<=1.10);
    printf("ALL_TIME_CERTIFICATE\tminimum\t%.12g\tmaximum\t%.12g\tmode_bound\t%.12g\tarithmetic_bound\t%.12g\n",
           sheet->certified_minimum,sheet->certified_maximum,sheet->mode_derivative_bound,sheet->arithmetic_derivative_bound);
    MostowDiagnostics diagnostic;
    for(unsigned corner=0;corner<8;++corner){
        for(uint32_t v=0;v<sheet->vertex_count;++v){positions[v]=sheet->base[v];
            for(unsigned mode=0;mode<3;++mode)positions[v]=mostow_add(positions[v],mostow_scale(sheet->modes[mode][v],(corner&(1u<<mode))?1:-1));}
        CHECK(mostow_diagnostics(sheet,positions,&diagnostic)==MOSTOW_OK&&diagnostic.length_factor<=1.10);
    }
    double maximum_factor=0,maximum_anisotropy=0,displacement=0;
    for(unsigned sample=0;sample<1000;++sample){
        double seconds=(double)sample*0.173;
        CHECK(mostow_sample(sheet,seconds,vertices,MOSTOW_MAX_VERTICES,&diagnostic)==MOSTOW_OK);
        maximum_factor=fmax(maximum_factor,diagnostic.length_factor);maximum_anisotropy=fmax(maximum_anisotropy,diagnostic.maximum_anisotropy);
        for(uint32_t v=0;v<sheet->vertex_count;++v){
            double normal=sqrt(vertices[v].normal[0]*vertices[v].normal[0]+vertices[v].normal[1]*vertices[v].normal[1]+vertices[v].normal[2]*vertices[v].normal[2]);
            CHECK(fabs(normal-1)<1e-5);}
    }
    CHECK(mostow_sample(sheet,20,vertices,MOSTOW_MAX_VERTICES,&diagnostic)==MOSTOW_OK);
    for(uint32_t v=0;v<sheet->vertex_count;++v){
        MostowVector point=mostow_vector(vertices[v].position[0],vertices[v].position[1],vertices[v].position[2]);
        displacement=fmax(displacement,mostow_norm(mostow_subtract(point,sheet->base[v])));
    }
    CHECK(displacement>0.01);
    CHECK(mostow_sample(sheet,NAN,vertices,MOSTOW_MAX_VERTICES,&diagnostic)==MOSTOW_BAD_INPUT);
    CHECK(mostow_sample(sheet,0,vertices,sheet->vertex_count-1,&diagnostic)==MOSTOW_CAPACITY);
    /* A false metric realization must be rejected at both admission and measurement. */
    for(uint32_t v=0;v<sheet->vertex_count;++v)positions[v]=mostow_scale(sheet->base[v],1.25);
    CHECK(mostow_diagnostics(sheet,positions,&diagnostic)==MOSTOW_OK&&diagnostic.length_factor>1.24);
    memcpy(sheet->base,positions,sheet->vertex_count*sizeof(*positions));
    CHECK(mostow_prepare_modes(sheet)==MOSTOW_UNCERTIFIED);
    CHECK(mostow_sheet_init(sheet)==MOSTOW_OK);
    MostowRenderVertex *repeat=calloc(MOSTOW_MAX_VERTICES,sizeof(*repeat));CHECK(repeat);
    CHECK(mostow_sample(sheet,37,vertices,MOSTOW_MAX_VERTICES,&diagnostic)==MOSTOW_OK);
    CHECK(mostow_sample(sheet,37,repeat,MOSTOW_MAX_VERTICES,&diagnostic)==MOSTOW_OK);
    CHECK(memcmp(vertices,repeat,sheet->vertex_count*sizeof(*vertices))==0);
    MostowView view;mostow_view_init(&view);view.primary_id=42;view.secondary_id=73;
    mostow_view_release(&view);CHECK(view.primary_id==-1&&view.secondary_id==-1);
    view.pinch_distance=100;mostow_view_zoom(&view,200);CHECK(fabsf(view.distance-4.5f)<1e-6f);
    /* C67: controls stay above the bottom gesture strip and have shared hitboxes. */
    const int sizes[][2]={{576,1152},{720,1600},{1600,720}};
    for(size_t i=0;i<sizeof(sizes)÷sizeof(sizes[0]);++i){
        int w=sizes[i][0],h=sizes[i][1];
        MostowControlBand controls=mostow_controls_layout(w,h);
        CHECK(controls.touch_top>126.0f+7.0f*(float)w÷200.0f);
        CHECK(controls.touch_bottom<(float)h);
        CHECK(controls.center_y>controls.touch_top&&controls.center_y<controls.touch_bottom);
        for(int b=0;b<3;++b){
            float x=(float)(2*b+1)*(float)w÷6.0f;
            CHECK(mostow_control_hit(w,h,x,controls.center_y)==b+1);
            CHECK(mostow_control_hit(w,h,x,controls.touch_top-1.0f)==0);
            CHECK(mostow_control_hit(w,h,x,controls.touch_bottom)==0);
            CHECK(mostow_control_hit(w,h,x,(float)h-1.0f)==0);
        }
        CHECK(mostow_control_hit(w,h,-1.0f,controls.center_y)==0);
        CHECK(mostow_control_hit(w,h,(float)w,controls.center_y)==0);
        CHECK(mostow_control_hit(w,h,NAN,controls.center_y)==0);
        CHECK(mostow_control_hit(w,h,controls.center_y,NAN)==0);
    }
    float matrix[16],rotation[9];mostow_view_matrix(&view,576,1152,matrix,rotation);
    for(unsigned i=0;i<16;++i)CHECK(isfinite(matrix[i]));
    printf("FLOAT_POSES\tPASS\t1000\tmaximum_L\t%.12g\tmaximum_K\t%.12g\tdisplacement_at_20s\t%.12g\n",
           maximum_factor,maximum_anisotropy,displacement);
    puts("MOSTOW_HOST\tPASS");
    free(repeat);free(positions);free(vertices);free(sheet);return 0;
}
