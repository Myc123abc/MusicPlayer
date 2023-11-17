#include "MusicPlayer.h"
#include "bass/bass.h"
#include "error.h"
#include "other.h"
#include <chrono>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

// 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

/*
 *  MusicPlayer
 */

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
  _musicInfo.runUI();
  while (true) {
    switch (getch()) {
    case ' ':
      _music.pause();
      _musicInfo._pause = !_musicInfo._pause;
      break;
    case 'q':
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
    }
  }
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
  _music.end();
  load((std::string(_path) + _musiclist[++_index]).c_str());
  _music.pause();
  _musicInfo._pause = false;
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
  _music.end();
  load((std::string(_path) + _musiclist[--_index]).c_str());
  _music.pause();
  _musicInfo._pause = false;
}

void MusicPlayer::readMusicDirectory(const char *dirname) {
  DIR *dir;
  if ((dir = opendir(dirname)) == NULL)
    error("music direcotry error");

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
    if (entry->d_type == DT_REG)
      _musiclist.emplace_back(entry->d_name);
}

/*
 * MusicInfo
 */

MusicPlayer::MusicInfo::MusicInfo()
    : _pause(true), _cycle(false), _mute(false), _begin(true), _end(false),
      _reset(false) {}
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

void MusicPlayer::MusicInfo::runUI() {
  std::thread([this] {
    static unsigned cnt = 0;
    while (true) {
      print();
      if (_reset) {
        if (_begin)
          printf("This is the first music.\n");
        if (_end)
          printf("This is the last music\n");
        if (++cnt == 10) {
          cnt = 0;
          _reset = false;
        }
      } else
        printf("\n");
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);

      if (_currminute == _minute && _currsecond == _second) {
        if (!_pause && !_cycle) {
          if (!_end) {
            _p->next();
          } else {
            _p->_music.end();
            _p->load(_p->_music.getName());
            _pause = true;
          }
        }
      }
    }
  }).detach();
}

void MusicPlayer::MusicInfo::print() {
  update();
  clear();
  printf("%s\n", _name);
  printf("[ %s ", _pause ? "" : "");
  printf(" %s  |  ", _cycle ? "" : "");
  if (_mute)
    printf("󰝟 ");
  else
    printf(" ");
  printf("%3u%% ] ", _p->_music.getVolume());
  printf("[%u:%02u/", _currminute, _currsecond);
  printf("%u:%02u]\n", _minute, _second);
}
