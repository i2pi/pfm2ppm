#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <ctype.h>
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

typedef struct {
    float       black_input;
    float       black_output;
    float       white_input;
    float       white_output;

    float       mid_input;
    float       lower_gamma;
    float       upper_gamma; 
} correctionT;

char            *in_fname;
char            *out_fname;

imageT          image, orig_image;
correctionT     corrections;

float apply_correction_inner (correctionT *c, float x) {
    if (x < c->mid_input) {
        x = powf(x, c->lower_gamma);
    } else {
        x= powf(x, c->upper_gamma) + (powf(c->mid_input, c->lower_gamma) - powf(c->mid_input, c->upper_gamma));
    }

    if (x <= c->black_input) return(c->black_output);
    if (x >= c->white_input) return(c->white_output);

    return(x);
}

float apply_correction(correctionT *c, float x) {

    x = apply_correction_inner(c, x);

    return (x);
}

void init_corrections(correctionT *c) {
    c->black_input = 0.0;
    c->black_output = 0.0;
    c->white_input = 1.0;
    c->white_output = 1.0;      

    c->mid_input = 0.5;
    c->lower_gamma = 1.0;       
    c->upper_gamma = 1.0;
}
    

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

void clone_image(imageT *image, imageT *clone) {
    size_t n = sizeof(float) * CHANNELS * image->width * image->height;

    clone->rgb = (float *) malloc (n);
    if (!clone->rgb) {
        fprintf (stderr, "Failed to allocate memory for cloning image\n");
        exit (-1);
    }
    memcpy(clone->rgb, image->rgb, n);

    clone->width = image->width;
    clone->height = image->height;

    memcpy(clone->stats, image->stats, sizeof(statsT) * CHANNELS);
}



void float_to_char_image (imageT *img, correctionT *corr, char *rgb_c) {
    int i;

    for (i=0; i<img->width * img->height * 3; i++) {
        float x;
        x = apply_correction(corr, img->rgb[i]);
        rgb_c[i] = (char) (x * 255.0);
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

void show_corrections(imageT *img, correctionT *c, float x1, float y1, float x2, float y2) {
    float       min, max;
    int         i;

    min = 1e9;
    max = -min; 

    for (i=0; i<CHANNELS; i++) {
        if (img->stats[i].min < min) min = img->stats[i].min;
        if (img->stats[i].max > max) max = img->stats[i].max;
    }

    glBegin(GL_LINE_STRIP);
    glLineWidth(2.0);
    glColor3f(10,10,10);

    for (i=0; i<HIST_BINS; i++) {
        float x = x1 + (x2 - x1) * (i/ (float)HIST_BINS);
        float y = y1 + (y2 - y1) * apply_correction(c, (max - min)*(i/(float)HIST_BINS) + min);
        glVertex2d (x, y);
    }
    glEnd();

    glLineWidth(1.0);
    glColor3f(3,3,3);
    glBegin(GL_LINES);
    glVertex2d(x1 + (x2 - x1) * (c->mid_input - min) / (max - min), y1);
    glVertex2d(x1 + (x2 - x1) * (c->mid_input - min) / (max - min), y2);
    glVertex2d(x1 + (x2 - x1) * (c->black_input - min) / (max - min), y1);
    glVertex2d(x1 + (x2 - x1) * (c->black_input - min) / (max - min), y2);
    glVertex2d(x1 + (x2 - x1) * (c->white_input - min) / (max - min), y1);
    glVertex2d(x1 + (x2 - x1) * (c->white_input - min) / (max - min), y2);
    glEnd();

    glLineWidth(1.0);
    glColor3f(3,3,0);
    glBegin(GL_LINES);
    glVertex2d(x1 + (x2 - x1) * (min) / (max - min), y1);
    glVertex2d(x1 + (x2 - x1) * (1.0 - min) / (max - min), y2);
    glEnd();
}

typedef enum {
    BLACK,
    WHITE,
    LOWER,
    UPPER,
    MIDLE
} gui_modeT;

int gui_keys(unsigned char key, correctionT *c) {
    static gui_modeT gui_mode = MIDLE;

    float x,y;

    x = y = 0;
    
    switch(tolower(key)){
        case 'b': gui_mode = BLACK; break;
        case 'w': gui_mode = WHITE; break;
        case 'l': gui_mode = LOWER; break;
        case 'u': gui_mode = UPPER; break;
        case 'm': gui_mode = MIDLE; break;
        case ',': x--; break;
        case '.': x++; break;
        case '=': y++; break;
        case '-': y--; break;
        case ' ': float_to_char_image (&image, &corrections, SCREEN_PIXELS); break;
        case 's': {
            save_screen(out_fname, SCREEN_PIXELS, image.width, image.height);
            printf ("Saved %s\n", out_fname);
        } break;
        default: return (0);
    }

    x /= 100.0;
    y /= 100.0;

    switch(gui_mode) {
        case BLACK: {c->black_input += x; c->black_output += y; } break;
        case WHITE: {c->white_input += x; c->white_output += y; } break;
        case UPPER: {c->upper_gamma += y; } break;
        case LOWER: {c->lower_gamma += y; } break;
        case MIDLE: {c->mid_input += x; } break;
    }

    printf ("BLACK: %6.4f %6.4f\n", c->black_input, c->black_output);
    printf ("WHITE: %6.4f %6.4f\n", c->white_input, c->white_output);
    printf ("GAMMA: %6.4f %6.4f %6.4f\n", c->lower_gamma, c->mid_input, c->upper_gamma);


    return (1);
}


void	render_scene(void)
{
    unsigned char key;

    key = get_last_key();

    gui_keys(key, &corrections);

	// Render GL version
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

    show_histograms(&orig_image, -1, 0.5, 1, 1.0);

    show_corrections(&image, &corrections, -1, -0.5, 1, 0.5);

    show_histograms(&image, -1, -1, 1, -0.5);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glViewport(gui_state.w/2, 0, gui_state.w/2, gui_state.h);		

    switch (tolower(key)) {
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
    glBindTexture(GL_TEXTURE_2D, 0); 
	
    glutSwapBuffers();	
}


int main(int argc, char **argv)
{  
    init_gl (argc, argv);
    init_screen ();

    if (argc < 2) {
        fprintf (stderr, "%s input_file.pfm\n", argv[0]);
        exit(-1);
    }

    in_fname = strdup(argv[1]);

    out_fname = (char *) malloc (strlen(in_fname) + 10);
    snprintf(out_fname, strlen(in_fname)+8, "%s.ppm", in_fname);

    load_pfm(in_fname, &orig_image);
    clone_image(&orig_image, &image);

    SCREEN_WIDTH = image.width;
    SCREEN_HEIGHT = image.height;

    SCREEN_PIXELS = (char *) calloc (sizeof(char), SCREEN_WIDTH * SCREEN_HEIGHT * 3);

    init_corrections(&corrections);

    
    float_to_char_image (&image, &corrections, SCREEN_PIXELS);
  
    glutMainLoop();  

    return (1);
}
