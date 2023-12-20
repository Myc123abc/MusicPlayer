#include "MusicPlayer.h"
#include <curses.h>
#include <functional>
#include <ncurses.h>
#include <string.h>
#include <thread>

using namespace std::chrono_literals;

MusicPlayer::MusicInfo::MusicInfo()
    : _pause(true), _cycle(false), _mute(false), _begin(true), _end(false),
      _reset(false), _dir(false), _dirwin(nullptr), _cursor(0), _find(false),
      _resultpos(-1) {}
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

// the find function can get result which have the wchar_t position
std::pair<size_t, size_t> myfind(const std::string &music,
                                 const std::string &musicname, size_t pos);

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

      wattron(_dirwin, COLOR_PAIR(2) | A_REVERSE);
      for (const auto &result : find_result) {
        if (result.first == index) {
          for (const auto &pos : result.second) {
            mvwprintw(_dirwin, num, pos + 1, "%s", _musicname.c_str());
          }
        }
      }
      wattroff(_dirwin, COLOR_PAIR(2) | A_REVERSE);
      if (!_musicname.empty())
        if (_resultpos != -1 && _resultpos == index) {
          wattron(_dirwin, COLOR_PAIR(1) | A_REVERSE);
          mvwprintw(_dirwin, num, 1, "%s", entry.c_str());
          wattroff(_dirwin, COLOR_PAIR(1) | A_REVERSE);
        }
    }
  };

  auto printCursor = [highth, width, this](unsigned index) {
    attron(COLOR_PAIR(1));
    mvprintw(_row / 10 + 1 + index, _col / 4 - 1, ">");
    attroff(COLOR_PAIR(1));
    refresh();
  };

  static int *p1, *p2, *p3;
  static bool first2 = true;
  if (!first2)
    checkout(p1, p2, p3, highth, -1);
  _p->_callback = [this, highth](int forfind) {
    checkout(p1, p2, p3, highth, forfind);
  };

  static int cursorStation = 0;
  static int oldcursor = _cursor;
  static int oldStation = _cursor;
  if (first2) {
    p1 = &cursorStation;
    p2 = &oldcursor;
    p3 = &oldStation;
    first2 = false;
    if (size - _cursor < highth) {
      cursorStation = highth - 1;
      oldStation -= cursorStation;
    }
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

    // the function find music
    if (_find) {
      find_result.clear();
      if (!_musicname.empty()) {
        size_t index = 0;
        for (const auto &music : _p->_musiclist) {
          std::vector<int> result;
          size_t pos = 0;
          while (true) {
            auto res = myfind(music, _musicname.c_str(), pos);
            if ((pos = res.first) == std::string::npos)
              break;
            result.push_back(res.second);
            pos += _musicname.size();

            // if ((pos = music.find(_musicname.c_str(), pos)) ==
            //     std::string::npos)
            //   break;
            // result.push_back(pos);
            // pos += _musicname.size();
          }
          if (!result.empty()) {
            find_result.push_back({index, result});
          }
          ++index;
        }
      }
    }

    printCursor(cursorStation);
    printDir(oldStation, cursorStation + 1);
    oldcursor = _cursor;

    wattron(_dirwin, COLOR_PAIR(1));
    box(_dirwin, 0, 0);
    wattroff(_dirwin, COLOR_PAIR(1));

    if (_find) {
      int y = _dirwin->_begy + _dirwin->_maxy + 1, x = _dirwin->_begx;
      attron(COLOR_PAIR(1));
      mvprintw(y - 1, x, "├");
      mvprintw(y - 1, x + _dirwin->_maxx, "┤");
      mvprintw(y, x, "│");
      mvprintw(y, x + _dirwin->_maxx, "│");
      mvprintw(y + 1, x, "└");
      mvprintw(y + 1, x + _dirwin->_maxx, "┘");
      for (int i = 1; i < _dirwin->_maxx; ++i) {
        mvprintw(y + 1, x + i, "─");
        mvprintw(y - 1, x + i, "─");
      }
      attroff(COLOR_PAIR(1));

      mvprintw(y, x + 1, "%s", _musicname.c_str());
      move(y, x + 1 + _musicname.length());
      // mvprintw(y + 1, x + 1, "%ld", find_result.size());
      //
      // for (size_t i = 0; i < find_result.size(); ++i) {
      //   mvprintw(y + 1 + i, 0, "%3d", find_result[i].first);
      //   for (size_t j = 0, z = 0; j < find_result[i].second.size();
      //        ++j, z += 2) {
      //     mvprintw(y + 1 + i, 3 + z, " %d", find_result[i].second[j]);
      //   }
      // }
    }

    wrefresh(_dirwin);
    refresh();

    lock1.release();
  }

  find_result.clear();
  _musicname.clear();
  _resultpos = -1;

  wborder(_dirwin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
  wrefresh(_dirwin);
  delwin(_dirwin);
}

void MusicPlayer::MusicInfo::checkout(int *p1, int *p2, int *p3, int highth,
                                      int forfind) {
  int &cursorStation = *p1, &oldcursor = *p2, &oldStation = *p3;
  int index = _p->_index;
  if (forfind != -1)
    index = forfind;

  int tmp = index - oldcursor;
  if (tmp < 0) {
    tmp *= -1;
    if (tmp > cursorStation) {
      cursorStation = 0;
      oldcursor = index;
      oldStation = index;
      return;
    }
  } else {
    if (tmp > highth - 1 - cursorStation) {
      oldStation += tmp - highth + 1 + cursorStation;
      cursorStation = highth - 1;
      oldcursor = index;
      return;
    }
  }

  if (cursorStation == 0) {
    if (index < oldcursor)
      oldStation = index;
    else
      cursorStation += index - oldcursor;
  } else if (cursorStation < highth - 1) {
    cursorStation += index - oldcursor;
  } else {
    if (index < oldcursor)
      cursorStation += index - oldcursor;
    else
      oldStation += index - oldcursor;
  }
  oldcursor = index;
}

#include <codecvt>
#include <locale>
#include <wchar.h>

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

std::pair<size_t, size_t> myfind(const std::string &music,
                                 const std::string &musicname, size_t pos) {
  pos = music.find(musicname.c_str(), pos);

  if (pos == std::string::npos)
    return {pos, pos};

  auto tmp = music.substr(0, pos);
  std::wstring ws = converter.from_bytes(tmp);
  auto width = wcswidth(ws.c_str(), -1);

  return {pos, width};
}
