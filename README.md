# Mamba
Virtual Midi Keyboard for Jack Audio Connection Kit

![Mamba](https://github.com/brummer10/Mamba/raw/master/Mamba.png)


## Features

- Virtual Midi Keyboard and Midi file player/recorder  for [Jack Audio Connection Kit](https://jackaudio.org/)
- Including [NSM](https://linuxaudio.github.io/new-session-manager/) support
- Channel selector
- Bank and Program selector
- Keyboard mapping for qwertz, qwerty and azerty selectable from menu
- PC Keyboard mapping selector from C0 to C4
- Pitchbend, Balance, Modwheel, Detune, Expression, Attack, Release, Volume and Velocity controllers
- Sustain and Sostenuto switches
- Midi Through: forward midi input to output
- Midi input highlighting
- Resizable to a full range 127 key view
- Save and load Midi-files
- Load Midi-files on command-line
- Record and play Midi-Loops
- Support jack_transport to start/stop Midi-Loops
- Keyboard Shortcuts
- `ctrl + r` toggle Record Button
- `ctrl + p` toggle Play Button
- `ctrl + l` open load file dialoge
- `ctrl + s` open save file dialoge
- `ctrl + a` show info box
- `ctrl + q` quit
- `ctrl + c` quit

## Dependencies

- libsmf
- libcairo2-dev
- libx11-dev
- liblo-dev
- libsigc++-2.0-dev
- libjack-(jackd2)-dev

## Build

- git submodule init
- git submodule update
- make
- sudo make install # will install into /usr/bin
