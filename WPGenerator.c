#include <assert.h>
#include <cairo.h>
#include <fcntl.h>
#include <getopt.h>
#include <librsvg/rsvg.h>
#ifndef RSVG_CAIRO_H
#include <librsvg/rsvg-cairo.h>
#endif
#include <limits.h>
#include <math.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROGRAM_NAME        "WPGenerator"
#ifndef M_PI
#define M_PI                3.141592653589793
#endif
#define PHI                 1.6180339887
#define LOGO_FILENAME       "archlinux-logo-light-scalable.svg"
#define FILENAME_MAXLENGTH	1000
#define NUM_CIRCLES_DEFAULT 0
#define NUM_WAVES_DEFAULT   0
#define MAX_RANDOM_CIRCLES  5000
#define MAX_RANDOM_WAVES    5000
#define STANDARD_OUTFILENAME "wallpaper.png"

// http://stackoverflow.com/questions/195975/how-to-make-a-char-string-from-a-c-macros-value
#define xstr(s) blub(s)
#define blub(s) #s

enum LogoAlignment {LEFT_ALIGNED, CENTER_ALIGNED, RIGHT_ALIGNED};

struct ProgramArguments {
    int screenWidth;
    int screenHeight;
    int numCircles;
    int numWaves;
    char * outFile;
    bool quadsBackground;
    bool dotPattern;
    bool noLogo;
    bool help;
    bool stripedBackground;
    enum LogoAlignment alignment;
};

struct ColorRGBA {
    double red;
    double green;
    double blue;
    double alpha;
};

static struct ColorRGBA colorDarkBlue = { 60.0/255.0, 76.0/255.0,  85.0/255.0,  1.0 };
static struct ColorRGBA colorArchBlue = { 23.0/255.0, 147.0/255.0, 209.0/255.0, 1.0 };
static struct ColorRGBA colorBgGrey   = { 38.0/255.0, 39.0/255.0,  33.0/255.0,  0.9 };

unsigned int urandomRng(void) {
    int urandomFD = open("/dev/urandom", O_RDONLY);
    unsigned int randomInt;
    ssize_t ret = read(urandomFD, &randomInt, sizeof(randomInt));
    assert(ret == sizeof(randomInt));
    close(urandomFD);
    return randomInt;
}

char* strdup(const char *str) {
    int n = strlen(str) + 1;
    char *dup = malloc(n);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}

// xorshift, see http://en.wikipedia.org/wiki/Xorshift
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
    assert(cr);

    cairo_surface_t* surface = cairo_get_group_target(cr);
    return cairo_image_surface_get_width(surface);
}

int getSurfaceHeight(cairo_t* cr) {
    assert(cr);

    cairo_surface_t* surface = cairo_get_group_target(cr);
    return cairo_image_surface_get_height(surface);
}

void drawRandomCircles(cairo_t* cr) {
    assert(cr);

    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth = getSurfaceWidth(cr);

    cairo_set_line_width(cr, 2.0 * rng() + 3.0);
    cairo_set_source_rgba(cr, colorDarkBlue.red, colorDarkBlue.green, colorDarkBlue.blue, 0.3 * rng());
    cairo_arc(cr, rng() * surfaceWidth, rng() * surfaceHeight, rng() * 20, 0, 2.0 * M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 60.0/255.0, 76.0/255.0, 85.0/255.0, 0.3 * rng());
    cairo_stroke(cr);
}

void drawRandomSineWaves(cairo_t* cr) {
    assert(cr);

    cairo_new_path(cr);
    cairo_set_line_width(cr, 2.0);

    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth = getSurfaceWidth(cr);

    double minPeriod = 5;
    double period1 = (minPeriod + rng()) * surfaceWidth;
    double period2 = 2 * period1;

    double startX = 0.0;
    double startY = surfaceHeight * 1.1111 * (rng() - 0.1);

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
    cairo_set_source_rgba(cr, colorArchBlue.red, colorArchBlue.green, colorArchBlue.blue, 0.1 * rng());
    cairo_stroke(cr);
}

