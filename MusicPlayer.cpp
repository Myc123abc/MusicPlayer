#include "MusicPlayer.h"
#include "error.h"
#include <cstdlib>
#include <dirent.h>
#include <ncurses.h>
#include <string.h>

// for random
#include <algorithm>
#include <random>

MusicPlayer::MusicPlayer(const char *musicDir) {
  strcpy(_path, musicDir);
  if (_path[strlen(_path) - 1] != '/')
    strcat(_path, "/");
  readMusicDirectory(_path);
}

MusicPlayer::~MusicPlayer() {}

void MusicPlayer::load(const char *name) {
  _music.load(name);
  _music.play();
  _music.pause();
  _musicInfo.load(this);
}

void MusicPlayer::run() {
  load((std::string(_path) + _musiclist[_index]).c_str());

  // start ncurses
  setlocale(LC_ALL, "");
  initscr();
  noecho();
  use_default_colors();
  start_color();
  curs_set(0);
  keypad(stdscr, TRUE);

  init_pair(1, COLOR_GREEN, -1);
  init_pair(2, COLOR_YELLOW, -1);

  _musicInfo.runUI();
  while (true) {
    switch (getch()) {
    case ' ':
      _music.pause();
      _musicInfo._pause = !_musicInfo._pause;
      break;
    case 'q':
      endwin();
      return;
    case 'k':
      _music.raiseVolume(5);
      break;
    case 'j':
      _music.lowerVolume(5);
      break;
    case 'm':
      _music.muteVolume();
      _musicInfo._mute = !_musicInfo._mute;
      break;
    case 'l':
      _music.raisePosition(5);
      break;
    case 'h':
      _music.lowerPosition(5);
      break;
    case 'c':
      _music.loop();
      _musicInfo._cycle = !_musicInfo._cycle;
      break;
    case 'n':
      next();
      break;
    case 'p':
      prev();
      break;
    case 'd':
      runDirectory();
      break;
    }
  }
}

void MusicPlayer::play(unsigned index) {
  _index = index;

  _music.end();
  load((std::string(_path) + _musiclist[_index]).c_str());
  _music.pause();
  _musicInfo._pause = false;

  if (_musicInfo._cycle)
    _music.loop();
  if (_musicInfo._mute)
    _music.muteVolume();
}

void MusicPlayer::next() {
  if (_index == _musiclist.size() - 1) {
    if (!_musicInfo._pause) {
      _music.pause();
      _musicInfo._pause = true;
      _musicInfo._end = true;
    }
    _musicInfo._reset = true;
    return;
  }

  if (_musicInfo._begin)
    _musicInfo._begin = false;

  play(++_index);
}

void MusicPlayer::prev() {
  if (_index == 0) {
    if (!_musicInfo._pause) {
      _music.pause();
      _musicInfo._pause = true;
      _musicInfo._begin = true;
    }
    _musicInfo._reset = true;
    return;
  }

  if (_musicInfo._end)
    _musicInfo._end = false;

  play(--_index);
}

void MusicPlayer::readMusicDirectory(const char *dirname) {
  DIR *dir;
  if ((dir = opendir(dirname)) == NULL)
    error("music direcotry error");

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
    if (entry->d_type == DT_REG)
      _musiclist.emplace_back(entry->d_name);

  // random
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(_musiclist.begin(), _musiclist.end(), g);
}

std::binary_semaphore lock1{0};
std::binary_semaphore lock2{0};

void MusicPlayer::runDirectory() {
  _musicInfo._cursor = _index;
  _musicInfo._dir = true;
  while (_musicInfo._dir) {
    lock1.acquire();
  begin:
    switch (getch()) {
    case 'd':
      _musicInfo._dir = false;
      break;
    case 'j':
      if (_musicInfo._cursor == _musiclist.size() - 1)
        break;
      ++_musicInfo._cursor;
      break;
    case 'k':
      if (_musicInfo._cursor == 0)
        break;
      --_musicInfo._cursor;
      break;
    case '\n':
      play(_musicInfo._cursor);
      _musicInfo._dir = false;
      break;
    case '/':
      _musicInfo._find = true;
      find();
      break;
    case 'n':
      if (!_musicInfo.find_result.empty()) {
        bool fin = false;
        for (const auto &result : _musicInfo.find_result) {
          if (result.first > _musicInfo._cursor) {
            _musicInfo._resultpos = result.first;
            fin = true;
            break;
          }
        }
        if (!fin)
          _musicInfo._resultpos = _musicInfo.find_result[0].first;
        _musicInfo._cursor = _musicInfo._resultpos;
        _callback(_musicInfo._cursor);
      }
      break;
    case 'p':
      if (!_musicInfo.find_result.empty()) {
        bool fin = false;
        const auto &target = _musicInfo.find_result;
        for (int i = target.size() - 1; i >= 0; --i) {
          if (target[i].first < _musicInfo._cursor) {
            _musicInfo._resultpos = target[i].first;
            fin = true;
            break;
          }
        }
        if (!fin)
          _musicInfo._resultpos = target[target.size() - 1].first;
        _musicInfo._cursor = _musicInfo._resultpos;
        _callback(_musicInfo._cursor);
      }
      break;
    default:
      goto begin;
    }
    lock2.release();
  }
}

void MusicPlayer::find() {
  curs_set(1);
  MusicName &music = _musicInfo._musicname;
  while (true) {
    lock2.release();
    int c = getch();
    if (c == '\n')
      break;
    else if (c == KEY_BACKSPACE)
      music.pop_back();
    else
      music.push_back(c);
  }
  if (!_musicInfo.find_result.empty()) {
    std::vector<int> minusres;
    for (const auto &result : _musicInfo.find_result) {
      int i = result.first - _musicInfo._cursor;
      if (i < 0)
        i = 0 - i;
      minusres.push_back(i);
    }
    int min = minusres[0];
    int pos = 0;
    for (int i = 1; i < minusres.size(); ++i) {
      if (min > minusres[i]) {
        min = minusres[i];
        pos = i;
      }
    }
    _musicInfo._resultpos = _musicInfo.find_result[pos].first;
    _musicInfo._cursor = _musicInfo._resultpos;
    _callback(_musicInfo._cursor);
  }
  _musicInfo._find = false;
  curs_set(0);
}
