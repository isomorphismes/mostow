#include "mostow_renderer.h"
#include "mostow.h"
#include <GLES2/gl2.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __ANDROID__
#include <android/log.h>
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"MostowRenderer",__VA_ARGS__)
#else
#define LOGE(...) fprintf(stderr,__VA_ARGS__)
#endif

/* GLES lifecycle/buffer responsibilities adapted from Seifert's renderer.
 * Mathematical geometry and metric diagnostics remain in the shared kernel. */
static MostowSheet *sheet;
static MostowRenderVertex vertices[MOSTOW_MAX_VERTICES];
static uint16_t indices[MOSTOW_MAX_FACES*3];
static GLuint program,vbo,ebo,intrinsic_vbo,ui_program,ui_vbo;
static GLint matrix_location,rotation_location,rings_location,ui_color,light_location,ink_location,seed_location;
static int width=1,height=1;
static const char *vertex_shader =
 "attribute vec3 a_position; attribute vec3 a_normal; attribute float a_radius;attribute vec2 a_intrinsic;\n"
 "uniform mat4 u_matrix; uniform mat3 u_rotation;\n"
 "varying vec3 v_normal; varying float v_radius;varying vec2 v_intrinsic;\n"
 "void main(){gl_Position=u_matrix*vec4(a_position,1.0);"
 "v_normal=u_rotation*a_normal;v_radius=a_radius;v_intrinsic=a_intrinsic;}\n";
static const char *fragment_shader =
 "precision mediump float; varying vec3 v_normal; varying float v_radius;varying vec2 v_intrinsic;"
 "uniform float u_rings;uniform vec3 u_light;uniform float u_ink;uniform float u_seed;\n"
 "void main(){vec3 n=normalize(v_normal);if(!gl_FrontFacing)n=-n;"
 "float light=0.35+0.60*max(0.0,dot(n,normalize(u_light)));"
 "vec3 material=gl_FrontFacing?vec3(0.79,0.57,0.37):vec3(0.46,0.65,0.67);"
 "float band=1.0-smoothstep(0.0,0.12,abs(sin(v_radius*12.5663706)));"
 "material=mix(material,vec3(0.26,0.32,0.30),band*0.5*u_rings);"
 "vec3 clean=material*light;"
 "float wash=0.5+0.5*sin(v_intrinsic.x*2.7+sin(v_intrinsic.y*3.1)+u_seed);"
 "vec3 pigment=mix(vec3(0.29,0.54,0.53),vec3(0.80,0.43,0.25),wash);"
 "float fibre=sin(v_intrinsic.x*83.0+u_seed)*sin(v_intrinsic.y*71.0-u_seed);"
 "vec3 ink=mix(vec3(0.94,0.89,0.77),pigment,0.48+0.32*light)+fibre*0.018;"
 "float contour=1.0-smoothstep(0.04,0.18,abs(n.z));"
 "ink=mix(ink,vec3(0.12,0.18,0.18),contour*0.88);"
 "ink=mix(ink,vec3(0.20,0.30,0.28),band*0.62*u_rings);"
 "float highlight=pow(max(0.0,dot(n,normalize(normalize(u_light)+vec3(0.0,0.0,1.0)))),18.0);"
 "ink+=vec3(0.10,0.09,0.06)*highlight;"
 "gl_FragColor=vec4(mix(clean,ink,u_ink),1.0);}\n";

