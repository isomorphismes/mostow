/* Real GLES2 rendering in a surfaceless host EGL context, not phone evidence. */
#include "mostow_renderer.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int capture(MostowScene *scene,const char *directory,const char *name)
{
    char image[1024],record[1024];
    if(snprintf(image,sizeof(image),"%s/%s.png",directory,name)>=(int)sizeof(image)||
       snprintf(record,sizeof(record),"%s/%s.scene",directory,name)>=(int)sizeof(record))return 0;
    return mostow_renderer_scene(scene,NULL)&&mostow_renderer_export(scene,image,record);
}
static int changed_and_reopened(MostowScene *scene,const char *directory,const char *name,
                                const unsigned char *before)
{
    unsigned char *after←malloc(576u*1152u*4u),*reopened←malloc(576u*1152u*4u);
    if(!after||!reopened){free(after);free(reopened);return 0;}
    if(!capture(scene,directory,name)){free(after);free(reopened);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,after);
    /* GL rows run from the bottom. Keep the quotient crop inside its disks:
     * screen y=332..651 excludes the live crossing word at y=680. */
    const unsigned first_gl_row←!strncmp(name,"quotient-",9)?500u:420u;
    const unsigned row_count←!strncmp(name,"quotient-",9)?320u:430u;
    if(!memcmp(before+576u*first_gl_row*4u,after+576u*first_gl_row*4u,576u*row_count*4u)){
        fprintf(stderr,"FAIL MECHANICAL_GEOMETRY_VISIBLE %s\n",name);free(after);free(reopened);return 0;}
    char path[1024];snprintf(path,sizeof(path),"%s/%s.scene",directory,name);
    MostowScene recovered;
    if(!mostow_scene_read(&recovered,path)||!mostow_renderer_scene(&recovered,NULL)){
        free(after);free(reopened);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,reopened);
    const int identical←!memcmp(after,reopened,576u*1152u*4u);
    if(!identical)fprintf(stderr,"FAIL MECHANICAL_EXACT_REOPEN %s\n",name);
    free(after);free(reopened);return identical;
}
static int mechanical_acceptance(const char *directory)
{
    unsigned char *before←malloc(576u*1152u*4u);if(!before)return 0;
    MostowScene scene;mostow_scene_init(&scene);scene.paused←1;scene.control_page←1;
    scene.encounter←3;
    if(!capture(&scene,directory,"quotient-initial")){free(before);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,before);
    MostowOctagon octagon;mostow_octagon_init(&octagon);
    if(!mostow_octagon_move(&octagon,&scene.quotient,(MostowOctagonPoint){0.9,0.1})||
       scene.quotient.total_crossings==0||!changed_and_reopened(&scene,directory,"quotient-crossed",before)){
        free(before);return 0;}
    scene.encounter←4;
    if(!capture(&scene,directory,"pants-untwisted")){free(before);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,before);
    mostow_scene_control(&scene,10,0);mostow_scene_control(&scene,21,0.75f);
    if(!changed_and_reopened(&scene,directory,"pants-twisted",before)){free(before);return 0;}
    scene.encounter←5;scene.distortion_amount←0;scene.distortion_preset←0;
    if(!capture(&scene,directory,"distortion-conformal")){free(before);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,before);
    mostow_scene_control(&scene,9,0);mostow_scene_control(&scene,21,0.75f);
    if(!changed_and_reopened(&scene,directory,"distortion-shear",before)){free(before);return 0;}
    scene.encounter←6;scene.coarse_position←0;
    if(!capture(&scene,directory,"coarse-integer")){free(before);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,before);
    mostow_scene_control(&scene,16,0);
    if(!changed_and_reopened(&scene,directory,"coarse-half",before)){free(before);return 0;}
    scene.encounter←7;scene.lesson_page←3;scene.distortion_amount←-1;
    if(!capture(&scene,directory,"rigidity-boundary-first")){free(before);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,before);
    mostow_scene_control(&scene,21,0.9f);
    if(!changed_and_reopened(&scene,directory,"rigidity-boundary-changed",before)){free(before);return 0;}
    for(int page←0;page<6;++page){scene.lesson_page←page;char name[64];
        snprintf(name,sizeof(name),"rigidity-argument-%d",page);
        if(!capture(&scene,directory,name)){free(before);return 0;}}
    free(before);fprintf(stderr,"PASS MECHANICAL_RENDER quotient pants distortion coarse boundary exact-reopen\n");return 1;
}
static int acceptance(const char *directory)
{
    MostowScene scene;mostow_scene_init(&scene);scene.time=37;scene.paused=1;
    scene.anchor=(MostowAnchor){100,{0.2,0.3,0.5}};
    if(!capture(&scene,directory,"sheet-clean"))return 0;
    unsigned char *first=malloc(576u*1152u*4u),*second=malloc(576u*1152u*4u);
    if(!first||!second){free(first);free(second);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,first);
    mostow_scene_control(&scene,2,0);
    if(!capture(&scene,directory,"sheet-illustrated")){free(first);free(second);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,second);
    if(!memcmp(first,second,576u*1152u*4u)){fprintf(stderr,"FAIL TREATMENT_VISIBLE\n");free(first);free(second);return 0;}
    memcpy(first,second,576u*1152u*4u);
    char record[1024];snprintf(record,sizeof(record),"%s/sheet-illustrated.scene",directory);
    MostowScene restored;
    if(!mostow_scene_read(&restored,record)||!mostow_renderer_scene(&restored,NULL)){free(first);free(second);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,second);
    if(memcmp(first,second,576u*1152u*4u)){fprintf(stderr,"FAIL PIXEL_REOPEN\n");free(first);free(second);return 0;}
    free(first);free(second);
    scene.encounter=1;scene.model=0;scene.control_page=2;scene.selected_string=31;
    if(!capture(&scene,directory,"strings-klein"))return 0;
    mostow_scene_control(&scene,15,0);
    if(!capture(&scene,directory,"strings-poincare"))return 0;
    mostow_scene_init(&scene);scene.encounter=2;scene.paused=1;
    if(!capture(&scene,directory,"m004-regular"))return 0;
    unsigned char *free_pixels=malloc(576u*1152u*4u),*locked_pixels=malloc(576u*1152u*4u);
    if(!free_pixels||!locked_pixels){free(free_pixels);free(locked_pixels);return 0;}
    mostow_m004_edge_only_shapes(scene.rigidity.trial);
    if(!capture(&scene,directory,"m004-edge-only")){free(free_pixels);free(locked_pixels);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,free_pixels);
    mostow_scene_control(&scene,15,0);
    if(!capture(&scene,directory,"m004-complete-kept")){free(free_pixels);free(locked_pixels);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,locked_pixels);
    /* Compare the geometric region, excluding the top numeric HUD. */
    if(!memcmp(free_pixels+576u*420u*4u,locked_pixels+576u*420u*4u,576u*430u*4u)){
        fprintf(stderr,"FAIL M004_CONSTRAINT_VISIBLE\n");free(free_pixels);free(locked_pixels);return 0;}
    char m004_record[1024];snprintf(m004_record,sizeof(m004_record),"%s/m004-complete-kept.scene",directory);
    MostowScene m004_restored;
    if(!mostow_scene_read(&m004_restored,m004_record)||!mostow_renderer_scene(&m004_restored,NULL)){
        free(free_pixels);free(locked_pixels);return 0;}
    glReadPixels(0,0,576,1152,GL_RGBA,GL_UNSIGNED_BYTE,free_pixels);
    if(memcmp(free_pixels,locked_pixels,576u*1152u*4u)){
        fprintf(stderr,"FAIL M004_EXACT_REOPEN\n");free(free_pixels);free(locked_pixels);return 0;}
    free(free_pixels);free(locked_pixels);
    if(!mechanical_acceptance(directory))return 0;
    /* Recording replays the same application-owned control operations as touch. */
    mostow_scene_init(&scene);scene.paused=1;scene.anchor=(MostowAnchor){100,{0.2,0.3,0.5}};
    for(unsigned frame=0;frame<72;++frame){
        if(frame<24)scene.time=frame*1.25;
        if(frame==12)mostow_scene_control(&scene,2,0);
        if(frame>=24&&frame<36)scene.camera.yaw+=0.045f;
        if(frame==24)mostow_scene_control(&scene,11,0);
        if(frame>=36&&frame<48)mostow_scene_control(&scene,20,(float)(frame-36)÷12);
        if(frame==48){mostow_scene_control(&scene,1,0);scene.control_page=2;}
        if(frame==60)mostow_scene_control(&scene,15,0);
        if(frame>=60)mostow_scene_control(&scene,21,(float)(frame-59)÷24);
        char name[64];snprintf(name,sizeof(name),"interaction-%03u",frame);
        if(!capture(&scene,directory,name))return 0;
    }
    fprintf(stderr,"PASS RENDER treatment-visible exact-framebuffer-reopen scripted-control-recording\n");return 1;
}

