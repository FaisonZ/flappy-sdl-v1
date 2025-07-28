gcc src/flappy.c -o build/flappy/flappy `pkg-config --cflags --libs sdl3 --libs sdl3-image` -Wl,-rpath='$ORIGIN/lib' -g -Wall