void drawArchLogo(cairo_t* cr, RsvgHandle* archLogoSVG, enum LogoAlignment alignment) {
    assert(cr);
    assert(archLogoSVG);

    RsvgDimensionData* logoDimensions = (RsvgDimensionData*) malloc(sizeof(RsvgDimensionData));
    rsvg_handle_get_dimensions(archLogoSVG, logoDimensions);

    double rightBorderWidth = 10.0;
    double surfaceHeight = getSurfaceHeight(cr);
    double surfaceWidth  = getSurfaceWidth(cr);
    double logoWidth = logoDimensions->width;
    double logoHeight = logoDimensions->height;
    double scale = (0.17 * surfaceHeight) / logoHeight;
    logoWidth *= scale;
    logoHeight *= scale;

    double rectY = 1.0 / PHI * surfaceHeight - logoHeight / 2.0;
    double logoOriginX;
    double logoOriginY = 1.0 / PHI * surfaceHeight - logoHeight / 2.0;

    switch (alignment) {
        case LEFT_ALIGNED:
            logoOriginX = 0.0;
            break;
        case CENTER_ALIGNED:
            logoOriginX = (surfaceWidth - logoWidth) / 2.0;
            break;
        case RIGHT_ALIGNED:
            logoOriginX = surfaceWidth - logoWidth - rightBorderWidth;
            break;
        default: // should never come hear
            exit(EXIT_FAILURE);
    }

    // grey
    cairo_set_source_rgba(cr, colorBgGrey.red, colorBgGrey.green, colorBgGrey.blue, colorBgGrey.alpha);
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
    cairo_save(logoContext);
    cairo_scale(logoContext, scale, scale);

    rsvg_handle_render_cairo(archLogoSVG, logoContext);

    cairo_restore(logoContext);
    cairo_destroy(logoContext);
    cairo_surface_destroy(logoSurface);
    free(logoDimensions);
}

void usage(int status) {
    printf("WPGenerator is a tool which can create simple, nice looking wallpapers through argument variation.\n\n");

    printf("Usage:\n"
           "./WPGenerator --width WIDTH --height HEIGHT [--circles NUM_CIRCLES --waves NUM_WAVES --random --dots --quads --outFile FILENAME --nologo --logopos ALIGNMENT]\n\n");

    printf(
           "Optionname\tOptional Argument\tExplanation\n"
           "--circles\tNUM_CIRCLES\t\tControls the number of circles drawn\n"
           "--dots\t\t\t\t\ttoggles wheter a dot raster is drawn\n"
           "--height\tHEIGHT\t\t\tscreen resolution height [px]\n"
           "--nologo\t\t\t\tommit Arch Linux logo\n"
           "--help\t\t\t\t\tshow this help message\n"
           "--logopos\tleft,center,right\tsets the alignment of the Arch Linux Logo\n"
    	   "--outFile\tFILENAME\t\tpath of the generated wallpaper\n"
           "--quads\t\t\t\t\tdraw quads in the background\n"
           "--random\t\t\t\tdraw a random number of circles and waves\n"
           "\t\t\t\t\t(max. %i, max. %i)\n"
           "--stripes\t\t\t\tdraw a striped background\n"
           "--waves\t\tNUM_WAVES\t\tcontrol the number of waves drawn\n"
           "--width\t\tWIDTH\t\t\tscreen resolution width [px]\n",
          MAX_RANDOM_CIRCLES, MAX_RANDOM_WAVES);
    exit(status);
}


void checkNumericalArgument(char* arg) {
    assert(arg);

    regex_t regex;
    regcomp(&regex, "^[[:digit:]]+$", REG_EXTENDED);

    if(regexec(&regex, arg, 0, NULL, 0) == REG_NOMATCH) {
        printf("No match %s", arg);
        usage(EXIT_FAILURE);
    }

    regfree(&regex);
}

