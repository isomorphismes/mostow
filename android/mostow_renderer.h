#ifndef MOSTOW_RENDERER_H
#define MOSTOW_RENDERER_H
#include "mostow_view.h"
#include "mostow_scene.h"
int mostow_renderer_start(int width,int height);
void mostow_renderer_stop(void);
void mostow_renderer_resize(int width,int height);
int mostow_renderer_draw(const MostowView *view,double seconds,int paused,int rings,int information);
int mostow_renderer_scene(MostowScene *scene,const char *message);
int mostow_renderer_pick(MostowScene *scene,float x,float y);
int mostow_renderer_export(const MostowScene *scene,const char *image_path,const char *scene_path);
#endif
