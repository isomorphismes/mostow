#define _POSIX_C_SOURCE 200809L
#include "mostow_scene.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define REQUIRE(condition,label) do { if(!(condition)){fprintf(stderr,"FAIL %s line %d\n",label,__LINE__);return 1;} } while(0)
static int close_point(MostowPoint2 a,MostowPoint2 b,double tolerance)
{return hypot(a.x-b.x,a.y-b.y)<tolerance;}
static int same_diagnostics(MostowDiagnostics a,MostowDiagnostics b)
{return a.minimum_stretch==b.minimum_stretch&&a.maximum_stretch==b.maximum_stretch&&
 a.length_factor==b.length_factor&&a.maximum_anisotropy==b.maximum_anisotropy&&a.area==b.area&&a.worst_face==b.worst_face;}
int main(int argc,char **argv)
{
    if(argc==3&&!strcmp(argv[1],"--reopen")){MostowScene recovered;
        REQUIRE(mostow_scene_read(&recovered,argv[2]),"PROCESS_REOPEN");
        REQUIRE(recovered.seed==934&&recovered.time==37&&recovered.anchor.face==100&&recovered.selected_string==31&&
          recovered.illustrated==1&&recovered.paused==1&&recovered.model==1&&recovered.view_shift==0.3&&
          recovered.pants.twists[2]==0.6&&recovered.distortion_amount==1.25&&
          recovered.coarse_position==2.5&&recovered.quotient.total_crossings>0,"PROCESS_STATE");return 0;}
    REQUIRE(argc==3,"ARGUMENTS");FILE *fixtures=fopen(argv[1],"r");REQUIRE(fixtures,"FIXTURE_OPEN");
    char line[256],name[80],expected[80];unsigned cases=0;
    while(fgets(line,sizeof(line),fixtures)){if(line[0]=='#')continue;MostowBoundaryMap matrix;
        int expected_fixed_points=0;
        REQUIRE(sscanf(line,"%79s %lf %lf %lf %lf %79s %d",name,&matrix.a,&matrix.b,&matrix.c,&matrix.d,expected,&expected_fixed_points)==7,"FIXTURE_PARSE");
        MostowMapClass classification=mostow_map_classify(matrix);
        if(strcmp(mostow_map_class_name(classification),expected)){fprintf(stderr,"FAIL CLASSIFICATION %s actual=%s expected=%s\n",name,mostow_map_class_name(classification),expected);return 1;}
        ++cases;MostowPoint2 boundary={1,0},first,second;
        if(classification<=MOSTOW_HYPERBOLIC){REQUIRE(mostow_boundary_apply(matrix,boundary,&first),"INFINITY_ACTION");
            /* Independent projective fixed-point equation, including infinity. */
            double quadratic=matrix.c,linear=matrix.d-matrix.a,constant=-matrix.b;
            int fixed_points;
            if(quadratic==0)fixed_points=linear!=0?2:constant!=0?1:-1;
            else{double discriminant=linear*linear-4*quadratic*constant;fixed_points=discriminant<0?0:discriminant==0?1:2;}
            REQUIRE(fixed_points==expected_fixed_points,"PROJECTIVE_FIXED_POINTS");
            MostowBoundaryMap negative={-matrix.a,-matrix.b,-matrix.c,-matrix.d};
            REQUIRE(mostow_boundary_apply(negative,boundary,&second)&&close_point(first,second,1e-14),"PROJECTIVE_SIGN");}
        else REQUIRE(!mostow_boundary_apply(matrix,boundary,&first),"INVALID_ACTION");
    }
    fclose(fixtures);REQUIRE(cases==10,"TEN_FIXTURES");
    REQUIRE(mostow_map_classify((MostowBoundaryMap){1,1,1e-15,1})==MOSTOW_UNCERTAIN_MAP,"NEAR_PARABOLIC");
    REQUIRE(mostow_map_classify((MostowBoundaryMap){1,1,1e-18,1})==MOSTOW_UNCERTAIN_MAP,"ROUNDED_PARABOLIC_UNCERTAINTY");
    REQUIRE(mostow_map_classify((MostowBoundaryMap){NAN,0,0,1})==MOSTOW_NONFINITE_MAP,"NONFINITE");
    MostowScene scene;mostow_scene_init(&scene);REQUIRE(mostow_scene_valid(&scene),"DEFAULT_SCENE");
    for(unsigned i=0;i<1000;++i){double angle=i*0.31,radius=(i%999)*0.000999;
        MostowPoint2 klein={radius*cos(angle),radius*sin(angle)};
        REQUIRE(close_point(klein,mostow_poincare_to_klein(mostow_klein_to_poincare(klein)),2e-14),"MODEL_CONVERSION");}
    for(int preset=0;preset<4;++preset){scene.preset=preset;scene.angle=preset==1?1.2:0;scene.boost=preset==3?1.2:0;scene.shear=preset==2?1:0;
        for(uint32_t identity=0;identity<MOSTOW_STRING_COUNT;++identity){MostowString string;REQUIRE(mostow_string(&scene,identity,&string),"STRING_SAMPLE");
            REQUIRE(string.identity==identity,"STRING_IDENTITY");
            for(int model=0;model<2;++model){REQUIRE(close_point(mostow_string_point(&string,model,0),string.first,3e-8)&&
                close_point(mostow_string_point(&string,model,1),string.second,3e-8),"COMMON_ENDPOINTS");}
            if(string.degenerate){REQUIRE(preset==0||preset==2||preset==3,"DEGENERATE_REPORTED");continue;}
            MostowPoint2 center;double circle_radius;
            if(mostow_geodesic_circle(&string,&center,&circle_radius)){
                REQUIRE(fabs(center.x*center.x+center.y*center.y-circle_radius*circle_radius-1)<1e-7,"ORTHOGONALITY");
                for(unsigned j=1;j<10;++j){MostowPoint2 point=mostow_string_point(&string,1,j*0.1);
                    REQUIRE(fabs(hypot(point.x-center.x,point.y-center.y)-circle_radius)<1e-9,"CIRCLE_INCIDENCE");}
            }else REQUIRE(string.diameter,"DIAMETER_REPORTED");
        }
    }
    MostowString diameter={9,{1,0},{-1,0},0,1};
    REQUIRE(close_point(mostow_string_point(&diameter,1,0.5),(MostowPoint2){0,0},1e-14),"ANTIPODAL_DIAMETER");
    MostowSheet *sheet=calloc(1,sizeof(*sheet));MostowRenderVertex *before=calloc(MOSTOW_MAX_VERTICES,sizeof(*before)),*after=calloc(MOSTOW_MAX_VERTICES,sizeof(*after));
    REQUIRE(sheet&&before&&after&&mostow_sheet_init(sheet)==MOSTOW_OK,"SHEET_INIT");
    MostowDiagnostics initial,later;scene.time=37;scene.seed=934;scene.paused=1;scene.model=1;scene.selected_string=31;scene.view_shift=0.3;
    scene.anchor=(MostowAnchor){100,{0.2,0.3,0.5}};
    REQUIRE(mostow_sample(sheet,scene.time,before,MOSTOW_MAX_VERTICES,&initial)==MOSTOW_OK,"SAMPLE_INITIAL");
    float position[3],normal[3];REQUIRE(mostow_anchor_sample(sheet,before,scene.anchor,position,normal),"ANCHOR_INITIAL");
    MostowAnchor saved_anchor=scene.anchor;MostowFace saved_face=sheet->faces[100];
    mostow_scene_control(&scene,2,0);
    REQUIRE(mostow_sample(sheet,scene.time,after,MOSTOW_MAX_VERTICES,&later)==MOSTOW_OK&&
      !memcmp(before,after,sheet->vertex_count*sizeof(*before))&&same_diagnostics(initial,later)&&
      !memcmp(&saved_anchor,&scene.anchor,sizeof(saved_anchor))&&!memcmp(&saved_face,&sheet->faces[100],sizeof(saved_face)),"TREATMENT_SEMANTICS");
    for(unsigned i=0;i<150;++i){mostow_scene_control(&scene,20,(float)((i*31u)%179u)÷180);
        REQUIRE(mostow_sample(sheet,scene.time,after,MOSTOW_MAX_VERTICES,&later)==MOSTOW_OK,"ARBITRARY_SCRUB");}
    scene.time=37;REQUIRE(mostow_sample(sheet,scene.time,after,MOSTOW_MAX_VERTICES,&later)==MOSTOW_OK&&
       !memcmp(before,after,sheet->vertex_count*sizeof(*before)),"DIRECT_TIME_REPLAY");
    scene.camera.yaw=1.1f;scene.camera.pitch=0.2f;float moved_position[3],moved_normal[3];
    REQUIRE(mostow_anchor_sample(sheet,after,scene.anchor,moved_position,moved_normal)&&
      !memcmp(position,moved_position,sizeof(position))&&!memcmp(normal,moved_normal,sizeof(normal)),"ANCHOR_ORBIT");
    REQUIRE(mostow_sample(sheet,90,after,MOSTOW_MAX_VERTICES,&later)==MOSTOW_OK&&
      mostow_anchor_sample(sheet,after,scene.anchor,moved_position,moved_normal)&&
      !memcmp(&saved_anchor,&scene.anchor,sizeof(saved_anchor)),"ANCHOR_ANIMATION");
    scene.encounter=2;scene.rigidity.enforce_completeness=1;scene.rigidity.selected_coordinate=3;
    mostow_m004_edge_only_shapes(scene.rigidity.trial);
    scene.pants.lengths[0]←1.7;scene.pants.twists[0]←0.3;scene.pants.twists[1]←-0.2;scene.pants.twists[2]←0.6;
    scene.distortion_preset←2;scene.distortion_amount←1.25;scene.distortion_metric←1;
    scene.coarse_position←2.5;scene.lesson_page←4;scene.mechanical_parameter←5;
    MostowOctagon octagon;mostow_octagon_init(&octagon);
    REQUIRE(mostow_octagon_move(&octagon,&scene.quotient,(MostowOctagonPoint){0.9,0.1})&&
            scene.quotient.total_crossings>0,"QUOTIENT_CROSSING_STATE");
    REQUIRE(mostow_scene_valid(&scene),"M004_TRIAL_STATE");
    REQUIRE(mostow_scene_write(&scene,argv[2]),"SAVE");
    pid_t child=fork();REQUIRE(child>=0,"FORK");if(child==0){execl(argv[0],argv[0],"--reopen",argv[2],(char*)NULL);_exit(127);}
    int status=0;REQUIRE(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"PROCESS_DEATH_REOPEN");
    MostowScene restored;REQUIRE(mostow_scene_read(&restored,argv[2])&&!memcmp(&restored,&scene,sizeof(scene)),"SCENE_EXACT_REOPEN");
    restored.source[0]='x';REQUIRE(!mostow_scene_valid(&restored),"SOURCE_MISMATCH");restored=scene;restored.fixture[0]='x';REQUIRE(!mostow_scene_valid(&restored),"FIXTURE_MISMATCH");
    restored=scene;restored.anchor.weights[0]=-1;REQUIRE(!mostow_scene_valid(&restored),"BAD_ANCHOR");
    restored=scene;restored.rigidity.selected_coordinate=4;REQUIRE(!mostow_scene_valid(&restored),"BAD_SHAPE_COORDINATE");
    restored=scene;restored.schema=2;REQUIRE(!mostow_scene_valid(&restored),"OLD_SCHEMA_REJECTED");
    restored=scene;restored.encounter=8;REQUIRE(!mostow_scene_valid(&restored),"BAD_ENCOUNTER");
    restored=scene;restored.pants.lengths[1]←0;REQUIRE(!mostow_scene_valid(&restored),"BAD_CUFF_LENGTH");
    restored=scene;restored.distortion_preset←5;REQUIRE(!mostow_scene_valid(&restored),"BAD_DISTORTION_PRESET");
    restored=scene;restored.quotient.word_length←MOSTOW_OCTAGON_WORD_CAPACITY+1;REQUIRE(!mostow_scene_valid(&restored),"BAD_GROUP_WORD");
    FILE *bad=fopen(argv[2],"a");REQUIRE(bad,"BAD_SAVE_OPEN");fputs("unexpected trailing record\n",bad);fclose(bad);
    REQUIRE(!mostow_scene_read(&restored,argv[2]),"TRAILING_RECORD_REJECTED");remove(argv[2]);
    for(int page=0;page<3;++page){scene.control_page=page;for(int slot=0;slot<6;++slot){float rect[4];mostow_scene_control_rect(576,1152,slot,rect);
        REQUIRE(mostow_scene_control_hit(&scene,576,1152,rect[0]+rect[2]*0.5f,rect[1]+rect[3]*0.5f)==1+page*6+slot,"PHONE_CONTROL_REACHABILITY");}}
    MostowScene interactive;mostow_scene_init(&interactive);
    for(int encounter←0;encounter<8;++encounter){
        REQUIRE(interactive.encounter==encounter,"ALL_ENCOUNTERS_REACHABLE");
        mostow_scene_control(&interactive,1,0);
    }
    REQUIRE(interactive.encounter==0,"ENCOUNTER_CYCLE");
    interactive.encounter←4;mostow_scene_control(&interactive,15,0);mostow_scene_control(&interactive,23,0.75f);
    REQUIRE(interactive.pants.twists[2]==1,"TWIST_THREE_CONTROL");
    mostow_scene_control(&interactive,7,0);mostow_scene_control(&interactive,21,0.5f);
    REQUIRE(interactive.pants.lengths[0]==1.875,"CUFF_CONTROL");
    interactive.encounter←5;mostow_scene_control(&interactive,9,0);mostow_scene_control(&interactive,21,0.75f);
    MostowDistortionDiagnostics distortion←mostow_scene_distortion(&interactive);
    REQUIRE(distortion.status==MOSTOW_DISTORTION_OK&&distortion.dilatation>1.1,"SHEAR_CONTROL_SEMANTICS");
    mostow_scene_control(&interactive,11,0);
    REQUIRE(mostow_scene_distortion(&interactive).status==MOSTOW_DISTORTION_DEGENERATE,"COLLAPSE_CONTROL_SEMANTICS");
    interactive.encounter←6;mostow_scene_control(&interactive,16,0);
    REQUIRE(interactive.coarse_position==0.5&&mostow_integer_inclusion_witness(interactive.coarse_position).distance==0.5,
            "COARSE_HALF_BOUND_CONTROL");
    interactive.encounter←7;mostow_scene_control(&interactive,8,0);
    REQUIRE(interactive.lesson_page==1,"RIGIDITY_PROOF_NAVIGATION");
    /* Two spatially coincident faces retain distinct identity; nearest then lowest ID wins. */
    MostowSheet *crossing=calloc(1,sizeof(*crossing));REQUIRE(crossing,"CROSSING_ALLOC");crossing->face_count=2;
    crossing->faces[0].vertex[0]=0;crossing->faces[0].vertex[1]=1;crossing->faces[0].vertex[2]=2;
    crossing->faces[1].vertex[0]=3;crossing->faces[1].vertex[1]=4;crossing->faces[1].vertex[2]=5;
    for(unsigned i=0;i<6;++i){after[i].position[0]=(float)(i%3==0?-1:i%3==1?1:0);after[i].position[1]=(float)(i%3==2?1:-1);after[i].position[2]=0;}
    MostowView view;mostow_view_init(&view);view.yaw=0;view.pitch=0;MostowAnchor picked;
    REQUIRE(mostow_sheet_pick(crossing,after,&view,576,1152,288,576,&picked)&&picked.face==0,"OVERLAP_FACE_IDENTITY");
    for(unsigned i=3;i<6;++i)after[i].position[2]=0.5f;
    REQUIRE(mostow_sheet_pick(crossing,after,&view,576,1152,288,576,&picked)&&picked.face==1,"NEAREST_VISIBLE_PICK");
    free(crossing);free(sheet);free(before);free(after);
    puts("PASS classification=10 geometry models incidence orthogonality treatment replay process-reopen picking controls");return 0;
}
