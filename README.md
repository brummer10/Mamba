# Mamba
Virtual MIDI Keyboard and MIDI file player/recorder for Jack Audio Connection Kit

![Mamba](https://github.com/brummer10/Mamba/raw/master/Mamba.png)


## Description

### Virtual MIDI Keyboard

Mamba comes with some predefined key-maps, qwertz, qwerty, azerty(fr) and azerty(be), but you could define your own
with the included Key-map Editor as well. Beside the computer keyboard and mouse, Mamba supports jack MIDI in and
ALSA (seq) MIDI in. Output goes to jack MIDI out. Every channel use it's own Colour to display the played Notes per channel.

The MIDI-CC controllers on GUI could be controlled by the mouse or the keyboard. With the mouse you could use the mouse-wheel,
or, press the left mouse button and move the mouse up/down. A special case is the Pitch-wheel control, to use it with the
mouse-wheel, you must press the mouse-wheel while moving it. This is because of the spring back to default value behave of
the Pitch-wheel. So, when you release the mouse-wheel press, the Pitch-wheel controller will spring back to default value '64'.

To use the keyboard to controll the MIDI-CC controllers on GUI you could use the up/down key's on your PC-keeyboard.
To select a controller it must be under the mouse pointer. When using the PC-keyboard to control the Pich-wheel, it wouldn't
spring back to default.

### 16 Channel Live MIDI Looper/Recorder: 

To record a loop, press "Play" and then to start recording press "Record".
To stop recording press record again. Playback will start immediately.

The first recorded channel will become the Master channel. This one set the time frame for all later recorded loops.
For the Master Channel the recording time will be stretched/clipped to match the next full beat time point.

To record a new loop, switch to a other channel, select your instrument and press "Record" again to start recording.

The later recorded loops will be synced to the master loop. When the recording time extend the absolute Master loop time
record will be switched off. Absolute time is not bound to the loop point, so you could record loops crossing it.
You could as well stop recording by press "Record" again before the time expires.

Each Channel could be cleared and re-recorded separate at any time.
even when you press "Record" on a already recorded channel, it will be cleared before recording starts.

You could record the connected input device or play the Keyboard itself.

### MIDI File player

You could select a MIDI file with the File Selector, or just drag'n drop it from your Filemanager on the Keyboard.
It will be loaded in the play buffer of the first channel, regardless how much channels it use. 
You could use then channel 2 - 16 to record your own playing into it. To play along with it you could use any channel.
A loaded file will become the Master channel for the looper.

To save your work just go to Menu -> "File" -> "Save MIDI file as", select the path and enter a file name.
If you don't give the usual file extension Mamba will add the extension .midi befor save it.

### Fluidsynth

![Fluidsynth-settings](https://github.com/brummer10/Mamba/raw/master/Fluidsynth-settings.png)

You could load a Sound-font via the Menu -> "Fluidsynth" -> "Load Sound-font", or drag'n drop it from your Filemanager on the Keyboard.
Mamba will start the Fluidsynth engine and do the needed connections so that you could just play along.
Menu -> "Fluidsynth" -> "Settings" will pop-up a new Window were you could select the Instrument for the channel and do settings for Fluisynth Reverb and Chorus.
All your Settings will be saved on exit, so on next start you could just play along.


## Features

- Virtual MIDI Keyboard for [Jack Audio Connection Kit](https://jackaudio.org/)
- Including [ALSA](https://www.alsa-project.org/wiki/Main_Page) MIDI in support
- Including [NSM](https://linuxaudio.github.io/new-session-manager/) support
- Including [gettext](https://www.gnu.org/software/gettext/) localization support
- Including [fluidsynth](https://github.com/FluidSynth/fluidsynth) support
- Sound-font loader for fluidsynth
- Controls for fluidsynth reverb, chorus and channel pressure
- Instrument selector for fluidsynth
- Channel selector
- Bank and Program selector
- Keyboard mapping for qwertz, qwerty, azerty(fr) and azerty(be) selectable from menu
- Key-map Editor to setup a custom Key-map
- PC Keyboard mapping selector from C0 to C4
- Pitch-bend, Balance, Mod-wheel, Detune, Expression, Attack, Release, Volume and Velocity controllers
- Sustain and Sostenuto switches
- Connection management Menu
- Support MIDI-file load, save, record and play in loop
- BPM controller for playback speed
- Support MIDI Beat Clock for playback speed
- MIDI Through: forward ALSA MIDI in to jack
- MIDI Through: forward MIDI input to output
- MIDI input highlighting
- Resizeable to a full range 127 key view
- Load MIDI-files on command-line
- Support jack_transport to start/stop MIDI-Loops
- Keyboard Shortcuts
- `ctrl + r` toggle Record Button
- `ctrl + p` toggle Play Button
- `ctrl + l` open load file dialogue
- `ctrl + s` open save file dialogue
- `ctrl + a` show info box
- `ctrl + k` show Key-map Editor
- `ctrl + q` quit
- `ctrl + c` quit


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

