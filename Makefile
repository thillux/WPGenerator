CC:=gcc
CFLAGS:=-O3 -march=native -std=c99 -g -Wall -Wextra -Werror
LIBS:=`pkg-config --libs cairo librsvg-2.0`
INCLUDES:=`pkg-config --cflags cairo librsvg-2.0`

PROJECTNAME:=WPGenerator

all: WPGenerator.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(PROJECTNAME) $< $(LIBS)

clean:
	rm -f *.o
	rm -f $(PROJECTNAME)
