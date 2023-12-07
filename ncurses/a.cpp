#include <chrono>
#include <curses.h>
#include <ncurses.h>
#include <thread>

using namespace std::chrono_literals;

int mode;
unsigned x, y;

int main() {
  initscr();
  curs_set(0);

  std::thread([] {
    while (true) {
      clear();
      mvprintw(y, x, "*");
      refresh();
      std::this_thread::sleep_for(33ms);

      if (mode == 0)
        ++x;
      else if (mode == 1)
        --x;
      else if (mode == 2)
        ++y;
      else if (mode == 3)
        --y;
    }
  }).detach();

  while (true) {
    switch (getch()) {
    case 'h':
      mode = 1;
      break;
    case 'j':
      mode = 2;
      break;
    case 'k':
      mode = 3;
      break;
    case 'l':
      mode = 0;
      break;
    }
  }

  endwin();

  return 0;
}
