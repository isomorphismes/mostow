/* Real GLES2 rendering in a surfaceless host EGL context, not phone evidence. */
#include "mostow_renderer.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv)
{
    const int width=576,height=1152;
    double seconds=argc>2?strtod(argv[2],NULL):0.0;
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
    if(!mostow_renderer_draw(&view,seconds,0,1,1)){fprintf(stderr,"DRAW_FAILED\n");return 1;}
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
