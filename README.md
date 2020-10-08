# Mamba
Virtual MIDI Keyboard and MIDI file player/recorder for Jack Audio Connection Kit

![Mamba](https://github.com/brummer10/Mamba/raw/master/Mamba.png)


## Features

- Virtual MIDI Keyboard for [Jack Audio Connection Kit](https://jackaudio.org/)
- Including [ALSA](https://www.alsa-project.org/wiki/Main_Page) MIDI in support
- Including [NSM](https://linuxaudio.github.io/new-session-manager/) support
- Including [gettext](https://www.gnu.org/software/gettext/) localization support
- Including [fluidsynth](https://github.com/FluidSynth/fluidsynth) support
- Soundfont loader for fluidsynth
- Controls for fluidsynth reverb, chorus and channel pressure
- Instrument selector for fluidsynth
- Channel selector
- Bank and Program selector
- Keyboard mapping for qwertz, qwerty, azerty(fr) and azerty(be) selectable from menu
- Keymap Editor to setup a custom Keymap
- PC Keyboard mapping selector from C0 to C4
- Pitchbend, Balance, Modwheel, Detune, Expression, Attack, Release, Volume and Velocity controllers
- Sustain and Sostenuto switches
- Connection management Menu
- Support MIDI-file load, save, record and play in loop
- BPM controller for playback speed
- Support MIDI Beat Clock for playback speed
- MIDI Through: forward ALSA MIDI in to jack
- MIDI Through: forward MIDI input to output
- MIDI input highlighting
- Resizable to a full range 127 key view
- Load MIDI-files on command-line
- Support jack_transport to start/stop MIDI-Loops
- Keyboard Shortcuts
- `ctrl + r` toggle Record Button
- `ctrl + p` toggle Play Button
- `ctrl + l` open load file dialog
- `ctrl + s` open save file dialog
- `ctrl + a` show info box
- `ctrl + k` show Keymap Editor
- `ctrl + q` quit
- `ctrl + c` quit

## Description

16 Channel Live MIDI Looper: 

To record a loop, press "Play" and then to start recording press "Record".
To stop recording press record again. Playback will start immediately.

The first recorded channel will become the Master channel. This one set the time frame for all later recorded loops.
For the Master Channel the recording time will be streched/clipped to match the next full beat time point.

To record a new loop, switch to a other channel, select your instrument and press "Record" again to start recording.

The later recorded loops will be synced to the master loop. When the recording time extend the absolute Master loop time
record will be switched off. Absolute time is not bound to the loop point, so you could record loops crossing it.
You could as well stop recording by pess "Record" again before the time expires.

Each Channel could be cleared and re-recorded seperate at any time.

You could record the connected input device or play the Keyboard itself.

## Dependencies

- libfluidsynth-dev
- libc6-dev
- libsmf-dev
- libcairo2-dev
- libx11-dev
- liblo-dev
- libsigc++-2.0-dev
- libjack-(jackd2)-dev
- libasound2-dev

## Build

- git submodule init
- git submodule update
- make
- sudo make install # will install into /usr/bin

## Build with localization support

- git submodule init
- git submodule update
- make nls
- sudo make install # will install into /usr/bin