void getArguments(int argc, char ** argv, struct ProgramArguments* progArgs) {
    assert(argv);
    assert(progArgs);

    bool widthOptionSet = false;
    bool heightOptionSet = false;
    bool circlesOptionSet = false;
    bool wavesOptionSet = false;
    bool randomOptionSet = false;
    bool quadsOptionSet = false;
    bool dotsOptionSet = false;
    bool showLogoOptionSet = false;
    bool helpOptionSet = false;
    bool alignmentOptionSet = false;
    bool stripedBackgroundOptionSet = false;
    bool outFileOptionSet = false;

    static struct option long_options[] = {
        { "logopos", required_argument, 0, 'a' },
        { "help",    no_argument,       0, 'x' },
        { "nologo",  no_argument,       0, 'l' },
        { "dots",    no_argument,       0, 'd' },
        { "quads",   no_argument,       0, 'q' },
        { "random",  no_argument,       0, 'r' },
        { "width",   required_argument, 0, 'w' },
        { "height",  required_argument, 0, 'h' },
        { "circles", required_argument, 0, 'c' },
        { "waves",   required_argument, 0, 's' },
        { "stripes", no_argument,       0, 't' },
        { "outFile", required_argument, 0, 'o' },
        { 0, 0, 0, 0 }
    };

    int shortOptionValue;
    while (true) {
        shortOptionValue = getopt_long(argc, argv, "axldqrwhcs", long_options, NULL);

        /* Detect the end of the options. */
        if (shortOptionValue == -1)
        break;

        switch (shortOptionValue) {
        case 'a':
            alignmentOptionSet = true;
            if (strncmp(optarg, "left", strlen("left")) == 0)
                progArgs->alignment = LEFT_ALIGNED;
            else if (strncmp(optarg, "center", strlen("center")) == 0)
                progArgs->alignment = CENTER_ALIGNED;
            else if (strncmp(optarg, "right", strlen("right")) == 0)
                progArgs->alignment = RIGHT_ALIGNED;
            else
                usage(EXIT_FAILURE);
            break;
        case 'x':
            helpOptionSet = true;
            progArgs->help = true;
            break;
        case 'l':
            showLogoOptionSet = true;
            progArgs->noLogo = true;
            break;
        case 'd':
            dotsOptionSet = true;
            progArgs->dotPattern = true;
            break;
        case 'q':
            quadsOptionSet = true;
            progArgs->quadsBackground = true;
            break;
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
        case 'o':
        	outFileOptionSet = true;
        	progArgs->outFile = strdup(optarg);
        	break;
        case 's':
            checkNumericalArgument(optarg);
            wavesOptionSet = true;
            progArgs->numWaves = atoi(optarg);
            break;
        case 't':
            stripedBackgroundOptionSet = true;
            progArgs->stripedBackground = true;
            break;
        case '?':
            usage(EXIT_FAILURE);
            break;
        default:
            abort();
        }
    }

    // set standard values
    if (!alignmentOptionSet)
        progArgs->alignment = CENTER_ALIGNED;

    if (!helpOptionSet && !(widthOptionSet && heightOptionSet)) {
        usage(EXIT_FAILURE);
    }

    if (!circlesOptionSet)
        progArgs->numCircles = NUM_CIRCLES_DEFAULT;
    if (!wavesOptionSet)
        progArgs->numWaves = NUM_WAVES_DEFAULT;

    if (randomOptionSet) {
        const size_t BUFSIZE = 50;
        char* tmpString = (char*) malloc(BUFSIZE * sizeof(char));

        printf("Random Mode\n");

        progArgs->numCircles = urandomRng() % MAX_RANDOM_CIRCLES;
        sprintf(tmpString, "%i", progArgs->numCircles);
        printf("Number circles: %s\n", tmpString);

        progArgs->numWaves = urandomRng() % MAX_RANDOM_WAVES;
        sprintf(tmpString, "%i", progArgs->numWaves);
        printf("Number waves: %s\n", tmpString);

        free(tmpString);
    }

    if(!helpOptionSet)
        progArgs->help = false;

    if (!dotsOptionSet)
        progArgs->dotPattern = false;

    if (!quadsOptionSet)
        progArgs->quadsBackground = false;

    if (!stripedBackgroundOptionSet)
        progArgs->stripedBackground = false;

    if (!showLogoOptionSet)
        progArgs->noLogo = false;

    if (!outFileOptionSet)
    	progArgs->outFile = strdup(STANDARD_OUTFILENAME);
}

