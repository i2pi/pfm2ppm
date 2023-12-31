#ifndef __GL_H__
#define __GL_H__

#ifdef MAC
	#include <GLUT/glut.h>
	#include <OpenGL/gl.h>
#else
	#include <GL/glut.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif


typedef struct {
	float	x, y, z;
} vectorT;


typedef struct gui_stateT
{
    int window;

    int w, h;

    struct {
        double x, y;
        int button, state;
    } mouse;

	unsigned char last_key;
} gui_stateT;

gui_stateT  gui_state;

extern void render_scene (void);

void normalize_vector (vectorT *v);
void params_to_vector(float x, float y, float z, vectorT *v);
void vector_to_array(vectorT *v, float *arr);

void init_gl (int argc, char **argv);
void gl_sphere (float x, float y, float z, float r, int segments);
void gl_cube(float x, float y, float z, float d);
void gl_triangle(float *vertex, float *normal);
void gl_positional_light(GLenum light, float x, float y, float z, float *color);
void gl_xy_sphere_cap (float r, float R, float z, int segments);
void gl_ortho_plane (float nx, float ny, float nz, float pos, float W, int N);
void gl_axes_wireframe (float x, float y, float z);
void gl_show_ray (float ox, float oy, float oz, float dx, float dy, float dz);
void set_camera();

unsigned char get_last_key (void);

void init_texture_for_pixels (int tex_id);
void draw_pixels_to_texture (char *pixels, int w, int h, int tex_id);
void save_screen (char *fname, char *rgb, int width, int height);
void save_screen_f (char *fname, float *rgb, int width, int height);


#endif
