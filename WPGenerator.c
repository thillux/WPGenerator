#include <cairo.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <limits.h>
#include <math.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define M_PI  3.141592653589793
#define PHI   1.6180339887

// xorshift
double rng() {
  static uint32_t x = 123456789;
  static uint32_t y = 362436069;
  static uint32_t z = 521288629;
  static uint32_t w = 88675123;

  uint32_t t = x ^ (x << 11);
  x = y; y = z; z = w;
  w ^= (w >> 19) ^ t ^ (t >> 8);

  return (double) w / UINT_MAX;
}

int getSurfaceWidth(cairo_t* cr) {
    cairo_surface_t* surface = cairo_get_group_target(cr);
    return cairo_image_surface_get_width(surface);
}

int getSurfaceHeight(cairo_t* cr) {
    cairo_surface_t* surface = cairo_get_group_target(cr);
    return cairo_image_surface_get_height(surface);
}

void drawRandomCircles(cairo_t* cr) {
    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth = getSurfaceWidth(cr);

    // dark blue
    cairo_set_source_rgba(cr, 60.0/255.0, 76.0/255.0, 85.0/255.0, 0.3 * rng());
    cairo_set_line_width(cr, 2.0 * rng() + 3.0);
    cairo_arc(cr, rng() * surfaceWidth, rng() * surfaceHeight, rng() * 20, 0, 2.0 * M_PI);

    if (rng() > 0.3)
        cairo_stroke(cr);
    else 
        cairo_fill(cr);
}

void drawRandomSineWaves(cairo_t* cr) {
    cairo_new_path(cr);
    cairo_set_line_width(cr, 2.0);

    int surfaceHeight = getSurfaceHeight(cr);    
    int surfaceWidth = getSurfaceWidth(cr);

    double minPeriod = 5;
    double period1 = (minPeriod + rng()) * surfaceWidth;
    double period2 = 2 * period1;

    double startX = 0.0;
    double startY = surfaceHeight * rng();

    double phase1 = rng() * surfaceWidth;
    double phase2 = rng() * surfaceWidth;

    double minAmplitude = 5.0;
    double amplitude1 = minAmplitude + rng() * surfaceHeight / 5.0;
    double amplitude2 = minAmplitude + rng() * surfaceHeight / 5.0;

    cairo_move_to(cr, startX, startY + amplitude1 * sin(2 * M_PI * (phase1)/ period1));
    for (double x = 1; x < surfaceWidth; ++x) {
        cairo_line_to(cr, x, startY + amplitude1 * sin(2 * M_PI * ((double) x + phase1 )/ period1));
    }
    cairo_stroke(cr);

    // second wave in around golden ratio of height to create a more dense
    // look around the logo if someone sets a big number of sinewaves (eg. 10000)
    startX = 0;
    startY = 1.0 / PHI * surfaceHeight + (rng() - 0.5) * surfaceHeight / 4.0;
    cairo_move_to(cr, startX, startY + amplitude2 * sin(2 * M_PI * ((double) surfaceWidth - 1 + phase2)/ period2));
    cairo_new_path(cr);
    for (double x = 0; x < surfaceWidth; ++x) {
        cairo_line_to(cr, x, startY + amplitude2 * sin(2 * M_PI * ((double) x + phase2 )/ period2));
    }

    // arch blue
    cairo_set_source_rgba(cr, 23.0/255.0, 147.0/255.0, 209.0/255.0, 0.1 * rng());
    cairo_stroke(cr);
}

void drawArchLogo(cairo_t* cr, RsvgHandle* archLogoSVG) {
    RsvgDimensionData* logoDimensions =(RsvgDimensionData*) malloc(sizeof(RsvgDimensionData));
    rsvg_handle_get_dimensions(archLogoSVG, logoDimensions);

    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth  = getSurfaceWidth(cr);
    int borderDistance = 20;
    int logoWidth = logoDimensions->width;
    int logoHeight = logoDimensions->height;
    int rectY = 1.0 / PHI * surfaceHeight - logoHeight / 2;
    int logoOriginX = surfaceWidth - logoWidth - borderDistance;
    int logoOriginY = 1.0 / PHI * surfaceHeight - logoHeight / 2;

    // grey
    cairo_set_source_rgb(cr, 38.0/255.0, 39.0/255.0, 33.0/255.0);
    cairo_rectangle(cr, 0, rectY, surfaceWidth, logoHeight);
    cairo_fill(cr);

    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, 0, rectY);
    cairo_rel_line_to(cr, surfaceWidth, 0);
    cairo_stroke(cr);

    cairo_move_to(cr, 0, rectY + logoHeight);
    cairo_rel_line_to(cr, surfaceWidth, 0);
    cairo_stroke(cr);

    cairo_surface_t* surface = cairo_get_group_target(cr);
    cairo_surface_t* logoSurface = cairo_surface_create_for_rectangle(surface, logoOriginX, logoOriginY, logoWidth, logoHeight);
    cairo_t* logoContext = cairo_create(logoSurface);

    rsvg_handle_render_cairo(archLogoSVG, logoContext);
    rsvg_handle_close(archLogoSVG, NULL);
    g_object_unref(archLogoSVG);

    cairo_destroy(logoContext);
    cairo_surface_destroy(logoSurface);
    free(logoDimensions);
}

void usage(void) {
    printf("Usage: ./WPGenerator width height\n");
}

void checkArguments(int argc, char ** argv) {
    if (argc != 3) {
        usage();
        exit(EXIT_FAILURE);
    }

    regex_t regex;
    regcomp(&regex, "^[[:digit:]]+$", REG_EXTENDED);

    for (int argument = 1; argument <= 2; ++argument) {
        if(regexec(&regex, argv[argument], 0, NULL, 0) == REG_NOMATCH) {
            usage();
            exit(EXIT_FAILURE);
        }
    }

    regfree(&regex);
}

int main(int argc, char ** argv) {
    checkArguments(argc, argv);

    int wallpaperWidth = atoi(argv[1]);
    int wallpaperHeight = atoi(argv[2]);

    g_type_init();
    RsvgHandle* archLogoSVG = rsvg_handle_new_from_file("archlinux-logo-light-scalable.svg", NULL);
    printf("Reading logo done.\n");

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, wallpaperWidth, wallpaperHeight);
    cairo_t* cr = cairo_create(surface);

    // fill background with grey color
    cairo_set_source_rgb(cr, 38.0/255.0, 39.0/255.0, 33.0/255.0);
    cairo_rectangle(cr, 0, 0, wallpaperWidth, wallpaperHeight);
    cairo_fill(cr);
    printf("Background filled.\n");

    const int NUM_CIRCLES = 200;
    for (int circle = 0; circle < NUM_CIRCLES; ++circle)
        drawRandomCircles(cr);
    printf("Circles drawn.\n");

    const int NUM_WAVES = 500;
    for (int wave = 0 ; wave < NUM_WAVES; ++wave)
        drawRandomSineWaves(cr);
    printf("Waves drawn.\n");

    drawArchLogo(cr, archLogoSVG);
    printf("Logo drawn.\n");

    cairo_surface_write_to_png(surface, "wallpaper.png");
    printf("Wallpaper written to file.\n");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    printf("Exiting.\n");
    return EXIT_SUCCESS;
}
