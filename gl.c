#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "gl.h"

#define ESCAPE 27

#define PI 3.14159265

#define MAX_SEGMENTS 16384

float	tick_length = 0.05;


void array_to_vector(float *arr, vectorT *v) {
	v->x = arr[0];
	v->y = arr[1];
	v->z = arr[2];
}

void vector_to_array(vectorT *v, float *arr) {
	arr[0] = v->x;
	arr[1] = v->y;
	arr[2] = v->z;
}

void params_to_array(float x, float y, float z, float *arr) {
	arr[0] = x;
	arr[1] = y;
	arr[2] = z;
}

void params_to_vector(float x, float y, float z, vectorT *v) {
	v->x = x;
	v->y = y;
	v->z = z;
}

float length_vector (vectorT *v) {
	// |v|
	float len;
	len = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
	return (len);	
}

void normalize_vector (vectorT *v) {
	// v / |v|
	float len = length_vector(v);
	v->x /= len;
	v->y /= len;
	v->z /= len;
}

void diff_vector (vectorT *a, vectorT *b, vectorT *v) {
	// v = a - b
	v->x = a->x - b->x;
	v->y = a->y - b->y;
	v->z = a->z - b->z;
}

float dist_vector (vectorT *a, vectorT *b) {
	vectorT d;
	diff_vector(a, b, &d);
	return (length_vector(&d));
}

float	dot_vector (vectorT *a, vectorT *b) {
	// a . b
	float d;
	d = a->x * b->x + 
		a->y * b->y +
		a->z * b->z;
	return (d);
}

float	cosine_vector (vectorT *a, vectorT *b) {
	// (a . b) / (|a| |b|)
	return (dot_vector(a,b) / (length_vector(a) * length_vector(b)));
}

void cross_vector(vectorT *a, vectorT *b, vectorT *v) {
	// v = a x b
	v->x = a->y * b->z - b->y * a->z; 
	v->y = a->z * b->x - b->z * a->x; 
	v->z = a->x * b->y - b->x * a->y;
}

void project_vector (vectorT *a, vectorT *b, vectorT *v) {
	// v = (a . b) / |a| * a;
	float 	d = dot_vector(a, b) / length_vector(a);
	*v = *a;
	v->x *= d;
	v->y *= d;
	v->z *= d;
}

void scale_vector (vectorT *v, float s) {
	// v = s * v
	v->x *= s;
	v->y *= s;
	v->z *= s;
}

void scale_offset_vector (vectorT *a, vectorT *d, float s, vectorT *v) {
	// v = a + (s * d)

	v->x = a->x + s * d->x;
	v->y = a->y + s * d->y;
	v->z = a->z + s * d->z;
}

void reflect_vector (vectorT *v, vectorT *n, vectorT *r) {
	// r = v - 2*n*(v . n)
	float	dot;
	vectorT a;

	a = *n;

	dot = dot_vector(v, n);

	scale_vector(&a, 2.0 * dot);
	diff_vector(v, &a, r);
	normalize_vector(r);
}

char refract_vector(vectorT *v, vectorT *n, float n1, float n2, vectorT *r) {
	// r = refraction of v against normal n on boundary of n1:n2

	// http://steve.hollasch.net/cgindex/render/refraction.txt
	
   //Vector3  I, N, T;		/* incoming, normal and Transmitted */
	double eta, c1, cs2 ;

	eta = n1 / n2 ;			
	c1 = -dot_vector(v, n);
	cs2 = 1.0 - pow(eta, 2.0) * (1 - pow(c1, 2.0));

	if (cs2 < 0)
		return (0);		// total internal reflection 
	
	r->x = eta * v->x + (eta * c1 - sqrt(cs2)) * n->x;
	r->y = eta * v->y + (eta * c1 - sqrt(cs2)) * n->y;
	r->z = eta * v->z + (eta * c1 - sqrt(cs2)) * n->z;

	return (1);
}

