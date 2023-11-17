#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "MusicCore.h"
#include <string>
#include <vector>

class MusicPlayer {
public:
  MusicPlayer(const char *musicDir);
  ~MusicPlayer();

  void run();

private:
  void readMusicDirectory(const char *dirname);

  void load(const char *name);

  void next();
  void prev();

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
    void clear() { printf("\033[H\033[J"); }
    void runUI();

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
  } _musicInfo;
};

#endif
