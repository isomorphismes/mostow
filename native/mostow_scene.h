#ifndef MOSTOW_SCENE_H
#define MOSTOW_SCENE_H
#include "mostow.h"
#include "mostow_view.h"
#include "mostow_m004.h"
#include "mostow_octagon.h"
#include "mostow_pants.h"
#include "mostow_distortion.h"
#ifndef MOSTOW_SOURCE_REVISION
#define MOSTOW_SOURCE_REVISION "unbound-development-source"
#endif
#define MOSTOW_FIXTURE_REVISION "d539265d7d195b6771cc6ac04f6af84384e33f1fc78ec5bed71a22d1b7d89d58"
#define MOSTOW_STRING_COUNT 144u
#define MOSTOW_STRING_STEPS 96u
typedef struct { double x,y; } MostowPoint2;
typedef struct { double a,b,c,d; } MostowBoundaryMap;
typedef enum {
    MOSTOW_IDENTITY, MOSTOW_ELLIPTIC, MOSTOW_PARABOLIC, MOSTOW_HYPERBOLIC,
    MOSTOW_SINGULAR_MAP, MOSTOW_REVERSED_MAP, MOSTOW_UNCERTAIN_MAP,
    MOSTOW_NONFINITE_MAP
} MostowMapClass;
typedef struct { int32_t face; double weights[3]; } MostowAnchor;
typedef struct {
    uint32_t schema,seed;
    char source[65],fixture[65];
    MostowView camera;
    double time,angle,boost,shear,view_turn,view_shift;
    float light[3];
    int32_t encounter,model,illustrated,paused,rings,mesh,normal,information;
    int32_t selected_string,preset,control_page,map_parameter;
    MostowAnchor anchor;
    MostowM004Experiment rigidity;
    MostowOctagonNavigator quotient;
    MostowPantsParameters pants;
    double distortion_amount,coarse_position;
    int32_t mechanical_parameter,distortion_preset,distortion_metric,lesson_page,navigation_error;
} MostowScene;
typedef struct {
    uint32_t identity;
    MostowPoint2 first,second;
    int degenerate,diameter;
} MostowString;
void mostow_scene_init(MostowScene *scene);
int mostow_scene_valid(const MostowScene *scene);
int mostow_scene_write(const MostowScene *scene,const char *path);
int mostow_scene_read(MostowScene *scene,const char *path);
MostowBoundaryMap mostow_boundary_map(double angle,double boost,double shear);
MostowMapClass mostow_map_classify(MostowBoundaryMap map);
const char *mostow_map_class_name(MostowMapClass classification);
int mostow_boundary_apply(MostowBoundaryMap map,MostowPoint2 input,MostowPoint2 *output);
MostowPoint2 mostow_klein_to_poincare(MostowPoint2 point);
MostowPoint2 mostow_poincare_to_klein(MostowPoint2 point);
int mostow_string(const MostowScene *scene,uint32_t identity,MostowString *string);
MostowPoint2 mostow_string_point(const MostowString *string,int model,double fraction);
int mostow_geodesic_circle(const MostowString *string,MostowPoint2 *center,double *radius);
int mostow_anchor_sample(const MostowSheet *sheet,const MostowRenderVertex *vertices,
                        MostowAnchor anchor,float position[3],float normal[3]);
int mostow_project(const float matrix[16],const float point[3],int width,int height,float screen[3]);
int mostow_sheet_pick(const MostowSheet *sheet,const MostowRenderVertex *vertices,
                      const MostowView *view,int width,int height,float x,float y,MostowAnchor *anchor);
/* Shared phone geometry. Pages contain six controls; drag controls are 20..23. */
void mostow_scene_control_rect(int width,int height,int slot,float rect[4]);
int mostow_scene_control_hit(const MostowScene *scene,int width,int height,float x,float y);
const char *mostow_scene_control_label(const MostowScene *scene,int slot);
void mostow_scene_control(MostowScene *scene,int control,float fraction);
MostowDifferential2 mostow_scene_differential(const MostowScene *scene);
void mostow_scene_tangent_metrics(const MostowScene *scene,MostowTangentMetric *source,
                                 MostowTangentMetric *target);
MostowDistortionDiagnostics mostow_scene_distortion(const MostowScene *scene);
#endif
