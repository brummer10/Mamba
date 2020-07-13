# Mamba
Virtual Midi keyboard for Jack Audio Connection Kit

![Mamba](https://github.com/brummer10/Mamba/raw/master/Mamba.png)


## Features

- Virtual Midi Keyboard for [Jack Audio Connection Kit](https://jackaudio.org/)
- including [NSM](https://linuxaudio.github.io/new-session-manager/) support
- Channel selector
- Bank and Program selector
- Keyboard mapping for qwertz, qwerty and azerty selectable from menu
- PC Keyboard mapping selector from C0 to C4
- Pitchbend, Balance, Modwheel, Detune, Expression, Attack, Release, Volume and Velocity controllers
- Sustain and Sostenuto switches
- Midi Through: forward midi input to output
- Midi input highlighting
- resizable to a full range 127 key view

## Dependencys

- libcairo2-dev
- libx11-dev
- liblo-dev
- libsig++-2.0-dev
- libjack-(jackd2)-dev

## Build

- git submodule init
- git submodule update
- make
- sudo make install # will install into /usr/bin