void drawSimpleFilledBackground(cairo_t* cr) {
    assert(cr);

    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth  = getSurfaceWidth(cr);

    cairo_set_source_rgba(cr, colorBgGrey.red, colorBgGrey.green, colorBgGrey.blue, 1.0);
    cairo_rectangle(cr, 0, 0, surfaceWidth, surfaceHeight);
    cairo_fill(cr);
}

void drawStripedBackground(cairo_t* cr) {
    assert(cr);

    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth  = getSurfaceWidth(cr);

    int width = 0;
    int stripeWidth = 0;

    while (width < surfaceWidth) {
        stripeWidth = 3;
        double colorFactor = rng();
        cairo_set_source_rgba(cr,
                              colorFactor * colorArchBlue.red,
                              colorFactor * colorArchBlue.green,
                              colorFactor * colorArchBlue.blue,
                              0.2);
        cairo_rectangle(cr, width, 0, width + stripeWidth, surfaceHeight);
        cairo_fill(cr);
        width += stripeWidth;
    }
}

void drawQuadsBackground(cairo_t* cr) {
    assert(cr);

    int surfaceHeight = getSurfaceHeight(cr);
    int surfaceWidth  = getSurfaceWidth(cr);

    cairo_set_line_width(cr, 0.5);
    int rectDim = surfaceWidth/200;
    int x = 0.0;
    int y = 0.0;
    while (y < surfaceHeight) {
        while (x < surfaceWidth) {
            cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.6 * rng());
            cairo_rectangle(cr, x, y, rectDim, rectDim);
            cairo_fill(cr);
            x += rectDim + 1;
        }
        x = 0.0;
        y += rectDim + 1;
    }
}

void drawDotPattern(cairo_t* cr) {
    assert(cr);

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

    if (progArgs->help)
        usage(EXIT_SUCCESS);

    RsvgHandle* archLogoSVG = NULL;

    // assemble logo path
    const char* datarootdir = xstr(DATAROOTDIR);
    size_t datarootdirLen = strlen(datarootdir);
    size_t filenameLen = strlen(LOGO_FILENAME);
    char* logoPath = (char*) malloc(sizeof(char) * (datarootdirLen + filenameLen + 2));
    strcat(logoPath, datarootdir);
    strcat(logoPath, "/");
    strcat(logoPath, LOGO_FILENAME);
    archLogoSVG = rsvg_handle_new_from_file(logoPath, NULL);
    printf("Reading logo done.\n");

    cairo_surface_t* surface = NULL;
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         progArgs->screenWidth,
                                         progArgs->screenHeight);
    cairo_t* cr = cairo_create(surface);

    drawSimpleFilledBackground(cr);
    if (progArgs->quadsBackground)
        drawQuadsBackground(cr);

    if (progArgs->stripedBackground)
        drawStripedBackground(cr);
    printf("Background filled.\n");

    for (int circle = 0; circle < progArgs->numCircles; ++circle)
        drawRandomCircles(cr);
    printf("Circles drawn.\n");

    for (int wave = 0 ; wave < progArgs->numWaves; ++wave)
        drawRandomSineWaves(cr);
    printf("Waves drawn.\n");

    if (progArgs->dotPattern) {
        drawDotPattern(cr);
        printf("Dot Pattern drawn.\n");
    }

    if (!progArgs->noLogo) {
        drawArchLogo(cr, archLogoSVG, progArgs->alignment);
        printf("Logo drawn.\n");
    }

    rsvg_handle_close(archLogoSVG, NULL);
    g_object_unref(archLogoSVG);

    cairo_surface_write_to_png(surface, progArgs->outFile);
    printf("Wallpaper written to file.\n");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(progArgs->outFile);
    free(progArgs);
    free(logoPath);
    printf("Exiting.\n");
    return EXIT_SUCCESS;
}
