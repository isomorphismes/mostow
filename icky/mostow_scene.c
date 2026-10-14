/* Application-owned semantic state, projective boundary action and picking.
 * No sheet construction or motion formula is duplicated here. */
#define _POSIX_C_SOURCE 200809L
#include "mostow_scene.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void mostow_scene_init(MostowScene *scene)
{
    memset(scene,0,sizeof(*scene));
    scene->schema=3;scene->seed=1729;
    mostow_m004_init(&scene->rigidity);
    snprintf(scene->source,sizeof(scene->source),"%s",MOSTOW_SOURCE_REVISION);
    snprintf(scene->fixture,sizeof(scene->fixture),"%s",MOSTOW_FIXTURE_REVISION);
    mostow_view_init(&scene->camera);
    scene->light[0]=-0.4f;scene->light[1]=0.6f;scene->light[2]=0.9f;
    scene->rings=1;scene->information=1;scene->normal=1;
    scene->anchor.face=-1;scene->selected_string=0;scene->preset=1;scene->angle=1.2;
}
int mostow_scene_valid(const MostowScene *s)
{
    if(!s||s->schema!=3||strcmp(s->source,MOSTOW_SOURCE_REVISION)||
       strcmp(s->fixture,MOSTOW_FIXTURE_REVISION))return 0;
    if(s->encounter<0||s->encounter>2||!mostow_m004_state_valid(&s->rigidity))return 0;
    if(!isfinite(s->time)||s->time<0||s->time>1e9||
       !isfinite(s->camera.yaw)||fabsf(s->camera.yaw)>1e6f||
       !isfinite(s->camera.pitch)||fabsf(s->camera.pitch)>1.5f||
       !isfinite(s->camera.distance)||s->camera.distance<4||s->camera.distance>20)return 0;
    if(!isfinite(s->angle)||fabs(s->angle)>6.28318530718||
       !isfinite(s->boost)||fabs(s->boost)>3||!isfinite(s->shear)||fabs(s->shear)>3||
       !isfinite(s->view_turn)||fabs(s->view_turn)>1e6||
       !isfinite(s->view_shift)||fabs(s->view_shift)>2)return 0;
    double light=0;for(unsigned i=0;i<3;++i){if(!isfinite(s->light[i])||fabsf(s->light[i])>2)return 0;light+=s->light[i]*s->light[i];}
    if(light<0.01||s->anchor.face < -1||s->anchor.face>=1881)return 0;
    if(s->anchor.face>=0){double total=0;for(unsigned i=0;i<3;++i){
        if(!isfinite(s->anchor.weights[i])||s->anchor.weights[i]<0||s->anchor.weights[i]>1)return 0;
        total+=s->anchor.weights[i];}if(fabs(total-1)>1e-8)return 0;}
    const int32_t flags[]={s->model,s->illustrated,s->paused,s->rings,s->mesh,s->normal,s->information};
    for(unsigned i=0;i<sizeof(flags)÷sizeof(flags[0]);++i)if(flags[i]<0||flags[i]>1)return 0;
    return s->selected_string>=0&&s->selected_string<(int32_t)MOSTOW_STRING_COUNT&&
           s->preset>=0&&s->preset<4&&s->control_page>=0&&s->control_page<3&&s->map_parameter>=0&&s->map_parameter<3;
}
int mostow_scene_write(const MostowScene *s,const char *path)
{
    if(!mostow_scene_valid(s)||!path)return 0;
    char temporary[1024];if(snprintf(temporary,sizeof(temporary),"%s.tmp",path)>=(int)sizeof(temporary))return 0;
    FILE *file=fopen(temporary,"w");if(!file)return 0;
    int ok=fprintf(file,
      "MOSTOW_SCENE 3\nsource %s\nfixture %s\nseed %u\n"
      "camera %a %a %a\ntime %a\nmap %a %a %a\nview %a %a\nlight %a %a %a\n"
      "flags %d %d %d %d %d %d %d %d\nselection %d %d %a %a %a\ncontrols %d %d %d\n"
      "m004 %a %a %a %a %d %u\nEND\n",
      s->source,s->fixture,s->seed,(double)s->camera.yaw,(double)s->camera.pitch,(double)s->camera.distance,
      s->time,s->angle,s->boost,s->shear,s->view_turn,s->view_shift,
      (double)s->light[0],(double)s->light[1],(double)s->light[2],
      s->encounter,s->model,s->illustrated,s->paused,s->rings,s->mesh,s->normal,s->information,
      s->selected_string,s->anchor.face,s->anchor.weights[0],s->anchor.weights[1],s->anchor.weights[2],
      s->preset,s->control_page,s->map_parameter,
      s->rigidity.trial[0].real,s->rigidity.trial[0].imaginary,
      s->rigidity.trial[1].real,s->rigidity.trial[1].imaginary,
      s->rigidity.enforce_completeness,s->rigidity.selected_coordinate)>0;
    if(fflush(file)||fsync(fileno(file)))ok=0;
    if(fclose(file))ok=0;
    if(!ok||rename(temporary,path)){remove(temporary);return 0;}return 1;
}
int mostow_scene_read(MostowScene *scene,const char *path)
{
    FILE *file=fopen(path,"r");if(!file)return 0;
    MostowScene value;mostow_scene_init(&value);char end[8]={0};
    unsigned version=0;double yaw=0,pitch=0,distance=0,lx=0,ly=0,lz=0;
    int count=fscanf(file,
      "MOSTOW_SCENE %u source %64s fixture %64s seed %u camera %la %la %la time %la "
      "map %la %la %la view %la %la light %la %la %la flags %d %d %d %d %d %d %d %d "
      "selection %d %d %la %la %la controls %d %d %d "
      "m004 %la %la %la %la %d %u %7s",
      &version,value.source,value.fixture,&value.seed,&yaw,&pitch,&distance,&value.time,
      &value.angle,&value.boost,&value.shear,&value.view_turn,&value.view_shift,&lx,&ly,&lz,
      &value.encounter,&value.model,&value.illustrated,&value.paused,&value.rings,&value.mesh,&value.normal,&value.information,
      &value.selected_string,&value.anchor.face,&value.anchor.weights[0],&value.anchor.weights[1],&value.anchor.weights[2],
      &value.preset,&value.control_page,&value.map_parameter,
      &value.rigidity.trial[0].real,&value.rigidity.trial[0].imaginary,
      &value.rigidity.trial[1].real,&value.rigidity.trial[1].imaginary,
      &value.rigidity.enforce_completeness,&value.rigidity.selected_coordinate,end);
    int tail;do{tail=fgetc(file);}while(tail==' '||tail=='\n'||tail=='\r'||tail=='\t');
    int io=ferror(file);fclose(file);
    value.schema=version;value.camera.yaw=(float)yaw;value.camera.pitch=(float)pitch;value.camera.distance=(float)distance;
    value.light[0]=(float)lx;value.light[1]=(float)ly;value.light[2]=(float)lz;
    if(count!=39||strcmp(end,"END")||tail!=EOF||io||!mostow_scene_valid(&value))return 0;
    *scene=value;return 1;
}
MostowBoundaryMap mostow_boundary_map(double angle,double boost,double shear)
{
    double cosine=cos(angle*0.5),sine=sin(angle*0.5),stretch=exp(boost*0.5);
    return (MostowBoundaryMap){cosine*stretch,cosine*stretch*shear-sine÷stretch,
        sine*stretch,sine*stretch*shear+cosine÷stretch};
}
MostowMapClass mostow_map_classify(MostowBoundaryMap m)
{
    if(!isfinite(m.a)||!isfinite(m.b)||!isfinite(m.c)||!isfinite(m.d))return MOSTOW_NONFINITE_MAP;
    int exact_small_integer=fabs(m.a)<=1048576&&fabs(m.b)<=1048576&&fabs(m.c)<=1048576&&fabs(m.d)<=1048576&&
        m.a==trunc(m.a)&&m.b==trunc(m.b)&&m.c==trunc(m.c)&&m.d==trunc(m.d);
    double exact_determinant=m.a*m.d-m.b*m.c;
    double exact_discriminant=(m.a+m.d)*(m.a+m.d)-4*exact_determinant;
    if(exact_small_integer){
        if(exact_determinant==0)return MOSTOW_SINGULAR_MAP;
        if(exact_determinant<0)return MOSTOW_REVERSED_MAP;
        if(m.b==0&&m.c==0&&m.a==m.d)return MOSTOW_IDENTITY;
        if(exact_discriminant==0)return MOSTOW_PARABOLIC;
        return exact_discriminant<0?MOSTOW_ELLIPTIC:MOSTOW_HYPERBOLIC;
    }
    double scale=fmax(fmax(fabs(m.a),fabs(m.b)),fmax(fabs(m.c),fabs(m.d)));
    if(scale==0)return MOSTOW_SINGULAR_MAP;
    m.a=m.a÷scale;m.b=m.b÷scale;m.c=m.c÷scale;m.d=m.d÷scale;
    double determinant=m.a*m.d-m.b*m.c;
    if(fabs(determinant)<=64*DBL_EPSILON)return MOSTOW_UNCERTAIN_MAP;
    if(determinant<0)return MOSTOW_REVERSED_MAP;
    if(m.b==0&&m.c==0&&m.a==m.d)return MOSTOW_IDENTITY;
    if(m.a==m.d&&(m.b==0||m.c==0))return MOSTOW_PARABOLIC;
    double trace=m.a+m.d,discriminant=trace*trace-4*determinant;
    double tolerance=128*DBL_EPSILON*(trace*trace+4*determinant);
    if(fabs(discriminant)<=tolerance)return MOSTOW_UNCERTAIN_MAP;
    return discriminant<0?MOSTOW_ELLIPTIC:MOSTOW_HYPERBOLIC;
}
const char *mostow_map_class_name(MostowMapClass c)
{
    static const char *names[]={"IDENTITY","ELLIPTIC","PARABOLIC","HYPERBOLIC","SINGULAR","REVERSED","UNCERTAIN","NONFINITE"};
    return c>=MOSTOW_IDENTITY&&c<=MOSTOW_NONFINITE_MAP?names[c]:"INVALID";
}
int mostow_boundary_apply(MostowBoundaryMap map,MostowPoint2 input,MostowPoint2 *output)
{
    MostowMapClass classification=mostow_map_classify(map);
    if(classification==MOSTOW_SINGULAR_MAP||classification==MOSTOW_REVERSED_MAP||classification==MOSTOW_NONFINITE_MAP)return 0;
    if(!isfinite(input.x)||!isfinite(input.y)||fabs(hypot(input.x,input.y)-1)>1e-8)return 0;
    double scale=fmax(fmax(fabs(map.a),fabs(map.b)),fmax(fabs(map.c),fabs(map.d)));
    map.a=map.a÷scale;map.b=map.b÷scale;map.c=map.c÷scale;map.d=map.d÷scale;
    /* RP1 uses homogeneous pairs. Infinity is [1:0], never a huge finite x. */
    double half=atan2(input.y,input.x)*0.5;
    double x=map.a*cos(half)+map.b*sin(half),y=map.c*cos(half)+map.d*sin(half);
    double length=hypot(x,y);if(!isfinite(length)||length<DBL_MIN)return 0;
    x=x÷length;y=y÷length;*output=(MostowPoint2){x*x-y*y,2*x*y};return 1;
}
MostowPoint2 mostow_klein_to_poincare(MostowPoint2 p)
{
    double denominator=1+sqrt(fmax(0,1-p.x*p.x-p.y*p.y));
    return (MostowPoint2){p.x÷denominator,p.y÷denominator};
}
MostowPoint2 mostow_poincare_to_klein(MostowPoint2 p)
{
    double factor=2÷(1+p.x*p.x+p.y*p.y);return (MostowPoint2){p.x*factor,p.y*factor};
}
int mostow_string(const MostowScene *scene,uint32_t identity,MostowString *string)
{
    if(identity>=MOSTOW_STRING_COUNT)return 0;
    double angle=6.283185307179586*(double)identity÷MOSTOW_STRING_COUNT;
    MostowPoint2 input={cos(angle),sin(angle)},image;
    if(!mostow_boundary_apply(mostow_boundary_map(scene->angle,scene->boost,scene->shear),input,&image))return 0;
    MostowBoundaryMap viewpoint=mostow_boundary_map(scene->view_turn,scene->view_shift,0);
    string->identity=identity;
    if(!mostow_boundary_apply(viewpoint,input,&string->first)||!mostow_boundary_apply(viewpoint,image,&string->second))return 0;
    string->degenerate=hypot(string->first.x-string->second.x,string->first.y-string->second.y)<1e-12;
    string->diameter=!string->degenerate&&hypot(string->first.x+string->second.x,string->first.y+string->second.y)<1e-12;
    return 1;
}
MostowPoint2 mostow_string_point(const MostowString *s,int model,double fraction)
{
    MostowPoint2 chord={(1-fraction)*s->first.x+fraction*s->second.x,(1-fraction)*s->first.y+fraction*s->second.y};
    return model?mostow_klein_to_poincare(chord):chord;
}
int mostow_geodesic_circle(const MostowString *s,MostowPoint2 *center,double *radius)
{
    if(s->degenerate||s->diameter)return 0;
    double determinant=s->first.x*s->second.y-s->first.y*s->second.x;
    if(fabs(determinant)<1e-14)return 0;
    *center=(MostowPoint2){(s->second.y-s->first.y)÷determinant,(s->first.x-s->second.x)÷determinant};
    *radius=sqrt(fmax(0,center->x*center->x+center->y*center->y-1));return 1;
}
int mostow_anchor_sample(const MostowSheet *sheet,const MostowRenderVertex *vertices,MostowAnchor anchor,float p[3],float n[3])
{
    if(anchor.face<0||(uint32_t)anchor.face>=sheet->face_count)return 0;
    for(unsigned coordinate=0;coordinate<3;++coordinate){p[coordinate]=0;n[coordinate]=0;
        for(unsigned corner=0;corner<3;++corner){uint16_t vertex=sheet->faces[anchor.face].vertex[corner];
            p[coordinate]+=(float)(anchor.weights[corner]*vertices[vertex].position[coordinate]);
            n[coordinate]+=(float)(anchor.weights[corner]*vertices[vertex].normal[coordinate]);}}
    float norm=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(norm<1e-8f)return 0;
    for(unsigned coordinate=0;coordinate<3;++coordinate)n[coordinate]=n[coordinate]÷norm;
    return 1;
}
int mostow_project(const float matrix[16],const float p[3],int w,int h,float output[3])
{
    float clip[4];for(unsigned row=0;row<4;++row)clip[row]=matrix[row]*p[0]+matrix[row+4]*p[1]+matrix[row+8]*p[2]+matrix[row+12];
    if(clip[3]<=1e-7f)return 0;
    output[0]=(1+clip[0]÷clip[3])*(float)w*0.5f;
    output[1]=(1-clip[1]÷clip[3])*(float)h*0.5f;output[2]=clip[2]÷clip[3];return 1;
}
int mostow_sheet_pick(const MostowSheet *sheet,const MostowRenderVertex *vertices,const MostowView *view,int w,int h,float x,float y,MostowAnchor *anchor)
{
    float matrix[16],rotation[9];mostow_view_matrix(view,w,h,matrix,rotation);
    double nearest=2;MostowAnchor found={-1,{0,0,0}};
    for(uint32_t face=0;face<sheet->face_count;++face){float points[3][3],reciprocal[3];int valid=1;
        for(unsigned corner=0;corner<3;++corner){const float *p=vertices[sheet->faces[face].vertex[corner]].position;
            if(!mostow_project(matrix,p,w,h,points[corner])){valid=0;break;}
            reciprocal[corner]=1÷(matrix[3]*p[0]+matrix[7]*p[1]+matrix[11]*p[2]+matrix[15]);}
        if(!valid)continue;
        double determinant=(points[1][1]-points[2][1])*(points[0][0]-points[2][0])+(points[2][0]-points[1][0])*(points[0][1]-points[2][1]);
        if(fabs(determinant)<1e-8)continue;
        double weights[3];weights[0]=((points[1][1]-points[2][1])*(x-points[2][0])+(points[2][0]-points[1][0])*(y-points[2][1]))÷determinant;
        weights[1]=((points[2][1]-points[0][1])*(x-points[2][0])+(points[0][0]-points[2][0])*(y-points[2][1]))÷determinant;
        weights[2]=1-weights[0]-weights[1];if(weights[0]<0||weights[1]<0||weights[2]<0)continue;
        double depth=0,total=0;for(unsigned corner=0;corner<3;++corner){depth+=weights[corner]*points[corner][2];weights[corner]*=reciprocal[corner];total+=weights[corner];}
        if(depth < -1||depth>1||depth>=nearest-1e-7)continue;
        nearest=depth;found.face=(int32_t)face;for(unsigned corner=0;corner<3;++corner)found.weights[corner]=weights[corner]÷total;
    }
    if(found.face<0)return 0;
    *anchor=found;return 1;
}
void mostow_scene_control_rect(int w,int h,int slot,float r[4])
{
    float row_height=fminf((float)h*0.068f,(float)w*0.17f);
    r[0]=(float)(slot%3)*(float)w÷3+6;r[1]=(float)h*0.76f+(float)(slot÷3)*row_height;
    r[2]=(float)w÷3-12;r[3]=row_height-6;
}
int mostow_scene_control_hit(const MostowScene *s,int w,int h,float x,float y)
{
    if(w<=0||h<=0||!isfinite(x)||!isfinite(y))return 0;
    for(int slot=0;slot<6;++slot){float r[4];mostow_scene_control_rect(w,h,slot,r);
        if(x>=r[0]&&x<r[0]+r[2]&&y>=r[1]&&y<r[1]+r[3])return 1+s->control_page*6+slot;}
    if(s->control_page==1&&y>h*0.69f&&y<h*0.74f&&x>=0&&x<w)return s->encounter?21:20;
    if(s->control_page==2&&y>h*0.69f&&y<h*0.74f&&x>=0&&x<w)return s->encounter?23:22;
    return 0;
}
const char *mostow_scene_control_label(const MostowScene *s,int slot)
{
    static const char *labels[3][6]={{"SHEET","INK","DETAIL","PAUSE","SAVE","MORE"},
      {"RINGS","MESH","NORMAL","LIGHT L","LIGHT R","MORE"},
      {"REOPEN","EXPORT","MODEL","PRESET","MAP","MORE"}};
    if(s->control_page==0&&slot==0)return s->encounter==2?"M004":(s->encounter?"STRINGS":"SHEET");
    if(s->encounter==2&&s->control_page==1){static const char *shape_parameters[6]={"Z0 REAL","Z0 IMAG","Z1 REAL","Z1 IMAG","REGULAR","MORE"};return shape_parameters[slot];}
    if(s->encounter==2&&s->control_page==2&&slot==2)return s->rigidity.enforce_completeness?"FREE":"COMPLETE";
    if(s->encounter==2&&s->control_page==2&&slot==3)return "EDGE ONLY";
    if(s->control_page==0&&slot==1)return s->illustrated?"CLEAN":"INK";
    if(s->control_page==0&&slot==3)return s->paused?"PLAY":"PAUSE";
    if(s->control_page==2&&slot==2)return s->model?"KLEIN":"POINCARE";
    if(s->encounter==1&&s->control_page==1){static const char *parameters[6]={"ANGLE","BOOST","SHEAR","VIEW L","VIEW R","MORE"};return parameters[slot];}
    return labels[s->control_page][slot];
}
void mostow_scene_control(MostowScene *s,int control,float fraction)
{
    fraction=fmaxf(0,fminf(1,fraction));
    if(control==6||control==12||control==18){s->control_page=(s->control_page+1)%3;return;}
    if(s->encounter==2){
        if(control>=7&&control<=10){s->rigidity.selected_coordinate=(unsigned)(control-7);return;}
        if(control==11){mostow_m004_complete_shapes(s->rigidity.trial);return;}
        if(control==15){s->rigidity.enforce_completeness=!s->rigidity.enforce_completeness;return;}
        if(control==16){mostow_m004_edge_only_shapes(s->rigidity.trial);return;}
        if(control==21||control==23){unsigned coordinate=s->rigidity.selected_coordinate;
            if(coordinate%2==0)s->rigidity.trial[coordinate÷2].real=2*fraction-0.5;
            else s->rigidity.trial[coordinate÷2].imaginary=0.1+1.9*fraction;
            return;}
    }
    switch(control){
      case 1:s->encounter=(s->encounter+1)%3;break;case 2:s->illustrated=!s->illustrated;break;
      case 3:s->information=!s->information;break;case 4:s->paused=!s->paused;break;
      case 7:case 8:case 9:
        if(s->encounter)s->map_parameter=control-7;
        else if(control==7)s->rings=!s->rings;else if(control==8)s->mesh=!s->mesh;else s->normal=!s->normal;
        break;
      case 10:case 11:{double angle=atan2(s->light[1],s->light[0])+(control==10?-0.25:0.25);
        if(s->encounter)s->view_turn+=control==10?-0.25:0.25;
        else{s->light[0]=(float)cos(angle);s->light[1]=(float)sin(angle);}break;}
      case 15:s->model=!s->model;break;
      case 16:s->preset=(s->preset+1)%4;s->angle=s->preset==1?1.2:0;s->boost=s->preset==3?1.2:0;s->shear=s->preset==2?1:0;break;
      case 17:s->control_page=1;break;
      case 20:s->time=180*fraction;s->paused=1;break;
      case 21:if(s->map_parameter==0)s->angle=(2*fraction-1)*6.283185307179586;
        else if(s->map_parameter==1)s->boost=(2*fraction-1)*3;else s->shear=(2*fraction-1)*3;
        break;
      case 22:s->light[2]=2*fraction-1;break;
      case 23:s->view_shift=4*fraction-2;break;
      default:break;
    }
}
