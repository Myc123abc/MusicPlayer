#include "MusicPlayer.h"
#include <cstdlib>
#include <stdio.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: ./a.out MusicDirectoryPath\n");
    exit(EXIT_FAILURE);
  }
  MusicPlayer player(argv[1]);
  player.run();
  return 0;
}
