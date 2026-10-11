#ifndef MOSTOW_VIEW_H
#define MOSTOW_VIEW_H
#include <stdint.h>
typedef struct {
    float yaw, pitch, distance;
    int32_t primary_id, secondary_id;
    float last_x, last_y, pinch_distance;
} MostowView;
void mostow_view_init(MostowView *view);
void mostow_view_release(MostowView *view);
void mostow_view_orbit(MostowView *view, float x, float y, int short_side);
void mostow_view_zoom(MostowView *view, float pinch_distance);
void mostow_view_matrix(const MostowView *view, int width, int height,
                        float matrix[16], float rotation[9]);

/* One coordinate source for rendered controls and NativeActivity hit testing. */
typedef struct {
    float center_y;
    float touch_top;
    float touch_bottom;
} MostowControlBand;
MostowControlBand mostow_controls_layout(int width, int height);
/* 0 is no hit, then 1 pause, 2 rings, 3 information. */
int mostow_control_hit(int width, int height, float x, float y);
#endif
