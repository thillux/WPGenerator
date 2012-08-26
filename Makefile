CC:=gcc
CFLAGS:=-O0 -march=native -std=c99 -g -Wall -Wextra -Werror
LIBS:=`pkg-config --libs cairo librsvg-2.0`
INCLUDES:=`pkg-config --cflags cairo librsvg-2.0`

PROJECTNAME:=WPGenerator

all: WPGenerator.c
	$(CC) $(CFLAGS) $(LIBS) $(INCLUDES) -o $(PROJECTNAME) $<

clean:
	rm -f *.o
	rm -f $(PROJECTNAME)
