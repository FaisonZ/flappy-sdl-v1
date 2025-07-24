gcc src/flappy.c -o build/flappy/bin/flappy `pkg-config --cflags --libs sdl3` -Wl,-rpath='$ORIGIN/../lib'
