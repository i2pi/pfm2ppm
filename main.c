#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "gl.h"

#define ESCAPE 27

#define SCREEN_TEXTURE_ID	1

uint32_t  SCREEN_WIDTH, SCREEN_HEIGHT;

char *SCREEN_PIXELS;

void char_swap(char *a, char *b) {
    char t;
    t = *a;
    *a = *b;
    *b = t;
}

float *load_pfm(const char *fname, int *width, int *height) {
    FILE *fp;
    char buf[1024];
    int byteorder;
    uint32_t size;
    float *rgb;

    fp = fopen (fname, "r");
    if (!fp) {
        fprintf (stderr, "Failed to open '%s'\n", fname); 
        exit (-1);
    }

    if (!fgets(buf, 1000, fp)) {
        fprintf (stderr, "Failed to read first line from '%s'\n", fname);
        exit (-1);
    }

    if (strnstr(buf, "PF4", 1000) != buf) {
        fprintf(stderr, "Read '%s' as header instead of 'PF4'\n", buf);
        exit (-1);
    }

    if (fscanf (fp, "%d %d\n%d\n", width, height, &byteorder) != 3) {
        fprintf (stderr, "Failed to read width, height, byteorder from '%s'\n", fname);
        exit (-1);
    }

    if (byteorder != -1) {
        fprintf (stderr, "Unsupported byteorder %d\n", byteorder);
        exit (-1);
    }

    size = 3 * *width * *height;

    rgb = (float *) malloc (sizeof(float) * size);
    if (!rgb) {
        fprintf (stderr, "Failed to allocate memory for image buffer\n");
        exit (-1);
    }
    size_t n;
    n = fread (rgb, sizeof(float), size, fp);

    if (n != size) {
        fprintf (stderr, "Failed to read image data %ld != %ld\n", n, size);
        exit (-1);
    }
/*
    for (int i=0; i<size; i++) {
        char *c = &rgb[i];
    
        char_swap(&c[0], &c[3]);
        char_swap(&c[1], &c[2]);
    }
*/

    printf ("%6.4f\n", rgb[0]);

    unsigned char *c = (char *) rgb;

    for (int i=0; i<16; i++) {
        printf ("%02x ", c[i]);
    }
    printf ("\n");

    return (rgb);
}

void float_to_char_image (float *rgb_f, int width, int height, char *rgb_c) {
    int i;

    for (i=0; i<width * height * 3; i++) {
        float x;
        x = rgb_f[i] * 255.0;
        if (x<0) x = 0;
        if (x>255) x=255;
        rgb_c[i] = (char) x;
    }
}

void init_screen(void) {
    init_texture_for_pixels(SCREEN_TEXTURE_ID);
}


void	render_scene(void)
{
	unsigned char key;


	key = get_last_key();

	// Render GL version
//	set_camera();
	float   fov = 45.0f;
    float   aspect = 1.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov,aspect,0.1f,100.0f);
	glTranslatef(0,0,-10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glViewport(0, 0, gui_state.w/2, gui_state.h);		

	glClearColor(0,0,0,0);
	glClearDepth(1);				
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	glViewport(gui_state.w/2, 0, gui_state.w/2, gui_state.h);		

	switch (key) {
	}

	draw_pixels_to_texture(SCREEN_PIXELS, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_TEXTURE_ID);

	glBindTexture(GL_TEXTURE_2D, SCREEN_TEXTURE_ID);
	glColor4f(0,0,0,1);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2f  (-1,-1);
	glTexCoord2f(1,0);
	glVertex2f  ( 1,-1);
	glTexCoord2f(1,1);
	glVertex2f  ( 1, 1);
	glTexCoord2f(0,1);
	glVertex2f  (-1, 1);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures
	
	glutSwapBuffers();	

}


int main(int argc, char **argv)
{  
    int width, height, i;
    float *rgb = 0;


    init_gl (argc, argv);
    init_screen ();

    rgb = load_pfm("test.pfm", &width, &height);

    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;

    SCREEN_PIXELS = (char *) calloc (sizeof(char), SCREEN_WIDTH * SCREEN_HEIGHT * 3);

    float_to_char_image (rgb, width, height, SCREEN_PIXELS);
    
  
    glutMainLoop();  

    return (1);
}