void triangle_normal_vector (vectorT *a, vectorT *b, vectorT *c, vectorT *n) {
	vectorT v1, v2;

	diff_vector(a, b, &v1);
	diff_vector(a, c, &v2);
	cross_vector(&v1, &v2, n);
	normalize_vector(n);
}

void set_camera (void) {
	float	fov = 45.0f;
	float	aspect = 1.0f;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();			
	gluPerspective(fov,aspect,0.1f,100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();			
}

void gl_positional_light(GLenum light, float x, float y, float z, float *color) {
	
	float pos[4];
#ifndef WIREFRAME
	float nil[4] = {0,0,0,0};
#endif
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = 1.0; // Indicates this is a positional light

#ifndef WIREFRAME
	glEnable(light);                   	
	glLightfv (light, GL_POSITION, pos);
	glLightfv (light, GL_DIFFUSE, color);
	glLightfv (light, GL_SPECULAR, color);
	glLightfv (light, GL_AMBIENT, nil);
#endif
}

void ReSizeGLScene(int Width, int Height)
{
	if (Height==0)	Height=1;

	glViewport(0, 0, Width/2, Height);		
	set_camera();
	gui_state.w = Width;
	gui_state.h = Height;
}

void	print (float x, float y, char *text)
{
	char	*p;
	glRasterPos2d(x,y);
	p = text;
	while (*p) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
		p++;
	}
}

void keyPressed(unsigned char key, int x, int y) 
{
    usleep(100);

    if (key == ESCAPE) 
    { 
      glutDestroyWindow(gui_state.window); 
      exit(0);                   
    }

	gui_state.last_key = key;
}

unsigned char get_last_key (void) {
	unsigned char c = gui_state.last_key;

	gui_state.last_key = '\0';

	return (c);
}

void gl_sphere (float x, float y, float z, float r, int segments) {
	vectorT p_circle[MAX_SEGMENTS];
	vectorT p_norm[MAX_SEGMENTS];
	vectorT circle[MAX_SEGMENTS];
	vectorT norm[MAX_SEGMENTS];
	int		i, j;

	for (i=0; i<segments; i++) {
		p_circle[i].x = x;
		p_circle[i].y = y - r;
		p_circle[i].z = z;
		p_norm[i].x = 0;
		p_norm[i].y = -1;
		p_norm[i].z = 0;
	}	

	for (i=1; i<segments; i++) {
		float h = r * ((2.0f * i / (float) (segments-1)) - 1.0f);
		float R = sqrt(r*r - h*h);
		for (j=0; j<segments; j++) {
			float t = (2.0f * PI * j / (float) segments);
			circle[j].x = x + R*sin(t);
			circle[j].y = y + h;
			circle[j].z = z + R*cos(t);

			norm[j].x = sin(t);
			norm[j].y = h / r;
			norm[j].z = cos(t);
			normalize_vector(&norm[j]); 
		}

		for (j=0; j<segments; j++) {
			int	n = (j+1) % segments;	

			float	vertex[9];	
			float	normal[9];

			// pointing up
			vector_to_array(&norm[j], &normal[0]);
			vector_to_array(&circle[j], &vertex[0]);

			vector_to_array(&norm[n], &normal[3]);
			vector_to_array(&circle[n], &vertex[3]);

			vector_to_array(&p_norm[j], &normal[6]);
			vector_to_array(&p_circle[j], &vertex[6]);
			gl_triangle (vertex, normal);

			// pointing down
			vector_to_array(&p_norm[j], &normal[0]);
			vector_to_array(&p_circle[j], &vertex[0]);

			vector_to_array(&p_norm[n], &normal[3]);
			vector_to_array(&p_circle[n], &vertex[3]);

			vector_to_array(&norm[n], &normal[6]);
			vector_to_array(&circle[n], &vertex[6]);
			gl_triangle (vertex, normal);
		}

		memcpy (p_circle, circle, sizeof(vectorT) * segments);
		memcpy (p_norm, norm, sizeof(vectorT) * segments);
	}
}


