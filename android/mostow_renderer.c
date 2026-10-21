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
 {'+',{0,4,4,31,4,4,0}},
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
        char letter←value[c];
        if(letter>='a'&&letter<='z')letter←(char)(letter-'a'+'A');
        const Glyph *glyph=NULL;
        for(size_t g=0;g<sizeof(glyphs)÷sizeof(glyphs[0]);++g)if(glyphs[g].letter==letter){glyph=&glyphs[g];break;}
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
static void draw_m004(const MostowScene *scene,const float matrix[16])
{
    MostowIdealShape complete[2];mostow_m004_complete_shapes(complete);
    const unsigned edges[6][2]={{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
    for(unsigned group=0;group<2;++group){
        const MostowIdealShape *shapes=group==1||scene->rigidity.enforce_completeness?complete:scene->rigidity.trial;
        for(unsigned tetrahedron=0;tetrahedron<2;++tetrahedron){
            double ideal[4][3];if(!mostow_m004_ideal_vertices(shapes[tetrahedron],ideal))continue;
            float projected[4][3];int visible=1;
            for(unsigned vertex=0;vertex<4;++vertex){
                const float position[3]={0.65f*(float)ideal[vertex][0]+(group?1.2f:-1.2f),
                    0.65f*(float)ideal[vertex][1]+(tetrahedron?-0.85f:0.85f),0.65f*(float)ideal[vertex][2]};
                if(!mostow_project(matrix,position,width,height,projected[vertex]))visible=0;
            }
            if(!visible)continue;
            for(unsigned edge=0;edge<6;++edge){float points[6];
                memcpy(points,projected[edges[edge][0]],3*sizeof(float));
                memcpy(points+3,projected[edges[edge][1]],3*sizeof(float));
                stroke(points,2,group?2.5f:3.5f,0,group?0.25f:0.95f,group?0.82f:0.48f,group?0.85f:0.22f);
            }
        }
    }
    ui_count=0;
    text(width*0.1f,height*0.30f,width÷300.0f,scene->rigidity.enforce_completeness?"COMPLETE KEPT":"FREE SHAPES");
    text(width*0.63f,height*0.30f,width÷300.0f,"REFERENCE");
    if(scene->information){const MostowM004Triangulation triangulation←mostow_m004_triangulation();
        const MostowM004Topology topology←mostow_m004_check_topology(&triangulation);
        text(14,height*0.62f,width÷350.0f,topology.manifold_verified?
            "INTEGER TOPOLOGY: 2 EDGE CLASSES / TORUS CUSP":"COMBINATORIAL TOPOLOGY CHECK FAILED");}
    ui_flush(scene->illustrated?0.12f:0.85f,scene->illustrated?0.18f:0.9f,scene->illustrated?0.2f:0.95f);
}
static void comparison_circle(float center_x,float center_y,float radius)
{
    float points[(MOSTOW_STRING_STEPS+1)*3];
    for(unsigned index←0;index<=MOSTOW_STRING_STEPS;++index){
        const double angle←6.283185307179586*(double)index÷MOSTOW_STRING_STEPS;
        points[3*index]←center_x+radius*(float)cos(angle);
        points[3*index+1]←center_y-radius*(float)sin(angle);points[3*index+2]←0;
    }
    stroke(points,MOSTOW_STRING_STEPS+1,1,0,0.4f,0.48f,0.48f);
}
static void draw_octagon(const MostowScene *scene)
{
    MostowOctagon octagon;mostow_octagon_init(&octagon);
    const float center_y←height*0.44f,radius←width*0.215f;
    const float colors[4][3]←{{0.95f,0.4f,0.18f},{0.2f,0.75f,0.8f},{0.7f,0.45f,0.85f},{0.7f,0.8f,0.22f}};
    const unsigned color_class[8]←{0,1,0,1,2,3,2,3};
    for(unsigned panel←0;panel<2;++panel){
        const float center_x←width*(panel?0.75f:0.25f);
        comparison_circle(center_x,center_y,radius);
        for(unsigned tile←0;tile<(panel&&!scene->model?MOSTOW_OCTAGON_NEIGHBORS:1u);++tile){
            MostowOctagonIsometry isometry;
            if(!mostow_octagon_neighbor(&octagon,tile,&isometry))continue;
            for(unsigned side←0;side<8;++side){
                float points[(MOSTOW_STRING_STEPS+1)*3];int available←1;
                for(unsigned index←0;index<=MOSTOW_STRING_STEPS;++index){
                    MostowOctagonPoint point,image;
                    if(!mostow_octagon_side_point(&octagon,side,(double)index÷MOSTOW_STRING_STEPS,&point)||
                       !mostow_octagon_apply(isometry,point,&image)){available←0;break;}
                    points[3*index]←center_x+radius*(float)image.x;
                    points[3*index+1]←center_y-radius*(float)image.y;points[3*index+2]←0;
                }
                const float *color←colors[color_class[side]];
                if(available)stroke(points,MOSTOW_STRING_STEPS+1,tile?0.7f:2.2f,0,color[0],color[1],color[2]);
            }
        }
        MostowOctagonPoint marker←scene->quotient.position;
        if(panel&&!mostow_octagon_apply(scene->quotient.lift,marker,&marker))continue;
        ui_count←0;rectangle(center_x+radius*(float)marker.x-4,center_y-radius*(float)marker.y-4,8,8);
        ui_flush(0.98f,0.35f,0.12f);
        if(!panel){
            static const char *side_names[8]←{"A+","B+","A-","B-","C+","D+","C-","D-"};
            ui_count←0;
            for(unsigned side←0;side<8;++side){MostowOctagonPoint midpoint;
                if(mostow_octagon_side_point(&octagon,side,0.5,&midpoint))
                    text(center_x+radius*1.12f*(float)midpoint.x-6,
                         center_y-radius*1.12f*(float)midpoint.y-4,width÷480.0f,side_names[side]);
            }
            ui_flush(scene->illustrated?0.12f:0.85f,scene->illustrated?0.18f:0.9f,scene->illustrated?0.2f:0.92f);
        }
    }
    ui_count←0;text(width*0.11f,height*0.29f,width÷310.0f,"QUOTIENT CUT");
    text(width*0.62f,height*0.29f,width÷310.0f,scene->model?"COVER: 1 TILE":"COVER: 9 TILES");
    char word[100]←"WORD";size_t used←4;
    for(unsigned index←0;index<scene->quotient.word_length&&index<14;++index){
        const int letter←scene->quotient.word[index];
        int written←snprintf(word+used,sizeof(word)-used," %c%s",'A'+abs(letter)-1,letter<0?"-":"+");
        if(written<0||(size_t)written>=sizeof(word)-used)break;
        used+=(size_t)written;
    }
    text(14,height*0.59f,width÷310.0f,word);
    text(14,height*0.62f,width÷340.0f,"PAIRED SIDES REVERSE - OVERLAP IS NOT GLUING");
    ui_flush(scene->illustrated?0.12f:0.82f,scene->illustrated?0.18f:0.88f,scene->illustrated?0.2f:0.9f);
}
static void draw_pants(const MostowScene *scene)
{
    const MostowPantsDiagnostics pants←mostow_pants_evaluate(&scene->pants);
    if(!pants.available)return;
    const float radius←width*0.175f;
    for(unsigned pant←0;pant<2;++pant)for(unsigned half←0;half<2;++half){
        const float center_x←width*(pant?0.74f:0.26f),center_y←height*(half?0.49f:0.36f);
        for(unsigned side←0;side<6;++side){
            float points[(MOSTOW_STRING_STEPS+1)*3];int available←1;
            for(unsigned index←0;index<=MOSTOW_STRING_STEPS;++index){MostowPantsPoint point;
                if(!mostow_pants_hexagon_point(&pants,side,(double)index÷MOSTOW_STRING_STEPS,&point)){available←0;break;}
                points[3*index]←center_x+radius*(float)point.x;
                points[3*index+1]←center_y+(half?1:-1)*radius*(float)point.y;points[3*index+2]←0;
            }
            if(available)stroke(points,MOSTOW_STRING_STEPS+1,side%2?2.5f:1.2f,0,
                side%2?0.95f:0.3f,side%2?0.48f:0.76f,side%2?0.22f:0.82f);
        }
    }
    /* The bars encode actual transverse geodesic lengths from holonomy.
     * A fixed panel layout is not an isometric embedding of the surface. */
    ui_count←0;
    for(unsigned loop←0;loop<3;++loop){
        const double length←pants.transverse_lengths[loop];
        if(isfinite(length)&&length>=0)rectangle(width*0.12f,height*(0.57f+0.018f*loop),
            width*0.76f*(float)(length÷(length+8)),4);
    }
    ui_flush(0.3f,0.76f,0.82f);
    ui_count←0;text(width*0.09f,height*0.27f,width÷320.0f,"PANT 1");
    text(width*0.64f,height*0.27f,width÷320.0f,"PANT 2");
    text(14,height*0.635f,width÷360.0f,"FOUR RIGHT HEXAGONS - CUT DRAWING");
    ui_flush(scene->illustrated?0.12f:0.82f,scene->illustrated?0.18f:0.88f,scene->illustrated?0.2f:0.9f);
}
static void draw_distortion(const MostowScene *scene)
{
    MostowTangentMetric source,target;mostow_scene_tangent_metrics(scene,&source,&target);
    const MostowDifferential2 differential←mostow_scene_differential(scene);
    const MostowDistortionDiagnostics measured←mostow_scene_distortion(scene);
    const double largest←measured.status==MOSTOW_DISTORTION_OK?fmax(1,measured.sigma_major):3;
    const float radius←width*0.205f÷(float)largest,center_y←height*0.44f;
    float circle[(MOSTOW_STRING_STEPS+1)*3],ellipse[(MOSTOW_STRING_STEPS+1)*3];
    int available←1;
    for(unsigned index←0;index<=MOSTOW_STRING_STEPS;++index){
        MostowTangentPoint before,after;
        if(mostow_distortion_sample(differential,source,target,6.283185307179586*index÷MOSTOW_STRING_STEPS,
                                   &before,&after)!=MOSTOW_DISTORTION_OK){available←0;break;}
        circle[3*index]←width*0.25f+radius*(float)before.x;
        circle[3*index+1]←center_y-radius*(float)before.y;circle[3*index+2]←0;
        ellipse[3*index]←width*0.75f+radius*(float)after.x;
        ellipse[3*index+1]←center_y-radius*(float)after.y;ellipse[3*index+2]←0;
    }
    if(available){stroke(circle,MOSTOW_STRING_STEPS+1,2.2f,0,0.3f,0.8f,0.85f);
        stroke(ellipse,MOSTOW_STRING_STEPS+1,2.6f,0,0.98f,0.45f,0.2f);
        /* Two labelled-by-color radii make rotations and orientation reversal
         * visible even when the boundary curve remains a circle. */
        for(unsigned axis←0;axis<2;++axis){const unsigned point←axis*(MOSTOW_STRING_STEPS÷4);
            const float source_radius[6]←{width*0.25f,center_y,0,circle[3*point],circle[3*point+1],0};
            const float target_radius[6]←{width*0.75f,center_y,0,ellipse[3*point],ellipse[3*point+1],0};
            stroke(source_radius,2,1.6f,0,axis?0.95f:0.4f,axis?0.8f:0.75f,axis?0.25f:0.9f);
            stroke(target_radius,2,1.6f,0,axis?0.95f:0.4f,axis?0.8f:0.75f,axis?0.25f:0.9f);
        }
    }
    else comparison_circle(width*0.25f,center_y,width*0.14f);
    ui_count←0;text(width*0.09f,height*0.29f,width÷320.0f,"SOURCE CIRCLE");
    text(width*0.60f,height*0.29f,width÷320.0f,"TARGET IMAGE");
    text(14,height*0.61f,width÷340.0f,"ORTHONORMAL FRAMES FOR SPECIFIED METRICS");
    ui_flush(scene->illustrated?0.12f:0.82f,scene->illustrated?0.18f:0.88f,scene->illustrated?0.2f:0.9f);
}
static void draw_coarse(const MostowScene *scene)
{
    const MostowIntegerWitness witness←mostow_integer_inclusion_witness(scene->coarse_position);
    const float left←width*0.12f,right←width*0.88f,center_y←height*0.44f;
    const float line[6]←{left,center_y,0,right,center_y,0};stroke(line,2,2,0,0.4f,0.6f,0.7f);
    ui_count←0;
    for(int integer←-4;integer<=4;++integer){
        const float x←left+(right-left)*(float)(integer+4)÷8;
        rectangle(x-2,center_y-9,4,18);char label[16];snprintf(label,sizeof(label),"%d",integer);
        text(x-4,center_y+20,width÷350.0f,label);
    }
    ui_flush(0.3f,0.8f,0.85f);
    const float point_x←left+(right-left)*(float)(scene->coarse_position+4)÷8;
    ui_count←0;rectangle(point_x-4,center_y-35,8,14);ui_flush(0.98f,0.4f,0.16f);
    if(witness.available){
        const float nearest_x←left+(right-left)*(float)(witness.integer+4)÷8;
        const float segment[6]←{point_x,center_y-25,0,nearest_x,center_y,0};
        stroke(segment,2,2,0,0.98f,0.4f,0.16f);
    }
    ui_count←0;text(14,height*0.56f,width÷320.0f,"FINITE WINDOW OF INFINITE SPACES");
    text(14,height*0.595f,width÷320.0f,"EVERY REAL HAS A NEAREST INTEGER");
    text(14,height*0.63f,width÷320.0f,"DISTANCE AT MOST ONE HALF - GLOBAL PROOF");
    ui_flush(scene->illustrated?0.12f:0.82f,scene->illustrated?0.18f:0.88f,scene->illustrated?0.2f:0.9f);
}
static void draw_boundary_comparison(const MostowScene *scene,const float matrix[16])
{
    MostowIdealShape complete[2];mostow_m004_complete_shapes(complete);
    const unsigned edges[6][2]←{{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
    for(unsigned example←0;example<2;++example){double vertices_on_sphere[4][3];MostowIdealShape cross_ratio;
        const double shear←example?0.25+0.15*scene->distortion_amount:0;
        if(!mostow_tetrahedron_boundary_example(complete[0],shear,1,vertices_on_sphere,&cross_ratio))continue;
        float projected[4][3];int visible←1;
        for(unsigned vertex←0;vertex<4;++vertex){
            const float position[3]←{0.9f*(float)vertices_on_sphere[vertex][0]+(example?1.2f:-1.2f),
                0.9f*(float)vertices_on_sphere[vertex][1],0.9f*(float)vertices_on_sphere[vertex][2]};
            if(!mostow_project(matrix,position,width,height,projected[vertex]))visible←0;
        }
        if(visible)for(unsigned edge←0;edge<6;++edge){float points[6];
            memcpy(points,projected[edges[edge][0]],3*sizeof(float));
            memcpy(points+3,projected[edges[edge][1]],3*sizeof(float));
            stroke(points,2,2.7f,0,example?0.98f:0.3f,example?0.45f:0.8f,example?0.2f:0.85f);
        }
    }
}
static void draw_rigidity_account(const MostowScene *scene,const float matrix[16])
{
    static const char *pages[6][8]←{
        {"CITED: MOSTOW-PRASAD RIGIDITY","COMPLETE CONNECTED HYPERBOLIC MANIFOLDS",
         "FINITE VOLUME - DIMENSION AT LEAST 3","ISOMORPHIC FUNDAMENTAL GROUPS",
         "GROUP ISOMORPHISM COMES FROM ISOMETRY","COMPLETENESS AND FINITE VOLUME MATTER",
         "SURFACES IN DIMENSION 2 ARE DIFFERENT","NEXT: HOW THE GLOBAL ARGUMENT WORKS"},
        {"PROOF ROUTE: GROUP TO BOUNDARY","A GROUP ISOMORPHISM FIXES EQUIVARIANCE",
         "CITED LEMMA: AN APPROPRIATE LIFT","EXTENDS TO AN EQUIVARIANT BOUNDARY MAP",
         "CUSPS REQUIRE THE FINITE-VOLUME ARGUMENT","AN ARBITRARY BOUNDARY MAP NEED NOT BE RIGID",
         "THE VOLUME STEP SUPPLIES THE CONSTRAINT","CITED LEMMAS - NOT A NUMERICAL PROOF"},
        {"PROOF ROUTE: GLOBAL VOLUME","STRAIGHTEN CHAINS TO GEODESIC SIMPLICES",
         "COMPARE THE SHARP SIMPLEX VOLUME BOUND","SMEARING TIES THAT BOUND TO TOTAL VOLUME",
         "EQUALITY FORCES REGULAR IDEAL SIMPLICES","BOUNDARY CONFIGURATIONS BECOME RIGID",
         "COMPLETE ARGUMENT IS A CITED THEOREM","FINITE TETRAHEDRA DO NOT PROVE THIS"},
        {"IDEAL TETRAHEDRA: DIMENSION 3","A COMPLEX CROSS-RATIO SPECIFIES SHAPE",
         "THREE POSITIVE ANGLES ADD TO PI","REGULAR: EACH DIHEDRAL ANGLE IS PI/3",
         "CITED: REGULAR IDEAL VOLUME IS MAXIMAL","M004 JOINS TWO REGULAR TETRAHEDRA",
         "EDGE EQUATIONS ALONE DO NOT CLOSE A CUSP","M004 BUTTON OPENS THE EXECUTABLE TEST"},
        {"SURFACE FLEXIBILITY: DIMENSION 2","TWO PANTS JOIN TO FORM GENUS 2",
         "THREE LENGTHS AND THREE TWISTS VARY","CURVATURE STAYS -1 - AREA STAYS 4 PI",
         "TRANSVERSE GEODESIC LENGTHS CHANGE","THE MARKED HYPERBOLIC STRUCTURE CHANGES",
         "TOPOLOGY ALONE DOES NOT FIX THIS METRIC","SURFACES BUTTON OPENS THE CONSTRUCTION"},
        {"KEEP THE EVIDENCE LEVELS DISTINCT","COMBINATORIAL CHECK: FINITE INTEGER DATA",
         "ALGEBRAIC IDENTITY: STATED ASSUMPTIONS","NUMERICAL TEST: FINITE PRECISION ERRORS",
         "CITED THEOREM: GLOBAL RIGIDITY RESULT","PICTURE: VIEW OF THE CURRENT MATH STATE",
         "HOST RENDERING IS NOT PHONE ACCEPTANCE","NO FINITE NUMERICAL TEST PROVES RIGIDITY"}
    };
    if(scene->lesson_page==3){
        draw_boundary_comparison(scene,matrix);
        ui_count←0;text(14,height*0.26f,width÷360.0f,"MOBIUS: CROSS-RATIO KEPT");
        text(width*0.53f,height*0.29f,width÷360.0f,"NONCONFORMAL IMAGE");
        text(14,height*0.59f,width÷360.0f,"SAME INPUT TETRAHEDRON - DIFFERENT MAPS");
        text(14,height*0.625f,width÷360.0f,"DRAG TO VARY REAL-LINEAR DISTORTION");
        ui_flush(scene->illustrated?0.12f:0.82f,scene->illustrated?0.18f:0.88f,scene->illustrated?0.2f:0.9f);return;
    }
    ui_count←0;
    for(unsigned row←0;row<8;++row)text(14,height*(0.24f+0.047f*row),width÷360.0f,pages[scene->lesson_page][row]);
    ui_flush(scene->illustrated?0.12f:0.82f,scene->illustrated?0.18f:0.88f,scene->illustrated?0.2f:0.9f);
}
static void draw_ui(const MostowScene *s,const MostowDiagnostics *d,const char *message)
{
    float scale=width÷220.0f;char buffer[128];ui_count=0;
    static const char *titles[8]←{"MOSTOW / LIVING SHEET","MOSTOW / HYPERBOLIC STRINGS","MOSTOW / FIGURE EIGHT",
        "MOSTOW / GENUS 2 QUOTIENT","MOSTOW / FENCHEL-NIELSEN","MOSTOW / LOCAL DISTORTION",
        "MOSTOW / COARSE GEOMETRY","MOSTOW / RIGIDITY ARGUMENT"};
    text(14,18,scale,titles[s->encounter]);
    if(s->information){
        if(s->encounter==3){MostowOctagon octagon;mostow_octagon_init(&octagon);
            const MostowOctagonDiagnostics topology←mostow_octagon_diagnostics(&octagon);
            snprintf(buffer,sizeof(buffer),"V %u E %u F %u / CHI %d",topology.vertices,topology.edges,topology.faces,topology.euler_characteristic);
            text(14,52,scale,buffer);snprintf(buffer,sizeof(buffer),"AREA %.8f / 4 PI",topology.area);text(14,84,scale,buffer);
            snprintf(buffer,sizeof(buffer),"CROSSED %u / WORD %u",s->quotient.total_crossings,s->quotient.word_length);text(14,116,scale,buffer);
            snprintf(buffer,sizeof(buffer),"PAIR ERR %.1E CYCLE %.1E",topology.pairing_error,topology.cycle_error);text(14,148,scale*0.8f,buffer);
            text(14,180,scale*0.8f,s->navigation_error?"MOVE REJECTED - RESET OR STEP BACK":"MORE: WALK THROUGH IDENTIFIED SIDES");
        }else if(s->encounter==4){const MostowPantsDiagnostics pants←mostow_pants_evaluate(&s->pants);
            snprintf(buffer,sizeof(buffer),"L %.2f %.2f %.2f",s->pants.lengths[0],s->pants.lengths[1],s->pants.lengths[2]);text(14,52,scale,buffer);
            snprintf(buffer,sizeof(buffer),"TWIST %.2f %.2f %.2f",s->pants.twists[0],s->pants.twists[1],s->pants.twists[2]);text(14,84,scale,buffer);
            if(pants.available){snprintf(buffer,sizeof(buffer),"LOOPS %.3f %.3f %.3f",pants.transverse_lengths[0],pants.transverse_lengths[1],pants.transverse_lengths[2]);text(14,116,scale*0.85f,buffer);
                snprintf(buffer,sizeof(buffer),"AREA %.6f K %.0f GENUS %d",pants.area,pants.curvature,pants.genus);text(14,148,scale*0.85f,buffer);
                snprintf(buffer,sizeof(buffer),"HOLONOMY ERR %.1E",pants.surface_relation_error);text(14,180,scale*0.85f,buffer);
                text(14,212,scale*0.75f,pants.numerically_qualified?"NUMERICALLY QUALIFIED - NOT INTERVAL PROOF":"NUMERICAL QUALIFICATION FAILED");
                snprintf(buffer,sizeof(buffer),"CUFF TR %.3f %.3f %.3f",pants.cuff_traces[0],pants.cuff_traces[1],pants.cuff_traces[2]);text(14,240,scale*0.7f,buffer);
                snprintf(buffer,sizeof(buffer),"LOOP TR %.3f %.3f %.3f",pants.transverse_traces[0],pants.transverse_traces[1],pants.transverse_traces[2]);text(14,268,scale*0.7f,buffer);
            }else text(14,116,scale,"NUMERIC RANGE UNAVAILABLE");
        }else if(s->encounter==5){const MostowDistortionDiagnostics distortion←mostow_scene_distortion(s);
            static const char *maps[5]←{"ROTATION","UNIFORM SCALE","SHEAR","ORIENTATION REVERSING","DEGENERATE MAP"};
            text(14,52,scale,maps[s->distortion_preset]);
            if(distortion.status==MOSTOW_DISTORTION_OK){
                snprintf(buffer,sizeof(buffer),"SIGMA %.4f %.4f",distortion.sigma_major,distortion.sigma_minor);text(14,84,scale,buffer);
                snprintf(buffer,sizeof(buffer),"K %.4f AREA %+.4f",distortion.dilatation,distortion.signed_area_scale);text(14,116,scale,buffer);
                text(14,148,scale*0.85f,distortion.conformal_to_tolerance?"CONFORMAL TO NUMERICAL TOLERANCE":
                    (distortion.anticonformal_to_tolerance?"ANTICONFORMAL - ORIENTATION REVERSES":"ANISOTROPIC DIFFERENTIAL"));
                snprintf(buffer,sizeof(buffer),"ROUNDING EST %.1E",distortion.relative_roundoff_estimate);text(14,212,scale*0.75f,buffer);
            }else text(14,84,scale,mostow_distortion_status_name(distortion.status));
            text(14,180,scale*0.85f,s->distortion_metric?"METRICS: SOURCE DIAG 4 1 / TARGET I":"METRICS: SOURCE I / TARGET I");
        }else if(s->encounter==6){const MostowCoarseConstants constants←mostow_integer_inclusion_constants();
            const MostowIntegerWitness witness←mostow_integer_inclusion_witness(s->coarse_position);
            snprintf(buffer,sizeof(buffer),"GLOBAL L %.0f A %.0f B %.1f",constants.multiplicative,constants.additive,constants.coarse_surjectivity);text(14,52,scale,buffer);
            snprintf(buffer,sizeof(buffer),"REAL %.3f NEAREST %lld",s->coarse_position,(long long)witness.integer);text(14,84,scale,buffer);
            snprintf(buffer,sizeof(buffer),"DISTANCE %.4f <= ONE HALF",witness.distance);text(14,116,scale*0.85f,buffer);
            text(14,148,scale*0.8f,"INTEGER DISTANCES ARE PRESERVED EXACTLY");
            text(14,180,scale*0.8f,"[N-1/2,N+1/2] COVERS THE REAL LINE");
        }else if(s->encounter==7){
            snprintf(buffer,sizeof(buffer),"ARGUMENT PAGE %d / 6",s->lesson_page+1);text(14,52,scale,buffer);
            text(14,84,scale*0.8f,"CITED GLOBAL THEOREM - NOT A FINITE TEST");
            if(s->lesson_page==3){MostowIdealShape complete[2],image;double sphere[4][3];mostow_m004_complete_shapes(complete);
                if(mostow_tetrahedron_boundary_example(complete[0],0.25+0.15*s->distortion_amount,1,sphere,&image)){
                    const MostowTetrahedronVolume regular←mostow_ideal_tetrahedron_volume(complete[0]);
                    const MostowTetrahedronVolume changed←mostow_ideal_tetrahedron_volume(image);
                    snprintf(buffer,sizeof(buffer),"CROSS RATIO %.3f + %.3f I",image.real,image.imaginary);text(14,116,scale*0.85f,buffer);
                    snprintf(buffer,sizeof(buffer),"V REG %.5f / IMAGE %.5f",regular.volume,changed.volume);text(14,148,scale*0.85f,buffer);
                    text(14,180,scale*0.75f,"REGULAR IDEAL TETRAHEDRON MAXIMIZES VOLUME");
                }
            }
        }else if(s->encounter==2){MostowM004Diagnostics trial=mostow_m004_constraints(s->rigidity.trial);
            text(14,52,scale,!trial.available?"INVALID IDEAL SHAPES":(trial.complete_to_tolerance?"COMPLETE TO TOLERANCE":
                (s->rigidity.enforce_completeness?"TRIAL REJECTED":"INCOMPLETE TRIAL")));
            if(trial.available){
                snprintf(buffer,sizeof(buffer),"EDGE ERR %.2E %.2E",trial.residual[0],trial.residual[1]);text(14,84,scale*0.85f,buffer);
                snprintf(buffer,sizeof(buffer),"CUSP ERR %.2E %.2E",trial.residual[2],trial.residual[3]);text(14,116,scale*0.85f,buffer);
            }
            snprintf(buffer,sizeof(buffer),"Z0 %+.3f %+.3f I",s->rigidity.trial[0].real,s->rigidity.trial[0].imaginary);text(14,148,scale,buffer);
            snprintf(buffer,sizeof(buffer),"Z1 %+.3f %+.3f I",s->rigidity.trial[1].real,s->rigidity.trial[1].imaginary);text(14,180,scale,buffer);
            text(14,212,scale*0.75f,"NUMERICAL CHECKS - TOLERANCE 1E-10");
            const MostowTetrahedronVolume first←mostow_ideal_tetrahedron_volume(s->rigidity.trial[0]);
            const MostowTetrahedronVolume second←mostow_ideal_tetrahedron_volume(s->rigidity.trial[1]);
            if(first.available&&second.available){snprintf(buffer,sizeof(buffer),"TRIAL VOLUME %.10f",first.volume+second.volume);text(14,240,scale*0.8f,buffer);}
        }else if(s->encounter){MostowMapClass c=mostow_map_classify(mostow_boundary_map(s->angle,s->boost,s->shear));
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
    if(s->encounter>=3&&s->control_page>0){
        const char *label←"BUTTONS WALK THROUGH IDENTIFIED SIDES";double value←0.5;int slider←0;
        if(s->encounter==4){static const char *parameters[6]←{"DRAG: CUFF LENGTH 1","DRAG: CUFF LENGTH 2","DRAG: CUFF LENGTH 3",
            "DRAG: SIGNED TWIST 1","DRAG: SIGNED TWIST 2","DRAG: SIGNED TWIST 3"};
            label←parameters[s->mechanical_parameter];slider←1;
            value←s->mechanical_parameter<3?(s->pants.lengths[s->mechanical_parameter]-0.75)÷2.25:
                (s->pants.twists[s->mechanical_parameter-3]+2)÷4;
        }else if(s->encounter==5){label←"DRAG: DIFFERENTIAL PARAMETER";slider←1;value←(s->distortion_amount+3)÷6;
        }else if(s->encounter==6){label←"DRAG: REAL POINT";slider←1;value←(s->coarse_position+4)÷8;
        }else if(s->encounter==7){label←"PREVIOUS / NEXT: PROOF AND EVIDENCE";
            if(s->lesson_page==3){label←"DRAG: NONCONFORMAL BOUNDARY MAP";slider←1;value←(s->distortion_amount+3)÷6;}}
        text(14,height*0.66f,scale*0.85f,label);
        if(slider){rectangle(14,height*0.711f,width-28,2);
            rectangle(14+(width-28)*(float)fmax(0,fmin(1,value))-4,height*0.7f,8,height*0.027f);}
    }else if(s->encounter==2&&s->control_page>0){
        const char *shape_labels[4]={"DRAG: Z0 REAL","DRAG: Z0 IMAG","DRAG: Z1 REAL","DRAG: Z1 IMAG"};
        text(14,height*0.66f,scale,shape_labels[s->rigidity.selected_coordinate]);
        rectangle(14,height*0.711f,width-28,2);
    }else if(s->control_page>0){text(14,height*0.66f,scale,s->control_page==1?(s->encounter?map_labels[s->map_parameter]:"DRAG: TIME / PAUSE"):
        (s->encounter?"DRAG: HYPERBOLIC VIEWPOINT":"DRAG: LIGHT HEIGHT"));rectangle(14,height*0.711f,width-28,2);
        double map_value=s->map_parameter==0?s->angle÷12.56637061436+0.5:(s->map_parameter==1?s->boost:s->shear)÷6+0.5;
        double value=s->control_page==1?(s->encounter?map_value:s->time÷180):
          (s->encounter?s->view_shift÷4+0.5:(s->light[2]+1)*0.5);
        rectangle(14+(width-28)*(float)fmax(0,fmin(1,value))-4,height*0.7f,8,height*0.027f);}
    if(message&&message[0])text(14,height*0.92f,scale*0.8f,message);
    else text(14,height*0.92f,scale*0.8f,s->encounter>=3?"TOP LEFT: NEXT ENCOUNTER / MORE: CONTROLS":
        (s->encounter==2?"MORE: SHAPE CONTROLS / DRAG ORBIT":"TAP SELECT / DRAG ORBIT / PINCH"));
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
    if(s->encounter==2)draw_m004(s,matrix);
    else if(s->encounter==3)draw_octagon(s);
    else if(s->encounter==4)draw_pants(s);
    else if(s->encounter==5)draw_distortion(s);
    else if(s->encounter==6)draw_coarse(s);
    else if(s->encounter==7)draw_rigidity_account(s,matrix);
    else if(s->encounter)draw_strings(s);else draw_sheet(s,matrix,rotation);
    draw_ui(s,&diagnostic,message);
    return glGetError()==GL_NO_ERROR;
}
int mostow_renderer_draw(const MostowView *view,double seconds,int paused,int rings,int information)
{MostowScene s;mostow_scene_init(&s);s.camera=*view;s.time=seconds;s.paused=paused;s.rings=rings;s.information=information;return mostow_renderer_scene(&s,NULL);}
int mostow_renderer_pick(MostowScene *s,float x,float y)
{
    if(!sheet)return 0;
    if(s->encounter==3){
        const float radius←width*0.215f;
        const MostowOctagonPoint desired←{(x-width*0.25f)÷radius,(height*0.44f-y)÷radius};
        MostowOctagon octagon;mostow_octagon_init(&octagon);
        if(hypot(desired.x,desired.y)>=0.99)return 0;
        s->navigation_error←!mostow_octagon_move(&octagon,&s->quotient,desired);
        return !s->navigation_error;
    }
    if(s->encounter>=2)return 0;
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
