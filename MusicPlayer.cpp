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

  init_pair(1, COLOR_GREEN, -1);

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
      lock2.release();
      break;
    case 'j':
      if (_musicInfo._cursor == _musiclist.size() - 1) {
        lock2.release();
        break;
      }
      ++_musicInfo._cursor;
      lock2.release();
      break;
    case 'k':
      if (_musicInfo._cursor == 0) {
        lock2.release();
        break;
      }
      --_musicInfo._cursor;
      lock2.release();
      break;
    case '\n':
      play(_musicInfo._cursor);
      _musicInfo._dir = false;
      lock2.release();
      break;
    default:
      goto begin;
      break;
    }
  }
}