void    gl_axes_wireframe (float x, float y, float z) {
    glBegin(GL_LINES);
	glColor4f(1,0,0,1);
    glVertex3f(x, y, z);
    glVertex3f(x+tick_length, y, z);
    glEnd();

    glBegin(GL_LINES);
	glColor4f(0,1,0,1);
    glVertex3f(x, y, z);
    glVertex3f(x, y+tick_length, z);
    glEnd();

    glBegin(GL_LINES);
	glColor4f(0,0,1,1);
    glVertex3f(x, y, z);
    glVertex3f(x, y, z+tick_length);
    glEnd();
}

void gl_show_ray_tick (float vertex[3], float normal[3]) {
	float s = tick_length;

	glBegin(GL_LINES);
	glColor4f(1,1,0,1);
	glVertex3f(vertex[0], vertex[1], vertex[2]);
	glVertex3f(vertex[0]+s*normal[0], 
		vertex[1]+s*normal[1], 
		vertex[2]+s*normal[2]);
	glEnd();
}

void gl_show_ray (float ox, float oy, float oz, float dx, float dy, float dz) {
	float s = 100;

	glBegin(GL_LINES);
	glColor4f(1,1,1,1);
	glVertex3f(ox, oy, oz);
	glVertex3f(ox+s*dx, oy+s*dy, oz+s*dz);
	glEnd();
}


void gl_triangle(float *vertex, float *normal) {
#ifndef WIREFRAME
	glBegin(GL_TRIANGLES);
#else
	glBegin(GL_LINES);
#endif
	glColor4f(0,1,0,1);
	glVertex3f(vertex[0], vertex[1], vertex[2]);
	glNormal3f(vertex[0], vertex[1], vertex[2]);
	glVertex3f(vertex[3], vertex[4], vertex[5]);
	glNormal3f(vertex[3], vertex[4], vertex[5]);
	glVertex3f(vertex[6], vertex[7], vertex[8]);
	glNormal3f(vertex[6], vertex[7], vertex[8]);
#ifdef WIREFRAME
	glVertex3f(vertex[0], vertex[1], vertex[2]);
	glNormal3f(vertex[0], vertex[1], vertex[2]);
#endif
	glEnd();

#ifdef WIREFRAME
	gl_show_ray_tick(&vertex[0], &normal[0]);
#endif
}

void gl_ortho_plane (float nx, float ny, float nz, float pos, float W, int N) {
	float x, y, z;
	float step;
	float n[3], v[3];

	int	i, j;

	params_to_array(nx, ny, nz, n);

	step = (2.0f * W) / (float) N;

	if (nx != 0) {
		// vertical plane
		for (i=0; i<N; i++)
		for (j=0; j<N; j++) {
			x = nx * pos;
			y = -W + i*step;
			z = -W + j*step;
			glBegin(GL_LINES);
			glColor4f(0,1,0,1);	
			glVertex3f(x, y, z);
			glVertex3f(x, y+step, z);
			glVertex3f(x, y, z+step);
			glVertex3f(x, y, z);
			glEnd();	
			params_to_array(x,y,z, v);
			gl_show_ray_tick(v, n);
		}
	} else 
	if (ny != 0) {
		// horitzontal plane
		for (i=0; i<N; i++)
		for (j=0; j<N; j++) {
			x = -W + i*step;
			y = ny * pos;
			z = -W + j*step;
			glBegin(GL_LINES);
			glColor4f(0,1,0,1);	
			glVertex3f(x, y, z);
			glVertex3f(x+step, y, z);
			glVertex3f(x, y, z+step);
			glVertex3f(x, y, z);
			glEnd();	
			params_to_array(x,y,z, v);
			gl_show_ray_tick(v, n);
		}
	} else {
		// coronal plane
		for (i=0; i<N; i++)
		for (j=0; j<N; j++) {
			x = -W + i*step;
			y = -W + j*step;
			z = nz * pos;
			glBegin(GL_LINES);
			glColor4f(0,1,0,1);	
			glVertex3f(x, y, z);
			glVertex3f(x+step, y, z);
			glVertex3f(x, y+step, z);
			glVertex3f(x, y, z);
			glEnd();	
			params_to_array(x,y,z, v);
			gl_show_ray_tick(v, n);
		}
	}
}

