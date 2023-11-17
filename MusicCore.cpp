#include "MusicCore.h"
#include "bass/bass.h"
#include "bass/bassflac.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <string>

// flac cannot used
MusicCore::MusicCore(unsigned volume) : _volume(volume) {
  BASS_Init(-1, 44100, 0, 0, 0);
  setVolume(_volume);
}

MusicCore::~MusicCore() { BASS_Free(); }

void MusicCore::load(const char *name) {
  strcpy(_name, name);

  std::string musicpath(_name);
  std::string musictype = musicpath.substr(musicpath.find_last_of('.') + 1);
  if (musictype == "mp3")
    _music = BASS_StreamCreateFile(FALSE, _name, 0, 0, BASS_STREAM_AUTOFREE);
  else if (musictype == "flac")
    _music =
        BASS_FLAC_StreamCreateFile(FALSE, _name, 0, 0, BASS_STREAM_AUTOFREE);
  else {
    fprintf(stderr, "unsupport this music type!\n");
    exit(EXIT_FAILURE);
  }

  _len = BASS_ChannelGetLength(_music, BASS_POS_BYTE);
  _time = BASS_ChannelBytes2Seconds(_music, _len);

  _pause = false;
  _mute = false;
  _loop = false;
}

void MusicCore::play() { BASS_ChannelPlay(_music, TRUE); }

void MusicCore::pause() {
  if (!_pause) {
    BASS_ChannelPause(_music);
    _pause = true;
  } else {
    BASS_ChannelPlay(_music, FALSE);
    _pause = false;
  }
}

void MusicCore::end() { BASS_StreamFree(_music); }

void MusicCore::raiseVolume(int num) {
  if (_volume < 100) {
    _volume += num;
    if (_volume > 100)
      _volume = 100;
    setVolume(_volume);
  }
}

void MusicCore::lowerVolume(int num) {
  if (_volume > 0) {
    _volume -= num;
    if (_volume < 0)
      _volume = 0;
    setVolume(_volume);
  }
}

void MusicCore::muteVolume() {
  if (!_mute) {
    setVolume(0);
    _mute = true;
  } else {
    setVolume(_volume);
    _mute = false;
  }
}

void MusicCore::raisePosition(int num) {
  QWORD pos = BASS_ChannelGetPosition(_music, BASS_POS_BYTE);
  pos += BASS_ChannelSeconds2Bytes(_music, num);
  if (pos > _len)
    pos = _len - 1;
  BASS_ChannelSetPosition(_music, pos, BASS_POS_BYTE);
}

void MusicCore::lowerPosition(int num) {
  QWORD pos = BASS_ChannelGetPosition(_music, BASS_POS_BYTE);
  QWORD size = BASS_ChannelSeconds2Bytes(_music, num);
  if (pos < size)
    pos = 0;
  else
    pos -= size;
  BASS_ChannelSetPosition(_music, pos, BASS_POS_BYTE);
}

double MusicCore::getCurrentTime() {
  QWORD pos = BASS_ChannelGetPosition(_music, BASS_POS_BYTE);
  double time = BASS_ChannelBytes2Seconds(_music, pos);
  return time;
}

void MusicCore::loop() {
  if (!_loop) {
    BASS_ChannelFlags(_music, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    _loop = true;
  } else {
    BASS_ChannelFlags(_music, 0, BASS_SAMPLE_LOOP);
    _loop = false;
  }
}
