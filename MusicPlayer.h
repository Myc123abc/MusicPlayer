#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "MusicCore.h"
#include <functional>
#include <ncurses.h>
#include <semaphore>
#include <string>
#include <vector>

class MusicName {
public:
  bool empty() const noexcept { return _s.empty(); }

  void pop_back() noexcept {
    if (!_s.empty()) {
      if (!_ivec.empty() && _s.size() == _ivec[_ivec.size() - 1]) {
        _ivec.pop_back();
        _s.pop_back();
        _s.pop_back();
        --_size;
      }
      _s.pop_back();
      --_size;
    }
  }

  void push_back(int c) {
    _s.push_back(c);
    ++_size;
    if (c & 0x80) {
      static int cnt = 0;
      if (++cnt == 3) {
        cnt = 0;
        --_size;
        _ivec.push_back(_s.size());
      }
    }
  }

  const char *c_str() const noexcept { return _s.c_str(); }
  size_t size() const noexcept { return _s.size(); }
  size_t length() const noexcept { return _size; }
  void clear() noexcept {
    _s.clear();
    _ivec.clear();
    _size = 0;
  }

private:
  std::string _s;
  std::vector<int> _ivec;
  size_t _size = 0;
};

class MusicPlayer {
public:
  MusicPlayer(const char *musicDir);
  ~MusicPlayer();

  void run();

private:
  void readMusicDirectory(const char *dirname);

  void load(const char *name);

  void play(unsigned index);
  void next();
  void prev();

  void runDirectory();
  void find();

private:
  char _path[256];
  MusicCore _music;
  std::vector<std::string> _musiclist;
  unsigned _index = 0;

private:
  friend struct MusicInfo;
  struct MusicInfo {
    MusicInfo();
    ~MusicInfo();

    void load(MusicPlayer *p);
    void update();
    void print();
    void runUI();
    void directory();

    MusicPlayer *_p;
    char _name[256];
    unsigned _minute;
    unsigned _second;
    unsigned _currminute;
    unsigned _currsecond;
    bool _pause;
    bool _cycle;
    bool _mute;
    bool _end;
    bool _begin;
    bool _reset;
    int _row, _col;

    bool _dir;
    WINDOW *_dirwin;
    int _cursor;
    void checkout(int *p1, int *p2, int *p3, int highth, int forfind);
    MusicName _musicname;
    bool _find;
    std::vector<std::pair<int, std::vector<int>>> find_result;
    int _resultpos;
  } _musicInfo;
  std::function<void(int)> _callback;
};

#endif
