#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "MusicCore.h"
#include <ncurses.h>
#include <string>
#include <vector>
#include <semaphore>

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
  } _musicInfo;
};

#endif