void gl_cube(float x, float y, float z, float d)
{
  glBegin(GL_QUADS);                // start drawing the cube.

  // top of cube
  glVertex3f(x+ d,y+ d,z+-d);       // Top Right Of The Quad (Top)
  glVertex3f(x+-d,y+ d,z+-d);       // Top Left Of The Quad (Top)
  glVertex3f(x+-d,y+ d,z+ d);       // Bottom Left Of The Quad (Top)
  glVertex3f(x+ d,y+ d,z+ d);       // Bottom Right Of The Quad (Top)

  // bottom of cube
  glVertex3f(x+ d,y+-d,z+ d);       // Top Right Of The Quad (Bottom)
  glVertex3f(x+-d,y+-d,z+ d);       // Top Left Of The Quad (Bottom)
  glVertex3f(x+-d,y+-d,z+-d);       // Bottom Left Of The Quad (Bottom)
  glVertex3f(x+ d,y+-d,z+-d);       // Bottom Right Of The Quad (Bottom)

  // front of cube
  glVertex3f(x+ d,y+ d,z+ d);       // Top Right Of The Quad (Front)
  glVertex3f(x+-d,y+ d,z+ d);       // Top Left Of The Quad (Front)
  glVertex3f(x+-d,y+-d,z+ d);       // Bottom Left Of The Quad (Front)
  glVertex3f(x+ d,y+-d,z+ d);       // Bottom Right Of The Quad (Front)

  // back of cube.
  glVertex3f(x+ d,y+-d,z+-d);       // Top Right Of The Quad (Back)
  glVertex3f(x+-d,y+-d,z+-d);       // Top Left Of The Quad (Back)
  glVertex3f(x+-d,y+ d,z+-d);       // Bottom Left Of The Quad (Back)
  glVertex3f(x+ d,y+ d,z+-d);       // Bottom Right Of The Quad (Back)

  // left of cube
  glVertex3f(x+-d,y+ d,z+ d);       // Top Right Of The Quad (Left)
  glVertex3f(x+-d,y+ d,z+-d);       // Top Left Of The Quad (Left)
  glVertex3f(x+-d,y+-d,z+-d);       // Bottom Left Of The Quad (Left)
  glVertex3f(x+-d,y+-d,z+ d);       // Bottom Right Of The Quad (Left)

  // Right of cube
  glVertex3f(x+ d,y+ d,z+-d);           // Top Right Of The Quad (Right)
  glVertex3f(x+ d,y+ d,z+ d);       // Top Left Of The Quad (Right)
  glVertex3f(x+ d,y+-d,z+ d);       // Bottom Left Of The Quad (Right)
  glVertex3f(x+ d,y+-d,z+-d);       // Bottom Right Of The Quad (Right)
  glEnd();                  // Done Drawing The Cube
}