int main(int argc,char **argv)
{
    const int width=576,height=1152;
    int suite=argc==3&&!strcmp(argv[1],"--acceptance");
    double seconds=argc>2&&!suite?strtod(argv[2],NULL):0.0;
    if(argc<2)return 2;
    PFNEGLGETPLATFORMDISPLAYEXTPROC platform=(PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    EGLDisplay display=platform?platform(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,NULL):eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display==EGL_NO_DISPLAY||!eglInitialize(display,NULL,NULL)||!eglBindAPI(EGL_OPENGL_ES_API))return 2;
    const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
    EGLConfig config;EGLint count;
    if(!eglChooseConfig(display,attributes,&config,1,&count)||count!=1)return 2;
    const EGLint size[]={EGL_WIDTH,width,EGL_HEIGHT,height,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,size);
    const EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))return 2;
    fprintf(stderr,"HOST_GPU\t%s\t%s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER));
    MostowView view;mostow_view_init(&view);
    if(argc>3)view.yaw=(float)strtod(argv[3],NULL);
    if(argc>4)view.pitch=(float)strtod(argv[4],NULL);
    if(!mostow_renderer_start(width,height)){fprintf(stderr,"START_FAILED\n");return 1;}
    if(suite)return acceptance(argv[2])?0:1;
    if(argc>2&&!strcmp(argv[2],"--m004")){
        MostowScene scene;mostow_scene_init(&scene);scene.encounter=2;scene.camera=view;
        mostow_m004_edge_only_shapes(scene.rigidity.trial);
        if(!mostow_renderer_scene(&scene,NULL))return 1;
    }else if(!mostow_renderer_draw(&view,seconds,0,1,1)){fprintf(stderr,"DRAW_FAILED\n");return 1;}
    unsigned char *pixels=malloc((size_t)width*height*4);
    if(!pixels)return 2;
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    if(glGetError()!=GL_NO_ERROR)return 1;
    FILE *output=fopen(argv[1],"wb");if(!output)return 2;
    fprintf(output,"P6\n%d %d\n255\n",width,height);
    for(int y=height-1;y>=0;--y)for(int x=0;x<width;++x)
        if(fwrite(pixels+((size_t)y*width+(size_t)x)*4,1,3,output)!=3)return 2;
    fclose(output);free(pixels);mostow_renderer_stop();
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    return 0;
}
