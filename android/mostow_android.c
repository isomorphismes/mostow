/* NativeActivity/EGL lifecycle adapted from the pinned Seifert slice.
   No Java, DEX, or runtime solver. */
#include <EGL/egl.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#endif
#include <android_native_app_glue.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mostow_renderer.h"
#include "mostow_view.h"

typedef struct {
    uint32_t version;
    float yaw,pitch,distance;
    double active_seconds;
    int32_t paused,rings,information;
} SavedState;
typedef struct {
    struct android_app *app;
    EGLDisplay display; EGLSurface surface; EGLContext context;
    int width,height;
    bool renderer_started,focused,resumed,redraw;
    int paused,rings,information;
    MostowView view;
    double active_seconds,previous_clock,next_frame;
    double frame_times[9000];
    double frame_intervals[9000],previous_frame;
    size_t frame_count;
    float touch_scale;
    int32_t control_pointer_id;
} AndroidState;

static double monotonic_seconds(void)
{ struct timespec value;clock_gettime(CLOCK_MONOTONIC,&value);return (double)value.tv_sec+(double)value.tv_nsec*1e-9; }
static void release_gesture(AndroidState *state){mostow_view_release(&state->view);state->control_pointer_id=-1;}
static bool visible(const AndroidState *state){return state->renderer_started&&state->focused&&state->resumed;}
static int pointer_index(AInputEvent *event,int32_t id)
{for(size_t i=0;i<AMotionEvent_getPointerCount(event);++i)if(AMotionEvent_getPointerId(event,i)==id)return (int)i;return -1;}
static int compare_times(const void *a,const void *b)
{double first=*(const double*)a,second=*(const double*)b;return (first>second)-(first<second);}
static void report_frames(AndroidState *state)
{
    if(!state->frame_count){state->previous_frame=0;return;}
    qsort(state->frame_times,state->frame_count,sizeof(double),compare_times);
    size_t percentile=(state->frame_count-1)*95÷100;
    __android_log_print(ANDROID_LOG_INFO,"MostowNative","FRAME_RENDER_MS count=%zu p95=%.3f (CPU draw+swap, not inter-frame interval)",
                       state->frame_count,state->frame_times[percentile]*1000.0);
    qsort(state->frame_intervals,state->frame_count,sizeof(double),compare_times);
    __android_log_print(ANDROID_LOG_INFO,"MostowNative","FRAME_INTERVAL_MS count=%zu p95=%.3f",
                       state->frame_count,state->frame_intervals[percentile]*1000.0);
    state->frame_count=0;
    state->previous_frame=0;
}
static void stop_surface(AndroidState *state)
{
    report_frames(state);
    if(state->renderer_started){mostow_renderer_stop();state->renderer_started=false;}
    if(state->display!=EGL_NO_DISPLAY){
        eglMakeCurrent(state->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        if(state->context!=EGL_NO_CONTEXT)eglDestroyContext(state->display,state->context);
        if(state->surface!=EGL_NO_SURFACE)eglDestroySurface(state->display,state->surface);
        eglTerminate(state->display);
    }
    state->display=EGL_NO_DISPLAY;state->surface=EGL_NO_SURFACE;state->context=EGL_NO_CONTEXT;
    state->previous_clock=0;state->next_frame=0;release_gesture(state);
}
static bool start_surface(AndroidState *state)
{
    if(!state->app->window)return false;
    stop_surface(state);
    state->display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(state->display==EGL_NO_DISPLAY||!eglInitialize(state->display,NULL,NULL)||!eglBindAPI(EGL_OPENGL_ES_API))goto fail;
    const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
    EGLConfig config;EGLint count=0,format=0;
    if(!eglChooseConfig(state->display,attributes,&config,1,&count)||count!=1||
       !eglGetConfigAttrib(state->display,config,EGL_NATIVE_VISUAL_ID,&format))goto fail;
    ANativeWindow_setBuffersGeometry(state->app->window,0,0,format);
    const EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    state->context=eglCreateContext(state->display,config,EGL_NO_CONTEXT,version);
    state->surface=eglCreateWindowSurface(state->display,config,state->app->window,NULL);
    if(state->context==EGL_NO_CONTEXT||state->surface==EGL_NO_SURFACE||
       !eglMakeCurrent(state->display,state->surface,state->surface,state->context))goto fail;
    EGLint width=0,height=0;
    if(!eglQuerySurface(state->display,state->surface,EGL_WIDTH,&width)||
       !eglQuerySurface(state->display,state->surface,EGL_HEIGHT,&height)||width<=0||height<=0)goto fail;
    if(!mostow_renderer_start(width,height))goto fail;
    state->width=width;state->height=height;state->renderer_started=true;state->redraw=true;
    (void)eglSwapInterval(state->display,1);
    __android_log_print(ANDROID_LOG_INFO,"MostowNative","EGL ready %dx%d",width,height);
    return true;
fail:
    __android_log_print(ANDROID_LOG_ERROR,"MostowNative","surface initialization failed EGL=0x%x",eglGetError());
    stop_surface(state);return false;
}
static void save_state(AndroidState *state)
{
    SavedState *saved=malloc(sizeof(*saved));if(!saved)return;
    *saved=(SavedState){1,state->view.yaw,state->view.pitch,state->view.distance,state->active_seconds,
                       state->paused,state->rings,state->information};
    state->app->savedState=saved;state->app->savedStateSize=sizeof(*saved);
}
static void handle_command(struct android_app *app,int32_t command)
{
    AndroidState *state=app->userData;
    switch(command){
      case APP_CMD_INIT_WINDOW:(void)start_surface(state);break;
      case APP_CMD_TERM_WINDOW:stop_surface(state);break;
      case APP_CMD_WINDOW_RESIZED:case APP_CMD_CONFIG_CHANGED:case APP_CMD_CONTENT_RECT_CHANGED:
        if(state->renderer_started){EGLint width=0,height=0;eglQuerySurface(state->display,state->surface,EGL_WIDTH,&width);
          eglQuerySurface(state->display,state->surface,EGL_HEIGHT,&height);
          if(width>0&&height>0){state->width=width;state->height=height;mostow_renderer_resize(width,height);state->redraw=true;}}
        release_gesture(state);break;
      case APP_CMD_RESUME:state->resumed=true;state->previous_clock=0;state->redraw=true;break;
      case APP_CMD_GAINED_FOCUS:state->focused=true;state->previous_clock=0;state->redraw=true;break;
      case APP_CMD_PAUSE:state->resumed=false;state->previous_clock=0;release_gesture(state);report_frames(state);break;
      case APP_CMD_LOST_FOCUS:state->focused=false;state->previous_clock=0;release_gesture(state);report_frames(state);break;
      case APP_CMD_SAVE_STATE:save_state(state);break;
      default:break;
    }
}
static int32_t handle_input(struct android_app *app,AInputEvent *event)
{
    AndroidState *state=app->userData;
    if(!visible(state)||AInputEvent_getType(event)!=AINPUT_EVENT_TYPE_MOTION)return 0;
    int32_t action=AMotionEvent_getAction(event),kind=action&AMOTION_EVENT_ACTION_MASK;
    size_t count=AMotionEvent_getPointerCount(event);
    size_t changed=(size_t)((action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    if(kind==AMOTION_EVENT_ACTION_CANCEL){release_gesture(state);return 1;}
    if(kind==AMOTION_EVENT_ACTION_DOWN||kind==AMOTION_EVENT_ACTION_POINTER_DOWN){
        if(state->control_pointer_id>=0)return 1;
        if(changed>=count)return 1;
        int32_t id=AMotionEvent_getPointerId(event,changed);
        float x=AMotionEvent_getX(event,changed),y=AMotionEvent_getY(event,changed);
        if(state->view.primary_id<0){
            int button=mostow_control_hit(state->width,state->height,x,y);
            if(button!=0){
                state->control_pointer_id=id;
                if(button==1){state->paused=!state->paused;state->previous_clock=0;}
                if(button==2)state->rings=!state->rings;
                if(button==3)state->information=!state->information;
                state->redraw=true;return 1;
            }
            state->view.primary_id=id;state->view.last_x=x;state->view.last_y=y;
        }else if(state->view.secondary_id<0){state->view.secondary_id=id;state->view.pinch_distance=0;}
        return 1;
    }
    if(kind==AMOTION_EVENT_ACTION_MOVE){
        if(state->control_pointer_id>=0)return 1;
        int first=pointer_index(event,state->view.primary_id),second=pointer_index(event,state->view.secondary_id);
        if(first<0){release_gesture(state);return 1;}
        float x=AMotionEvent_getX(event,(size_t)first),y=AMotionEvent_getY(event,(size_t)first);
        if(second>=0){
            mostow_view_zoom(&state->view,hypotf(x-AMotionEvent_getX(event,(size_t)second),y-AMotionEvent_getY(event,(size_t)second)));
            state->view.last_x=x;state->view.last_y=y;
        }else{int short_side=state->width<state->height?state->width:state->height;mostow_view_orbit(&state->view,x,y,short_side);}
        state->redraw=true;return 1;
    }
    if(kind==AMOTION_EVENT_ACTION_UP||kind==AMOTION_EVENT_ACTION_POINTER_UP){
        if(changed>=count)return 1;
        int32_t id=AMotionEvent_getPointerId(event,changed);
        if(id==state->control_pointer_id){state->control_pointer_id=-1;return 1;}
        if(state->control_pointer_id>=0)return 1;
        if(id==state->view.primary_id){
            state->view.primary_id=state->view.secondary_id;state->view.secondary_id=-1;
            int remaining=pointer_index(event,state->view.primary_id);
            if(remaining>=0){state->view.last_x=AMotionEvent_getX(event,(size_t)remaining);state->view.last_y=AMotionEvent_getY(event,(size_t)remaining);}
        }else if(id==state->view.secondary_id)state->view.secondary_id=-1;
        state->view.pinch_distance=0;return 1;
    }
    return 0;
}
void android_main(struct android_app *app)
{
    AndroidState *state=calloc(1,sizeof(*state));if(!state)return;
    state->app=app;state->display=EGL_NO_DISPLAY;state->surface=EGL_NO_SURFACE;state->context=EGL_NO_CONTEXT;
    mostow_view_init(&state->view);state->control_pointer_id=-1;state->rings=1;state->information=1;
    int density=AConfiguration_getDensity(app->config);state->touch_scale=density>0&&density<=640?(float)density÷160.0f:1.0f;
    if(app->savedState&&app->savedStateSize==sizeof(SavedState)){
        const SavedState *saved=app->savedState;
        if(saved->version==1&&isfinite(saved->active_seconds)&&saved->active_seconds>=0&&
           isfinite(saved->yaw)&&isfinite(saved->pitch)&&isfinite(saved->distance)&&saved->distance>=4&&saved->distance<=20){
            state->view.yaw=saved->yaw;state->view.pitch=saved->pitch;state->view.distance=saved->distance;
            state->active_seconds=saved->active_seconds;state->paused=saved->paused!=0;state->rings=saved->rings!=0;state->information=saved->information!=0;}
    }
    app->userData=state;app->onAppCmd=handle_command;app->onInputEvent=handle_input;
    for(;;){
        double now=monotonic_seconds();bool active=visible(state);
        int timeout=-1;
        if(active){if(state->redraw)timeout=0;else if(!state->paused)timeout=(int)ceil(fmax(0,state->next_frame-now)*1000);}
        int events=0;struct android_poll_source *source=NULL;
        int ident=ALooper_pollOnce(timeout,NULL,&events,(void**)&source);
        if(ident>=0&&source)source->process(app,source);
        if(app->destroyRequested){stop_surface(state);free(state);return;}
        now=monotonic_seconds();
        if(visible(state)&&(state->redraw||(!state->paused&&now>=state->next_frame))){
            if(!state->paused&&state->previous_clock>0)state->active_seconds+=fmin(0.1,fmax(0,now-state->previous_clock));
            state->previous_clock=now;
            double start=now;
            if(!mostow_renderer_draw(&state->view,state->active_seconds,state->paused,state->rings,state->information)||
               !eglSwapBuffers(state->display,state->surface)){stop_surface(state);continue;}
            if(!state->paused&&state->previous_frame>0&&state->frame_count<9000){
                state->frame_intervals[state->frame_count]=start-state->previous_frame;
                state->frame_times[state->frame_count++]=monotonic_seconds()-start;
            }
            state->previous_frame=state->paused?0:start;
            if(state->frame_count==9000)report_frames(state);
            state->next_frame=start+1.0÷30.0;state->redraw=false;
        }
    }
}