void gl_xy_sphere_cap (float r, float R, float z, int segments) {

	// r = radius of curvature
	// R = radius of cap
	// z = position on z-axis

	int		i, j;
	float	max_theta;
	float	theta, phi;
	vectorT p_circle[MAX_SEGMENTS];
	vectorT p_norm[MAX_SEGMENTS];		// TODO
	vectorT circle[MAX_SEGMENTS];
	vectorT norm[MAX_SEGMENTS];

	max_theta = asinf (R / r);

	for (i=0; i<segments; i++) {
		phi = i * 2.0f * PI / (float) (segments-1);
		p_circle[i].x = R * cos(phi);
		p_circle[i].y = R * sin(phi);
		p_circle[i].z = z;
	}

	for (i=1; i<segments; i++) {
		// phi goes from 0 to 2pi
		// theta goes from asin(R/r) to 0
		float	RR; // radius of this segment

		theta = max_theta * (1.0 - (i / (float) (segments-1)));
		RR = r * sin(theta);

		for (j=0; j<segments; j++) {
			phi = j * 2.0f * PI / (float) (segments-1);

			circle[j].x = RR * cos(phi);
			circle[j].y = RR * sin(phi);
			circle[j].z = z + r * cos(theta) - r * cos (max_theta);
		}

		for (j=0; j<segments; j++) {
			int	n = (j+1) % segments;	

			float	vertex[9];	
			float	normal[9];

			// pointing up
			vector_to_array(&norm[j], &normal[0]);
			vector_to_array(&circle[j], &vertex[0]);

			vector_to_array(&norm[n], &normal[3]);
			vector_to_array(&circle[n], &vertex[3]);

			vector_to_array(&p_norm[j], &normal[6]);
			vector_to_array(&p_circle[j], &vertex[6]);
			gl_triangle (vertex, normal);

			// pointing down
			vector_to_array(&p_norm[j], &normal[0]);
			vector_to_array(&p_circle[j], &vertex[0]);

			vector_to_array(&p_norm[n], &normal[3]);
			vector_to_array(&p_circle[n], &vertex[3]);

			vector_to_array(&norm[n], &normal[6]);
			vector_to_array(&circle[n], &vertex[6]);
			gl_triangle (vertex, normal);
		}

		memcpy (p_circle, circle, sizeof(vectorT) * segments);
		memcpy (p_norm, norm, sizeof(vectorT) * segments);
	}
}


void init_texture_for_pixels (int tex_id) {
	glBindTexture(GL_TEXTURE_2D, tex_id);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

void draw_pixels_to_texture (char *pixels, int w, int h, int tex_id) {
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

void init_gl(int argc, char **argv)
{
	int	Width = 2048;
	int Height = 1024;

	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);

    glutInitWindowSize(Width, Height);

    gui_state.window = glutCreateWindow(argv[0]);
    glutDisplayFunc(render_scene);
    glutIdleFunc(render_scene);
    glutReshapeFunc(&ReSizeGLScene);
    glutKeyboardFunc(&keyPressed);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); 
	glDepthFunc(GL_LESS);			   
	glEnable(GL_DEPTH_TEST);		  
	glShadeModel(GL_SMOOTH);		
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef WIREFRAME
	glEnable(GL_LIGHTING);                 	//enables lighting
#endif
	glEnable(GL_COLOR_MATERIAL);

	gui_state.w = Width;
	gui_state.h = Height;

	set_camera();
}

void save_screen (int frame, char *rgb, int width, int height)
{
    static unsigned char *screen = NULL;
    FILE    *fp;
    char    str[256];

    if (!screen) {
        screen = (unsigned char *) malloc (sizeof (unsigned char) * width * height * 4);
    }
    if (!screen) {
        fprintf (stderr, "Failed to malloc screen\n");
        exit (-1);
    }

    snprintf (str, 250, "frame%08d.ppm", frame);
    fp = fopen (str, "w");
    fprintf (fp, "P6\n");
    fprintf (fp, "%d %d 255\n", width, height);

	fwrite (rgb, 1, width*height*3, fp);

    fclose (fp);
}

void save_screen_f (int frame, float *rgb, int width, int height)
{
    static unsigned char *screen = NULL;
    FILE    *fp;
    char    str[256];

    if (!screen) {
        screen = (unsigned char *) malloc (sizeof (unsigned char) * width * height * 4);
    }
    if (!screen) {
        fprintf (stderr, "Failed to malloc screen\n");
        exit (-1);
    }

    snprintf (str, 250, "frame%08d.float", frame);
    fp = fopen (str, "w");
    fprintf (fp, "PF4\n");
    fprintf (fp, "%d %d\n-1\n", width, height);

	fwrite (rgb, sizeof(float), width*height*3, fp);

    fclose (fp);
}
