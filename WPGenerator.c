#include <cairo.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define M_PI                3.141592653589793
#define PHI                 1.6180339887
#define LOGO_FILENAME       "archlinux-logo-light-scalable.svg"
#define NUM_CIRCLES_DEFAULT 0
#define NUM_WAVES_DEFAULT   5000
#define MAX_RANDOM_CIRCLES  5000
#define MAX_RANDOM_WAVES    5000

struct ProgramArguments {
    int screenWidth;
    int screenHeight;
    int numCircles;
    int numWaves;
};

unsigned int urandomRng(void) {
	int urandomFD = open("/dev/urandom", O_RDONLY);
	unsigned int randomInt;
	read(urandomFD, &randomInt, sizeof(randomInt));
	close(urandomFD);
	return randomInt;
}

// xorshift
double rng(void) {
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
    cairo_set_line_width(cr, 2.0 * rng() + 3.0);
    cairo_set_source_rgba(cr, 60.0/255.0, 76.0/255.0, 85.0/255.0, 0.3 * rng());
    cairo_arc(cr, rng() * surfaceWidth, rng() * surfaceHeight, rng() * 20, 0, 2.0 * M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 60.0/255.0, 76.0/255.0, 85.0/255.0, 0.3 * rng());
    cairo_stroke(cr);
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
    cairo_set_source_rgba(cr, 38.0/255.0, 39.0/255.0, 33.0/255.0, 0.9);
    cairo_rectangle(cr, 0, rectY, surfaceWidth, logoHeight);
    cairo_fill(cr);

    cairo_set_line_width(cr, 2.0);

    cairo_set_source_rgba(cr, 23.0/255.0, 147.0/255.0, 209.0/255.0, 0.8);
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
    printf("Usage: ./WPGenerator --width WIDTH --height HEIGHT [--circles NUM_CIRCLES --waves NUM_WAVES --random]\n");
}

void checkNumericalArgument(char* arg) {
    regex_t regex;
    regcomp(&regex, "^[[:digit:]]+$", REG_EXTENDED);

    if(regexec(&regex, arg, 0, NULL, 0) == REG_NOMATCH) {
    	printf("No match %s", arg);
    	usage();
        exit(EXIT_FAILURE);
    }

    regfree(&regex);
}

void getArguments(int argc, char ** argv, struct ProgramArguments* progArgs) {
	bool widthOptionSet = false;
	bool heightOptionSet = false;
	bool circlesOptionSet = false;
	bool wavesOptionSet = false;
	bool randomOptionSet = false;

	static struct option long_options[] = {
		{ "random",  no_argument, 		0, 'r' },
		{ "width", 	 required_argument, 0, 'w' },
		{ "height",  required_argument, 0, 'h' },
		{ "circles", required_argument, 0, 'c' },
		{ "waves", 	 required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	int shortOptionValue;
	while (true) {
		shortOptionValue = getopt_long(argc, argv, "w:h:cs", long_options, NULL);

		/* Detect the end of the options. */
		if (shortOptionValue == -1)
			break;

		switch (shortOptionValue) {
		case 'r':
			randomOptionSet = true;
			break;
		case 'w':
			checkNumericalArgument(optarg);
			widthOptionSet = true;
			progArgs->screenWidth = atoi(optarg);
			break;
		case 'h':
			checkNumericalArgument(optarg);
			heightOptionSet = true;
			progArgs->screenHeight = atoi(optarg);
			break;
		case 'c':
			checkNumericalArgument(optarg);
			circlesOptionSet = true;
			progArgs->numCircles = atoi(optarg);
			break;
		case 's':
			checkNumericalArgument(optarg);
			wavesOptionSet = true;
			progArgs->numWaves = atoi(optarg);
			break;
		case '?':
			break;
		default:
			abort();
		}
	}

	if (! (widthOptionSet && heightOptionSet)) {
		usage();
		exit(EXIT_FAILURE);
	}

	if (!circlesOptionSet)
		progArgs->numCircles = NUM_CIRCLES_DEFAULT;
	if (!wavesOptionSet)
		progArgs->numWaves = NUM_WAVES_DEFAULT;

	if (randomOptionSet) {
		char * tmpString = (char*) malloc(50 * sizeof(char));

		printf("Random Mode\n");

		progArgs->numCircles = urandomRng() % MAX_RANDOM_CIRCLES;
		sprintf(tmpString, "%i", progArgs->numCircles);
		printf("Number circles: %s\n", tmpString);

		progArgs->numWaves = urandomRng() % MAX_RANDOM_WAVES;
		sprintf(tmpString, "%i", progArgs->numWaves);
		printf("Number waves: %s\n", tmpString);

		free(tmpString);
	}
}

void drawBackground(cairo_t* cr) {
    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth  = getSurfaceWidth(cr);

    cairo_set_source_rgba(cr, 38.0/255.0, 39.0/255.0, 33.0/255.0, 1.0);
    cairo_rectangle(cr, 0, 0, surfaceWidth, surfaceHeight);
    cairo_fill(cr);

    cairo_pattern_t *pat;
    pat = cairo_pattern_create_linear(0.0, 0.0,  surfaceWidth, surfaceHeight);
    cairo_pattern_add_color_stop_rgba(pat, 0, 23.0/255.0, 147.0/255.0, 209.0/255.0, 0.1);
    cairo_pattern_add_color_stop_rgba(pat, 1, 100.0/255.0, 100.0/255.0, 100.0/255.0, 0.1);
    cairo_rectangle(cr, 0, 0, surfaceWidth, surfaceHeight);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
}

void drawDotPattern(cairo_t* cr) {
    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth  = getSurfaceWidth(cr);

    cairo_set_source_rgba(cr, 1,1,1,0.2);
    cairo_set_line_width(cr, 2.0);
    // draw points
    double dx = surfaceWidth / 30.0;
    double x = dx;
    double y = dx;
    while (y < surfaceHeight) {
        while (x < surfaceWidth) {
            cairo_arc(cr, x, y, 2, 0, 2.0 * M_PI);
            cairo_fill (cr);
            x += dx;
        }
        x = dx;
        y += dx;
    }
}

int main(int argc, char ** argv) {
    struct ProgramArguments * progArgs = (struct ProgramArguments *) malloc(sizeof(struct ProgramArguments));
    getArguments(argc, argv, progArgs);

    g_type_init();
    RsvgHandle* archLogoSVG;
    archLogoSVG = rsvg_handle_new_from_file(LOGO_FILENAME, NULL);
    printf("Reading logo done.\n");

    cairo_surface_t* surface;
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         progArgs->screenWidth,
                                         progArgs->screenHeight);
    cairo_t* cr = cairo_create(surface);

    drawBackground(cr);
    printf("Background filled.\n");

    for (int circle = 0; circle < progArgs->numCircles; ++circle)
        drawRandomCircles(cr);
    printf("Circles drawn.\n");

    for (int wave = 0 ; wave < progArgs->numWaves; ++wave)
        drawRandomSineWaves(cr);
    printf("Waves drawn.\n");

    drawDotPattern(cr);
    printf("Dot Pattern drawn.\n");

    drawArchLogo(cr, archLogoSVG);
    printf("Logo drawn.\n");

    cairo_surface_write_to_png(surface, "wallpaper.png");
    printf("Wallpaper written to file.\n");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(progArgs);
    printf("Exiting.\n");
    return EXIT_SUCCESS;
}