static GLuint compile(GLenum type,const char *source)
{
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,NULL); glCompileShader(shader);
    GLint valid=0; glGetShaderiv(shader,GL_COMPILE_STATUS,&valid);
    if(!valid){char log[1024]={0};glGetShaderInfoLog(shader,sizeof(log),NULL,log);LOGE("shader: %s\n",log);glDeleteShader(shader);return 0;}
    return shader;
}
static GLuint link(const char *vs,const char *fs)
{
    GLuint a=compile(GL_VERTEX_SHADER,vs),b=compile(GL_FRAGMENT_SHADER,fs);
    if(!a||!b){if(a)glDeleteShader(a);if(b)glDeleteShader(b);return 0;}
    GLuint p=glCreateProgram();glAttachShader(p,a);glAttachShader(p,b);
    glBindAttribLocation(p,0,"a_position");glBindAttribLocation(p,1,"a_normal");glBindAttribLocation(p,2,"a_radius");
    glBindAttribLocation(p,3,"a_intrinsic");
    glLinkProgram(p);glDeleteShader(a);glDeleteShader(b);
    GLint valid=0;glGetProgramiv(p,GL_LINK_STATUS,&valid);
    if(!valid){glDeleteProgram(p);return 0;}return p;
}
void mostow_renderer_stop(void)
{
    if(vbo)glDeleteBuffers(1,&vbo);
    if(ebo)glDeleteBuffers(1,&ebo);
    if(intrinsic_vbo)glDeleteBuffers(1,&intrinsic_vbo);
    if(ui_vbo)glDeleteBuffers(1,&ui_vbo);
    if(program)glDeleteProgram(program);
    if(ui_program)glDeleteProgram(ui_program);
    vbo=ebo=intrinsic_vbo=ui_vbo=program=ui_program=0;
}
void mostow_renderer_resize(int w,int h){width=w>0?w:1;height=h>0?h:1;}
int mostow_renderer_start(int w,int h)
{
    mostow_renderer_stop();mostow_renderer_resize(w,h);
    if(!sheet){sheet=calloc(1,sizeof(*sheet));if(!sheet)return 0;
        MostowStatus status=mostow_sheet_init(sheet);if(status!=MOSTOW_OK){LOGE("sheet init failed: %d\n",status);free(sheet);sheet=NULL;return 0;}}
    program=link(vertex_shader,fragment_shader);
    ui_program=link("attribute vec3 a_position;void main(){gl_Position=vec4(a_position,1.0);}",
                    "precision mediump float;uniform vec3 u_color;void main(){gl_FragColor=vec4(u_color,1.0);}");
    if(!program||!ui_program){mostow_renderer_stop();return 0;}
    matrix_location=glGetUniformLocation(program,"u_matrix");rotation_location=glGetUniformLocation(program,"u_rotation");
    rings_location=glGetUniformLocation(program,"u_rings");ui_color=glGetUniformLocation(ui_program,"u_color");
    light_location=glGetUniformLocation(program,"u_light");ink_location=glGetUniformLocation(program,"u_ink");seed_location=glGetUniformLocation(program,"u_seed");
    for(uint32_t f=0;f<sheet->face_count;++f)for(uint32_t j=0;j<3;++j)indices[3*f+j]=sheet->faces[f].vertex[j];
    glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glGenBuffers(1,&ui_vbo);glGenBuffers(1,&intrinsic_vbo);
    float intrinsic[MOSTOW_MAX_VERTICES*2];
    for(uint32_t v=0;v<sheet->vertex_count;++v){
        intrinsic[v*2]=(float)(sheet->intrinsic[v].radius*cos(sheet->intrinsic[v].angle));
        intrinsic[v*2+1]=(float)(sheet->intrinsic[v].radius*sin(sheet->intrinsic[v].angle));}
    glBindBuffer(GL_ARRAY_BUFFER,intrinsic_vbo);glBufferData(GL_ARRAY_BUFFER,sheet->vertex_count*2*sizeof(float),intrinsic,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,sheet->face_count*3*sizeof(uint16_t),indices,GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sheet->vertex_count*sizeof(*vertices),NULL,GL_DYNAMIC_DRAW);
    return glGetError()==GL_NO_ERROR;
}

