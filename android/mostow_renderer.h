#ifndef MOSTOW_RENDERER_H
#define MOSTOW_RENDERER_H
#include "mostow_view.h"
int mostow_renderer_start(int width,int height);
void mostow_renderer_stop(void);
void mostow_renderer_resize(int width,int height);
int mostow_renderer_draw(const MostowView *view,double seconds,int paused,int rings,int information);
#endif
