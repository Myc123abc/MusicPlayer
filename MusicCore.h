#ifndef MUSICCORE_H
#define MUSICCORE_H

#include "./bass/bass.h"

class MusicCore {
public:
  MusicCore(unsigned volume = 100);
  ~MusicCore();

  void load(const char *name);
  void play();
  void pause();
  void end();

  void raiseVolume(int num = 1);
  void lowerVolume(int num = 1);
  void muteVolume();

  void raisePosition(int num = 1);
  void lowerPosition(int num = 1);

  void loop();

  const char *getName() { return _name; }
  double getTime() { return _time; }

  double getCurrentTime();
  unsigned getVolume() { return _volume; }

private:
  void setVolume(int volume);

  HSTREAM _music;
  char _name[256];
  unsigned _volume;
  double _time;
  QWORD _len;

  bool _pause;
  bool _mute;
  bool _loop;
};

inline void MusicCore::setVolume(int volume) {
  BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, volume * 100);
}

#endif