/* Small application-owned bitmap alphabet; no font/runtime dependency. */
typedef struct {char letter;unsigned char rows[7];} Glyph;
static const Glyph glyphs[]={
 {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},{'C',{14,17,16,16,16,17,14}},
 {'D',{30,17,17,17,17,17,30}},{'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,15}},{'H',{17,17,17,31,17,17,17}},{'I',{14,4,4,4,4,4,14}},
 {'J',{7,2,2,2,2,18,12}},{'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},{'O',{14,17,17,17,17,17,14}},
 {'P',{30,17,17,30,16,16,16}},{'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},{'U',{17,17,17,17,17,17,14}},
 {'V',{17,17,17,17,17,10,4}},{'W',{17,17,17,21,21,21,10}},{'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},{'2',{14,17,1,2,4,8,31}},
 {'3',{30,1,1,14,1,1,30}},{'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},{'8',{14,17,17,14,17,17,14}},
 {'9',{14,17,17,15,1,1,14}},{'.',{0,0,0,0,0,12,12}},{':',{0,12,12,0,12,12,0}},
 {'/',{1,2,2,4,8,8,16}},{'-',{0,0,0,31,0,0,0}},{'=',{0,31,0,31,0,0,0}},
 {'<',{1,2,4,8,4,2,1}}
};
static float ui_vertices[120000];
static size_t ui_count;
static void rectangle(float x,float y,float w,float h)
{
    if(ui_count+12>sizeof(ui_vertices)÷sizeof(ui_vertices[0]))return;
    float x0=2*x÷(float)width-1,x1=2*(x+w)÷(float)width-1;
    float y0=1-2*y÷(float)height,y1=1-2*(y+h)÷(float)height;
    const float points[12]={x0,y0,x1,y0,x1,y1,x0,y0,x1,y1,x0,y1};
    memcpy(ui_vertices+ui_count,points,sizeof(points));ui_count+=12;
}
static void text(float x,float y,float scale,const char *value)
{
    for(size_t c=0;value[c];++c){
        const Glyph *glyph=NULL;
        for(size_t g=0;g<sizeof(glyphs)÷sizeof(glyphs[0]);++g)if(glyphs[g].letter==value[c]){glyph=&glyphs[g];break;}
        if(glyph)for(unsigned row=0;row<7;++row)for(unsigned column=0;column<5;++column)
            if(glyph->rows[row]&(1u<<(4-column)))rectangle(x+(float)column*scale,y+(float)row*scale,scale*0.85f,scale*0.85f);
        x+=6*scale;
    }
}
static void ui_flush(float red,float green,float blue)
{
    glDisable(GL_DEPTH_TEST);glUseProgram(ui_program);glUniform3f(ui_color,red,green,blue);
    glBindBuffer(GL_ARRAY_BUFFER,ui_vbo);glBufferData(GL_ARRAY_BUFFER,ui_count*sizeof(float),ui_vertices,GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,NULL);
    glEnableVertexAttribArray(0);glDisableVertexAttribArray(1);glDisableVertexAttribArray(2);glDisableVertexAttribArray(3);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)(ui_count÷2));
}
/* Stroke tessellation is visual only; exact curve samples remain untouched. */
static void stroke(const float *points,unsigned count,float thickness,int depth,float red,float green,float blue)
{
    float triangles[MOSTOW_STRING_STEPS*18+18];unsigned used=0;
    for(unsigned i=1;i<count&&i<=MOSTOW_STRING_STEPS;++i){const float *a=points+(i-1)*3,*b=points+i*3;
        float dx=b[0]-a[0],dy=b[1]-a[1],length=hypotf(dx,dy);if(length<1e-6f)continue;
        float nx=-dy*thickness*0.5f÷length,ny=dx*thickness*0.5f÷length;
        const float quad[18]={a[0]+nx,a[1]+ny,a[2],a[0]-nx,a[1]-ny,a[2],b[0]-nx,b[1]-ny,b[2],
          a[0]+nx,a[1]+ny,a[2],b[0]-nx,b[1]-ny,b[2],b[0]+nx,b[1]+ny,b[2]};
        for(unsigned v=0;v<6;++v){triangles[used++]=2*quad[v*3]÷width-1;triangles[used++]=1-2*quad[v*3+1]÷height;triangles[used++]=quad[v*3+2]-0.00003f;}
    }
    if(depth)glEnable(GL_DEPTH_TEST);else glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);glUseProgram(ui_program);glUniform3f(ui_color,red,green,blue);
    glBindBuffer(GL_ARRAY_BUFFER,ui_vbo);glBufferData(GL_ARRAY_BUFFER,used*sizeof(float),triangles,GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,NULL);
    glEnableVertexAttribArray(0);glDisableVertexAttribArray(1);glDisableVertexAttribArray(2);glDisableVertexAttribArray(3);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)(used÷3));glDepthMask(GL_TRUE);
}
static void disk_layout(const MostowScene *scene,float *cx,float *cy,float *radius)
{*cx=width*0.5f;*cy=height*0.43f;*radius=fminf(width*0.43f,height*0.22f)*9÷scene->camera.distance;}
static void draw_strings(const MostowScene *s)
{
    float cx,cy,radius;disk_layout(s,&cx,&cy,&radius);
    float border[(MOSTOW_STRING_STEPS+1)*3];
    for(unsigned i=0;i<=MOSTOW_STRING_STEPS;++i){double angle=6.283185307179586*i÷MOSTOW_STRING_STEPS;
        border[i*3]=cx+radius*(float)cos(angle);border[i*3+1]=cy-radius*(float)sin(angle);border[i*3+2]=0;}
    stroke(border,MOSTOW_STRING_STEPS+1,2,0,0.42f,0.47f,0.43f);
    for(unsigned pass=0;pass<2;++pass)for(uint32_t id=0;id<MOSTOW_STRING_COUNT;++id){
        int selected=(int32_t)id==s->selected_string;if(selected!=(int)pass)continue;
        MostowString string;if(!mostow_string(s,id,&string))continue;
        if(string.degenerate){ui_count=0;rectangle(cx+radius*(float)string.first.x-2,cy-radius*(float)string.first.y-2,4,4);
            ui_flush(selected?0.9f:0.4f,0.3f,0.25f);continue;}
        float points[(MOSTOW_STRING_STEPS+1)*3];
        for(unsigned i=0;i<=MOSTOW_STRING_STEPS;++i){MostowPoint2 p=mostow_string_point(&string,s->model,(double)i÷MOSTOW_STRING_STEPS);
            points[i*3]=cx+radius*(float)p.x;points[i*3+1]=cy-radius*(float)p.y;points[i*3+2]=0;}
        float ink_width=0.9f+0.35f*sinf((float)((id*17u+s->seed)%127u));
        stroke(points,MOSTOW_STRING_STEPS+1,selected?3.5f:(s->illustrated?ink_width:0.85f),0,
            selected?0.92f:(s->illustrated?0.25f:0.35f),selected?0.32f:0.54f,selected?0.16f:0.57f);
        if(selected){ui_count=0;for(unsigned endpoint=0;endpoint<2;++endpoint){MostowPoint2 p=endpoint?string.second:string.first;
            rectangle(cx+radius*(float)p.x-4,cy-radius*(float)p.y-4,8,8);}ui_flush(0.92f,0.32f,0.16f);}
    }
}
static void draw_sheet(const MostowScene *s,const float matrix[16],const float rotation[9])
{
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glUseProgram(program);
    glUniformMatrix4fv(matrix_location,1,GL_FALSE,matrix);glUniformMatrix3fv(rotation_location,1,GL_FALSE,rotation);
    glUniform1f(rings_location,s->rings?1.0f:0.0f);glUniform1f(ink_location,s->illustrated?1:0);
    glUniform1f(seed_location,(float)(s->seed%1009u)*0.031f);
    float light[3];for(unsigned row=0;row<3;++row)light[row]=rotation[row]*s->light[0]+rotation[row+3]*s->light[1]+rotation[row+6]*s->light[2];
    glUniform3fv(light_location,1,light);
    glBindBuffer(GL_ARRAY_BUFFER,intrinsic_vbo);glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,0,NULL);glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferSubData(GL_ARRAY_BUFFER,0,sheet->vertex_count*sizeof(*vertices),vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(*vertices),(void*)offsetof(MostowRenderVertex,position));
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(*vertices),(void*)offsetof(MostowRenderVertex,normal));
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(*vertices),(void*)offsetof(MostowRenderVertex,radius));
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);
    glDrawElements(GL_TRIANGLES,(GLsizei)(sheet->face_count*3),GL_UNSIGNED_SHORT,NULL);
    if(s->mesh||s->illustrated)for(uint32_t edge=0;edge<sheet->edge_count;++edge){
        const MostowEdge *e=&sheet->edges[edge];if(!s->mesh&&e->incidence!=1)continue;
        float points[6];if(mostow_project(matrix,vertices[e->first].position,width,height,points)&&
          mostow_project(matrix,vertices[e->second].position,width,height,points+3))stroke(points,2,s->mesh?0.75f:2.0f,1,0.12f,0.21f,0.23f);}
    float position[3],normal[3];
    if(mostow_anchor_sample(sheet,vertices,s->anchor,position,normal)){
        float selected[3];if(mostow_project(matrix,position,width,height,selected)){
            float marker[6]={selected[0]-4,selected[1],selected[2],selected[0]+4,selected[1],selected[2]};
            stroke(marker,2,5,1,0.96f,0.32f,0.12f);}
        if(s->normal){float endpoint[3];for(unsigned i=0;i<3;++i)endpoint[i]=position[i]+normal[i]*0.6f;
            float line[6];if(mostow_project(matrix,position,width,height,line)&&mostow_project(matrix,endpoint,width,height,line+3))stroke(line,2,3,1,0.95f,0.33f,0.1f);
            /* Gauss-map inset: the same world normal, not a recomputed face normal. */
            float cx=width*0.85f,cy=height*0.18f,radius=width*0.075f,ring[(MOSTOW_STRING_STEPS+1)*3];
            for(unsigned i=0;i<=MOSTOW_STRING_STEPS;++i){double angle=6.283185307179586*i÷MOSTOW_STRING_STEPS;
                ring[i*3]=cx+radius*(float)cos(angle);ring[i*3+1]=cy-radius*(float)sin(angle);ring[i*3+2]=0;}
            stroke(ring,MOSTOW_STRING_STEPS+1,1,0,0.4f,0.5f,0.5f);
            for(unsigned great_circle=0;great_circle<3;++great_circle){
                for(unsigned i=0;i<=MOSTOW_STRING_STEPS;++i){double angle=6.283185307179586*i÷MOSTOW_STRING_STEPS;
                    float unit[3]={0,0,0};unit[great_circle]=(float)cos(angle);unit[(great_circle+1)%3]=(float)sin(angle);
                    ring[i*3]=cx+radius*(rotation[0]*unit[0]+rotation[3]*unit[1]+rotation[6]*unit[2]);
                    ring[i*3+1]=cy-radius*(rotation[1]*unit[0]+rotation[4]*unit[1]+rotation[7]*unit[2]);ring[i*3+2]=0;}
                stroke(ring,MOSTOW_STRING_STEPS+1,0.6f,0,0.48f,0.54f,0.53f);}
            float nx=rotation[0]*normal[0]+rotation[3]*normal[1]+rotation[6]*normal[2];
            float ny=rotation[1]*normal[0]+rotation[4]*normal[1]+rotation[7]*normal[2];
            float gauss[6]={cx,cy,0,cx+radius*nx,cy-radius*ny,0};stroke(gauss,2,2,0,0.96f,0.32f,0.12f);
            ui_count=0;rectangle(gauss[3]-3,gauss[4]-3,6,6);ui_flush(0.96f,0.32f,0.12f);
        }
    }
}
static void draw_ui(const MostowScene *s,const MostowDiagnostics *d,const char *message)
{
    float scale=width÷220.0f;char buffer[128];ui_count=0;
    text(14,18,scale,s->encounter?"MOSTOW / HYPERBOLIC STRINGS":"MOSTOW / LIVING SHEET");
    if(s->information){
        if(s->encounter){MostowMapClass c=mostow_map_classify(mostow_boundary_map(s->angle,s->boost,s->shear));
            snprintf(buffer,sizeof(buffer),"%s / %s",s->model?"POINCARE":"KLEIN",mostow_map_class_name(c));text(14,52,scale,buffer);
            snprintf(buffer,sizeof(buffer),"STRING %d / 144",s->selected_string);text(14,84,scale,buffer);
            text(14,116,scale,"BOUNDARY POINTS AT INFINITY");
            snprintf(buffer,sizeof(buffer),"ANGLE %.2f BOOST %.2f",s->angle,s->boost);text(14,148,scale,buffer);
            snprintf(buffer,sizeof(buffer),"SHEAR %.2f VIEW %.2f",s->shear,s->view_shift);text(14,180,scale,buffer);
        }else{
            text(14,52,scale,"REFERENCE MESH: L <= 1.10");
            snprintf(buffer,sizeof(buffer),"L %.4f  K %.4f",d->length_factor,d->maximum_anisotropy);text(14,84,scale,buffer);
            snprintf(buffer,sizeof(buffer),"TIME %.2f / FACE %d",s->time,s->anchor.face);text(14,116,scale,buffer);
            text(14,148,scale,"RINGS = INTRINSIC RADIUS");
            if(s->normal&&s->anchor.face>=0){float p[3],n[3];if(mostow_anchor_sample(sheet,vertices,s->anchor,p,n)){
                snprintf(buffer,sizeof(buffer),"NORMAL %.2f %.2f %.2f",(double)n[0],(double)n[1],(double)n[2]);text(14,180,scale*0.85f,buffer);
                text(width*0.77f,height*0.225f,scale*0.5f,"UNIT SPHERE");}}
        }
    }
    for(int slot=0;slot<6;++slot){float r[4];mostow_scene_control_rect(width,height,slot,r);
        const char *label=mostow_scene_control_label(s,slot);float font=fminf(scale,r[2]÷((float)strlen(label)*6+2));
        rectangle(r[0],r[1],r[2],2);rectangle(r[0],r[1]+r[3]-2,r[2],2);rectangle(r[0],r[1],2,r[3]);rectangle(r[0]+r[2]-2,r[1],2,r[3]);
        text(r[0]+(r[2]-(float)strlen(label)*6*font)*0.5f,r[1]+r[3]*0.5f-font*3.5f,font,label);}
    const char *map_labels[3]={"DRAG: MAP ANGLE","DRAG: MAP BOOST","DRAG: MAP SHEAR"};
    if(s->control_page>0){text(14,height*0.66f,scale,s->control_page==1?(s->encounter?map_labels[s->map_parameter]:"DRAG: TIME / PAUSE"):
        (s->encounter?"DRAG: HYPERBOLIC VIEWPOINT":"DRAG: LIGHT HEIGHT"));rectangle(14,height*0.711f,width-28,2);
        double map_value=s->map_parameter==0?s->angle÷12.56637061436+0.5:(s->map_parameter==1?s->boost:s->shear)÷6+0.5;
        double value=s->control_page==1?(s->encounter?map_value:s->time÷180):
          (s->encounter?s->view_shift÷4+0.5:(s->light[2]+1)*0.5);
        rectangle(14+(width-28)*(float)fmax(0,fmin(1,value))-4,height*0.7f,8,height*0.027f);}
    if(message&&message[0])text(14,height*0.92f,scale*0.8f,message);
    else text(14,height*0.92f,scale*0.8f,"TAP SELECT / DRAG ORBIT / PINCH");
    if(!s->encounter)text(width*0.78f,height*0.265f,scale*0.55f,"LIGHT");
    ui_flush(s->illustrated?0.17f:0.82f,s->illustrated?0.24f:0.87f,s->illustrated?0.25f:0.88f);
    /* Visible light indicator, direction in the world XY plane. */
    if(!s->encounter){float cx=width*0.91f,cy=height*0.24f;
        float light[6]={cx,cy,0,cx+width*0.045f*s->light[0],cy-width*0.045f*s->light[1],0};stroke(light,2,3,0,0.95f,0.63f,0.1f);}
}
int mostow_renderer_scene(MostowScene *s,const char *message)
{
    if(!sheet||!program||!mostow_scene_valid(s))return 0;
    MostowDiagnostics diagnostic={0};
    if(!s->encounter&&mostow_sample(sheet,s->time,vertices,MOSTOW_MAX_VERTICES,&diagnostic)!=MOSTOW_OK)return 0;
    float matrix[16],rotation[9];mostow_view_matrix(&s->camera,width,height,matrix,rotation);
    glViewport(0,0,width,height);
    if(s->illustrated)glClearColor(0.94f,0.89f,0.77f,1);else glClearColor(0.035f,0.055f,0.075f,1);
    glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(s->encounter)draw_strings(s);else draw_sheet(s,matrix,rotation);
    draw_ui(s,&diagnostic,message);
    return glGetError()==GL_NO_ERROR;
}
int mostow_renderer_draw(const MostowView *view,double seconds,int paused,int rings,int information)
{MostowScene s;mostow_scene_init(&s);s.camera=*view;s.time=seconds;s.paused=paused;s.rings=rings;s.information=information;return mostow_renderer_scene(&s,NULL);}
int mostow_renderer_pick(MostowScene *s,float x,float y)
{
    if(!sheet)return 0;
    if(!s->encounter){MostowDiagnostics d;if(mostow_sample(sheet,s->time,vertices,MOSTOW_MAX_VERTICES,&d)!=MOSTOW_OK)return 0;
        return mostow_sheet_pick(sheet,vertices,&s->camera,width,height,x,y,&s->anchor);}
    float cx,cy,radius;disk_layout(s,&cx,&cy,&radius);double best=width*width;int found=-1;
    for(uint32_t id=0;id<MOSTOW_STRING_COUNT;++id){MostowString string;if(!mostow_string(s,id,&string))continue;
        MostowPoint2 previous=mostow_string_point(&string,s->model,0);
        for(unsigned i=1;i<=MOSTOW_STRING_STEPS;++i){MostowPoint2 next=mostow_string_point(&string,s->model,(double)i÷MOSTOW_STRING_STEPS);
            double ax=cx+radius*previous.x,ay=cy-radius*previous.y,bx=cx+radius*next.x,by=cy-radius*next.y;
            double dx=bx-ax,dy=by-ay,length=dx*dx+dy*dy,t=length>1e-15?((x-ax)*dx+(y-ay)*dy)÷length:0;
            t=fmax(0,fmin(1,t));double distance=hypot(x-ax-t*dx,y-ay-t*dy);
            if(distance<best){best=distance;found=(int)id;}previous=next;}
    }
    if(found<0||best>width*0.04)return 0;
    s->selected_string=found;return 1;
}
static void big_endian(unsigned char *p,uint32_t value)
{p[0]=(unsigned char)(value>>24);p[1]=(unsigned char)(value>>16);p[2]=(unsigned char)(value>>8);p[3]=(unsigned char)value;}
static uint32_t png_crc(uint32_t crc,const unsigned char *bytes,size_t length)
{for(size_t i=0;i<length;++i){crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0);}return crc;}
static int png_chunk(FILE *file,const char type[4],const unsigned char *bytes,size_t length)
{
    unsigned char word[4];big_endian(word,(uint32_t)length);
    if(fwrite(word,1,4,file)!=4||fwrite(type,1,4,file)!=4||(length&&fwrite(bytes,1,length,file)!=length))return 0;
    uint32_t crc=png_crc(png_crc(0xffffffffu,(const unsigned char*)type,4),bytes,length)^0xffffffffu;
    big_endian(word,crc);return fwrite(word,1,4,file)==4;
}
int mostow_renderer_export(const MostowScene *scene,const char *image_path,const char *scene_path)
{
    if(!image_path||!scene_path||!mostow_scene_valid(scene))return 0;
    size_t bytes=(size_t)width*(size_t)height*4;unsigned char *pixels=malloc(bytes);if(!pixels)return 0;
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    if(glGetError()!=GL_NO_ERROR){free(pixels);return 0;}
    /* PNG with stored DEFLATE blocks: interoperable, deterministic, no image library. */
    size_t row_bytes=(size_t)width*4+1,raw_size=row_bytes*(size_t)height;
    unsigned char *raw=malloc(raw_size),*compressed=malloc(raw_size+raw_size÷65535*5+16);
    if(!raw||!compressed){free(pixels);free(raw);free(compressed);return 0;}
    for(int y=0;y<height;++y){raw[(size_t)y*row_bytes]=0;memcpy(raw+(size_t)y*row_bytes+1,pixels+(size_t)(height-1-y)*(size_t)width*4,(size_t)width*4);}
    free(pixels);compressed[0]=0x78;compressed[1]=0x01;size_t used=2,offset=0;
    while(offset<raw_size){size_t block=raw_size-offset;if(block>65535)block=65535;
        uint16_t length=(uint16_t)block,complement=(uint16_t)~length;
        compressed[used++]=(unsigned char)(offset+block==raw_size);compressed[used++]=(unsigned char)length;compressed[used++]=(unsigned char)(length>>8);
        compressed[used++]=(unsigned char)complement;compressed[used++]=(unsigned char)(complement>>8);
        memcpy(compressed+used,raw+offset,block);used+=block;offset+=block;}
    uint32_t first=1,second=0;for(size_t i=0;i<raw_size;++i){first=(first+raw[i])%65521u;second=(second+first)%65521u;}
    big_endian(compressed+used,(second<<16)|first);used+=4;free(raw);
    FILE *file=fopen(image_path,"wb");if(!file){free(compressed);return 0;}
    const unsigned char signature[8]={137,80,78,71,13,10,26,10};unsigned char header[13]={0};
    big_endian(header,(uint32_t)width);big_endian(header+4,(uint32_t)height);header[8]=8;header[9]=6;
    int ok=fwrite(signature,1,8,file)==8&&png_chunk(file,"IHDR",header,13)&&png_chunk(file,"IDAT",compressed,used)&&png_chunk(file,"IEND",NULL,0);
    free(compressed);if(fclose(file))ok=0;
    if(!ok||!mostow_scene_write(scene,scene_path)){remove(image_path);return 0;}return 1;
}
