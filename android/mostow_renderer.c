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
static GLuint program,vbo,ebo,ui_program,ui_vbo;
static GLint matrix_location,rotation_location,rings_location,ui_color;
static int width=1,height=1;
static const char *vertex_shader =
 "attribute vec3 a_position; attribute vec3 a_normal; attribute float a_radius;\n"
 "uniform mat4 u_matrix; uniform mat3 u_rotation;\n"
 "varying vec3 v_normal; varying float v_radius;\n"
 "void main(){gl_Position=u_matrix*vec4(a_position,1.0);"
 "v_normal=u_rotation*a_normal;v_radius=a_radius;}\n";
static const char *fragment_shader =
 "precision mediump float; varying vec3 v_normal; varying float v_radius; uniform float u_rings;\n"
 "void main(){vec3 n=normalize(v_normal);if(!gl_FrontFacing)n=-n;"
 "float light=0.35+0.60*max(0.0,dot(n,normalize(vec3(-0.4,0.6,0.9))));"
 "vec3 material=gl_FrontFacing?vec3(0.79,0.57,0.37):vec3(0.46,0.65,0.67);"
 "float band=1.0-smoothstep(0.0,0.12,abs(sin(v_radius*12.5663706)));"
 "material=mix(material,vec3(0.26,0.32,0.30),band*0.5*u_rings);"
 "gl_FragColor=vec4(material*light,1.0);}\n";

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
    glLinkProgram(p);glDeleteShader(a);glDeleteShader(b);
    GLint valid=0;glGetProgramiv(p,GL_LINK_STATUS,&valid);
    if(!valid){glDeleteProgram(p);return 0;}return p;
}
void mostow_renderer_stop(void)
{
    if(vbo)glDeleteBuffers(1,&vbo);
    if(ebo)glDeleteBuffers(1,&ebo);
    if(ui_vbo)glDeleteBuffers(1,&ui_vbo);
    if(program)glDeleteProgram(program);
    if(ui_program)glDeleteProgram(ui_program);
    vbo=ebo=ui_vbo=program=ui_program=0;
}
void mostow_renderer_resize(int w,int h){width=w>0?w:1;height=h>0?h:1;}
int mostow_renderer_start(int w,int h)
{
    mostow_renderer_stop();mostow_renderer_resize(w,h);
    if(!sheet){sheet=calloc(1,sizeof(*sheet));if(!sheet)return 0;
        MostowStatus status=mostow_sheet_init(sheet);if(status!=MOSTOW_OK){LOGE("sheet init failed: %d\n",status);free(sheet);sheet=NULL;return 0;}}
    program=link(vertex_shader,fragment_shader);
    ui_program=link("attribute vec2 a_position;void main(){gl_Position=vec4(a_position,0.0,1.0);}",
                    "precision mediump float;uniform vec3 u_color;void main(){gl_FragColor=vec4(u_color,1.0);}");
    if(!program||!ui_program){mostow_renderer_stop();return 0;}
    matrix_location=glGetUniformLocation(program,"u_matrix");rotation_location=glGetUniformLocation(program,"u_rotation");
    rings_location=glGetUniformLocation(program,"u_rings");ui_color=glGetUniformLocation(ui_program,"u_color");
    for(uint32_t f=0;f<sheet->face_count;++f)for(uint32_t j=0;j<3;++j)indices[3*f+j]=sheet->faces[f].vertex[j];
    glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glGenBuffers(1,&ui_vbo);
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
static void draw_ui(const MostowDiagnostics *diagnostic,int paused,int information)
{
    float scale=(float)width÷200.0f;
    ui_count=0;
    text(18,22,scale*1.3f,"MOSTOW / LIVING SHEET");
    if(information){char buffer[100];
        text(18,66,scale,"REFERENCE MESH: L <= 1.10");
        snprintf(buffer,sizeof(buffer),"CURRENT L %.4f  K %.4f",diagnostic->length_factor,diagnostic->maximum_anisotropy);
        text(18,96,scale,buffer);
        text(18,126,scale,"R 2: C 22.79 / EUCLID 12.57");
    }
    MostowControlBand band=mostow_controls_layout(width,height);
    const char *buttons[3]={paused?"PLAY":"PAUSE","RINGS","INFO"};
    for(int button=0;button<3;++button){
        float left=(float)button*(float)width÷3.0f+8.0f;
        float right=(float)(button+1)*(float)width÷3.0f-8.0f;
        float top=band.touch_top,bottom=band.touch_bottom;
        float center=(float)(2*button+1)*(float)width÷6.0f;
        float text_width=(float)strlen(buttons[button])*6.0f*scale;
        rectangle(left,top,right-left,2.0f);
        rectangle(left,bottom-2.0f,right-left,2.0f);
        rectangle(left,top,2.0f,bottom-top);
        rectangle(right-2.0f,top,2.0f,bottom-top);
        text(center-text_width÷2.0f,band.center_y-3.5f*scale,scale,buttons[button]);
    }
    glDisable(GL_DEPTH_TEST);glUseProgram(ui_program);glUniform3f(ui_color,0.82f,0.87f,0.88f);
    glBindBuffer(GL_ARRAY_BUFFER,ui_vbo);glBufferData(GL_ARRAY_BUFFER,ui_count*sizeof(float),ui_vertices,GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,NULL);
    glEnableVertexAttribArray(0);glDisableVertexAttribArray(1);glDisableVertexAttribArray(2);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)(ui_count÷2));
}
int mostow_renderer_draw(const MostowView *view,double seconds,int paused,int rings,int information)
{
    if(!sheet||!program)return 0;
    MostowDiagnostics diagnostic;
    MostowStatus status=mostow_sample(sheet,seconds,vertices,MOSTOW_MAX_VERTICES,&diagnostic);
    if(status!=MOSTOW_OK){LOGE("sampling failed: %d\n",status);return 0;}
    float matrix[16],rotation[9];mostow_view_matrix(view,width,height,matrix,rotation);
    glViewport(0,0,width,height);glClearColor(0.035f,0.055f,0.075f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glUseProgram(program);
    glUniformMatrix4fv(matrix_location,1,GL_FALSE,matrix);glUniformMatrix3fv(rotation_location,1,GL_FALSE,rotation);
    glUniform1f(rings_location,rings?1.0f:0.0f);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferSubData(GL_ARRAY_BUFFER,0,sheet->vertex_count*sizeof(*vertices),vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(*vertices),(void*)offsetof(MostowRenderVertex,position));
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(*vertices),(void*)offsetof(MostowRenderVertex,normal));
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(*vertices),(void*)offsetof(MostowRenderVertex,radius));
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);
    glDrawElements(GL_TRIANGLES,(GLsizei)(sheet->face_count*3),GL_UNSIGNED_SHORT,NULL);
    draw_ui(&diagnostic,paused,information);
    return glGetError()==GL_NO_ERROR;
}
