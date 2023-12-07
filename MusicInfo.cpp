#include "MusicPlayer.h"
#include <curses.h>
#include <ncurses.h>
#include <string.h>
#include <thread>

using namespace std::chrono_literals;

MusicPlayer::MusicInfo::MusicInfo()
    : _pause(true), _cycle(false), _mute(false), _begin(true), _end(false),
      _reset(false), _dir(false), _dirwin(nullptr), _cursor(0) {}
MusicPlayer::MusicInfo::~MusicInfo() {}

void MusicPlayer::MusicInfo::load(MusicPlayer *p) {
  _p = p;
  std::string name(_p->_music.getName());
  strcpy(_name, name.substr(name.find_last_of('/') + 1).c_str());
  _minute = _p->_music.getTime() / 60;
  _second = (unsigned)_p->_music.getTime() % 60;
  _currminute = 0;
  _currsecond = 0;
}

void MusicPlayer::MusicInfo::update() {
  unsigned time = _p->_music.getCurrentTime();
  _currminute = time / 60;
  _currsecond = time % 60;
}

static unsigned len;

void MusicPlayer::MusicInfo::runUI() {
  std::thread([this] {
    static unsigned cnt = 0;
    while (true) {
      getmaxyx(stdscr, _row, _col);

      if (_dir)
        directory();

      print();
      if (_reset) {
        if (_begin)
          mvprintw(_row / 2 + 1, len, "This is the first music.\n");
        if (_end)
          mvprintw(_row / 2 + 1, len, "This is the last music\n");
        if (++cnt == 10) {
          cnt = 0;
          _reset = false;
        }
      } else
        printw("\n");

      refresh();

      std::this_thread::sleep_for(100ms);

      if (_currminute == _minute && _currsecond == _second) {
        if (!_pause && !_cycle) {
          if (!_end) {
            _p->next();
            continue;
          } else {
            _p->_music.end();
            _p->load(_p->_music.getName());
            _pause = true;
            continue;
          }
        }
      }
    }
  }).detach();
}

void MusicPlayer::MusicInfo::print() {
  update();
  clear();
  char statusbar[100];
  sprintf(statusbar, "[ %s  %s  |  %s %3u%% ] [%02u:%02u/%02u:%02u]\n",
          _pause ? "" : "", _cycle ? "" : "",
          _mute ? "󰝟" : "", _p->_music.getVolume(), _currminute,
          _currsecond, _minute, _second);
  len = (_col - strlen(statusbar)) / 2;

  attron(COLOR_PAIR(1));
  mvprintw(_row / 2 - 1, len, "%s", _name);
  mvprintw(_row / 2, len, "%s", statusbar);
  attroff(COLOR_PAIR(1));
}

extern std::binary_semaphore lock1;
extern std::binary_semaphore lock2;

void MusicPlayer::MusicInfo::directory() {
  _dirwin = newwin(_row / 10 * 9, _col / 2, _row / 10, _col / 4);
  start_color();

  unsigned highth = _row / 10 * 9 - 2;
  unsigned width = _col / 2 - 2;
  unsigned size = _p->_musiclist.size();

  auto printDir = [highth, width, this](unsigned index, unsigned index2) {
    for (unsigned num = 1; num <= highth; ++num, ++index) {
      auto entry = _p->_musiclist[index];
      if (num == index2) {
        wattron(_dirwin, COLOR_PAIR(1) | A_UNDERLINE);
        mvwprintw(_dirwin, num, 1, "%-100s", entry.c_str());
        wattroff(_dirwin, COLOR_PAIR(1) | A_UNDERLINE);
      } else
        mvwprintw(_dirwin, num, 1, "%-100s", entry.c_str());
    }
  };

  auto printCursor = [highth, width, this](unsigned index) {
    attron(COLOR_PAIR(1));
    mvprintw(_row / 10 + 1 + index, _col / 4 - 1, ">");
    attroff(COLOR_PAIR(1));
    refresh();
  };

  static int cursorStation = 0;
  static int oldcursor = _cursor;
  static int oldStation = _cursor;
  if (size - _cursor < highth) {
    cursorStation = highth - size + _cursor;
    oldStation = size - highth;
  }

  bool first = true;

  while (_dir) {
    if (first) {
      first = false;
      goto jump;
    }
    lock2.acquire();
  jump:
    clear();
    wclear(_dirwin);

    if (cursorStation == 0) {
      if (_cursor < oldcursor) {
        oldStation = _cursor;
      } else {
        cursorStation += _cursor - oldcursor;
      }
    } else if (cursorStation > 0 && cursorStation < highth - 1) {
      cursorStation += _cursor - oldcursor;
    } else if (cursorStation == highth - 1) {
      if (_cursor < oldcursor) {
        cursorStation += _cursor - oldcursor;
      } else {
        oldStation += _cursor - oldcursor;
      }
    }

    printCursor(cursorStation);
    printDir(oldStation, cursorStation + 1);
    oldcursor = _cursor;

    wattron(_dirwin, COLOR_PAIR(1));
    box(_dirwin, 0, 0);
    wattroff(_dirwin, COLOR_PAIR(1));

    wrefresh(_dirwin);
    refresh();

    lock1.release();
  }

  wborder(_dirwin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
  wrefresh(_dirwin);
  delwin(_dirwin);
}
