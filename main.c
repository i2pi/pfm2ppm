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

#define CHANNELS 3

#define HIST_BINS 1024

uint32_t  SCREEN_WIDTH, SCREEN_HEIGHT;

char *SCREEN_PIXELS;

typedef struct {
    float   min;
    float   max;
    float   histogram[HIST_BINS];
} statsT;

typedef struct imageT {
    float       *rgb;
    int         width;
    int         height;
    statsT      stats[CHANNELS];
} imageT;

imageT  image;

void char_swap(char *a, char *b) {
    char t;
    t = *a;
    *a = *b;
    *b = t;
}

void reset_stats(statsT *s) {
    int i;

    s->min = s->max = 0.0f;
    for (i=0; i<HIST_BINS; i++) s->histogram[i] = 0.0;
}

void update_stats(statsT *s, float val) {
    if (val < s->min) s->min = val;
    if (val > s->max) s->max = val;
}

void calc_stats (imageT *img) {
    uint32_t i, pixels;
    uint32_t    max_bin_count[CHANNELS];

    pixels = img->width * img->height;

    for (i=0; i<CHANNELS; i++) {
        reset_stats(&img->stats[i]);
        max_bin_count[i] = 0;
    }

    for (i=0; i<CHANNELS*pixels; i++) {
        update_stats(&img->stats[i % CHANNELS], img->rgb[i]);
    }

    for (i=0; i<CHANNELS*pixels; i++) {
        int ch = i%CHANNELS;
        statsT *s = &img->stats[ch];
        int bin = ((HIST_BINS-1) * (img->rgb[i] - s->min)) / (s->max - s->min);
        s->histogram[bin]++;
        if (s->histogram[bin] > max_bin_count[ch]) max_bin_count[ch] = s->histogram[bin];
    }

    for (int ch=0; ch<CHANNELS; ch++) 
    for (i=0; i<HIST_BINS; i++) {
        img->stats[ch].histogram[i] /= (float) max_bin_count[ch];
    }

    for (i=0; i<CHANNELS; i++) {
        printf ("%d: %6.4f --> %6.4f\n", i, img->stats[i].min, img->stats[i].max);
    }
}

void load_pfm(const char *fname, imageT *img) {
    FILE *fp;
    char buf[1024];
    int byteorder;
    uint32_t size;

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

    if (fscanf (fp, "%d %d\n%d\n", &img->width, &img->height, &byteorder) != 3) {
        fprintf (stderr, "Failed to read width, height, byteorder from '%s'\n", fname);
        exit (-1);
    }

    if (byteorder != -1) {
        fprintf (stderr, "Unsupported byteorder %d\n", byteorder);
        exit (-1);
    }

    size = CHANNELS * img->width * img->height;

    img->rgb = (float *) malloc (sizeof(float) * size);
    if (!img->rgb) {
        fprintf (stderr, "Failed to allocate memory for image buffer\n");
        exit (-1);
    }
    uint32_t n;
    n = fread (img->rgb, sizeof(float), size, fp);

    if (n != size) {
        fprintf (stderr, "Failed to read image data %u!= %u\n", n, size);
        exit (-1);
    }
/*
    for (int i=0; i<size; i++) {
        char *c = &rgb[i];
    
        char_swap(&c[0], &c[3]);
        char_swap(&c[1], &c[2]);
    }
*/

    calc_stats(img);
}

void float_to_char_image (imageT *img, char *rgb_c) {
    int i;

    for (i=0; i<img->width * img->height * 3; i++) {
        float x;
        x = img->rgb[i] * 255.0;
        if (x<0) x = 0;
        if (x>255) x=255;
        rgb_c[i] = (char) x;
    }
}

void init_screen(void) {
    init_texture_for_pixels(SCREEN_TEXTURE_ID);
}

void show_histograms(imageT *img, float x1, float y1, float x2, float y2) {
    int c, i;

    for (c=0; c<CHANNELS; c++) {
        glLineWidth(2.0);
        glBegin(GL_LINE_STRIP);
        switch (c) {
            case 0: glColor3f(10,0,0); break;
            case 1: glColor3f(0,10,0); break;
            case 2: glColor3f(0,0,10); break;
            default: glColor3f(1,1,1);
        }
        
    
        for (i=0; i<HIST_BINS; i++) {
            float x = x1 + (x2 - x1) * i / (float) HIST_BINS;
            float y = y1 + (y2 - y1) * img->stats[c].histogram[i];
            glVertex2d (x,y);
        }
        glEnd();
    }
    
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
	glTranslatef(0,0,-2.5);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glViewport(0, 0, gui_state.w/2, gui_state.h);		

	glClearColor(0,0,0,0);
	glClearDepth(1);				
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


    show_histograms(&image, -1, -0.5, 1, 0);


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

    init_gl (argc, argv);
    init_screen ();

    load_pfm("test.pfm", &image);

    SCREEN_WIDTH = image.width;
    SCREEN_HEIGHT = image.height;

    SCREEN_PIXELS = (char *) calloc (sizeof(char), SCREEN_WIDTH * SCREEN_HEIGHT * 3);

    float_to_char_image (&image, SCREEN_PIXELS);
    
  
    glutMainLoop();  

    return (1);
}
