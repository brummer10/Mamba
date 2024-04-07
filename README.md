# Mamba

Virtual MIDI keyboard and MIDI file player/recorder for the JACK Audio Connection Kit

![Mamba](https://github.com/brummer10/Mamba/raw/master/Mamba.png)

## Description

### Virtual MIDI Keyboard

Mamba comes with some predefined key-maps: QWERTZ, QWERTY, AZERTY (fr) and AZERTY (be), but you can
also define your own key-maps with the included key-map editor. In addition to computer keyboard
and mouse input, Mamba also supports JACK and ALSA MIDI In and Out connections. Incoming MIDI notes
are displayed on the keyboard, each with their own colour according to their MIDI channels. JACK
and ALSA connections can be managed within the connection menu.

Mamba implements full [Scala](https://www.huygens-fokker.org/scala/) tuning support, which means it
can load Scala tuning files (`*.scl`) and Scala MIDI keymap files (`*.kbm`) to support micro-tonal
tunings. These can be loaded simply via drag and drop or via the "File" -> "Load Scala" menu. The
keyboard layout "tries" to reflect the loaded tuning ratio scala. MIDI keyboard mapping works for
both JACK and ALSA MIDI input.

The MIDI-CC control dials on the GUI can be used via the mouse or the keyboard. Using the mouse you
can hover over a dial and turn the mouse-wheel, or left-click and drag the mouse up/down. The
pitch-wheel control is a special case: to use it with the mouse-wheel, you need to press the
mouse-wheel while moving it. This is because of the auto-return-to-default value behaviour of the
pitch-wheel. This means, when you release the mouse-wheel press, the pitch-wheel control will
spring back to its default value `64`.

To change the MIDI-CC control dials on the GUI via the PC keyboard, use the up/down arrow keys. For
a control to receive keyboard input, it must be under the mouse pointer. When using the PC keyboard
to control the pitch-wheel, it does not spring back to the default value.

### 16 Channel Live MIDI Looper/Recorder

To record a loop, press "Play" and then, to start recording, press "Record". To stop recording
press "Record" again. Playback will start immediately.

The first recorded channel will become the Master Channel loop. This loop sets the length for all
loops recorded afterwards. The loop length of the Master Channel will be adjusted (i.e. extended or
cut) to the nearest full beat according to the tempo .

To record a new loop, switch to another MIDI channel, select your instrument and press "Record"
again to start recording.

The loops recorded after the Master Channel loop will be synced to it. When the recording time
exceeds the absolute Master Channel loop time, recording will be turned off, but the playback
continues and loops back. Absolute time is not bound to the loop point, so you can record loops
crossing it. You can also stop recording by pressing "Record" again before the time expires.

Each Channel can be cleared and re-recorded separately at any time. When you press "Record" on an
already recorded channel, it will be cleared automatically before recording starts.

You can record events from the connected MIDI input device or use the the keyboard to play and
record.

### MIDI File Player

You can select a MIDI file via the menu "File" -> "Load MIDI", or just drag and drop a MIDI file
from your file manager onto the keyboard. It will be loaded into the play buffer of the first
channel, regardless of how many MIDI channels it uses. You can then use channels 2 - 16 to record
your own playing on top of it. To play along with it you can use any channel. A loaded MIDI file
will become the Master Channel loop of the looper.

To save your work, just go to menu "File" -> "Save MIDI file as", select a path and enter a file
name. If the filename doesn't have one of the common MIDI file name extensions, Mamba will add the
extension `.midi` before saving the file.

### FluidSynth

![FluidSynth settings](https://github.com/brummer10/Mamba/raw/master/Fluidsynth-settings.png)

You can load an SF2 SoundFont via the menu "Fluidsynth" -> "Load SoundFont", or drag and drop it
from your file manager onto the keyboard. Mamba will start the FluidSynth engine and create the
necessary connections so that you can just start playing the SoundFont. The menu "Fluidsynth" ->
"Settings" will open a new window were you can select the instrument for the currently selected
MIDI channel and change the settings for the FluidSynth reverb and chorus. Also, you can select the
tuning scale to use there; the available options are *Just Intonation*, *12-edo* or *Scala*. All
your Settings will be saved on exit, so when you next open the application you can just start
playing.

## Features

- Virtual MIDI keyboard for [JACK Audio Connection Kit](https://jackaudio.org/)
- Includes [ALSA](https://www.alsa-project.org/wiki/Main_Page) MIDI support
- Includes [NSM](https://linuxaudio.github.io/new-session-manager/) support
- Includes [gettext](https://www.gnu.org/software/gettext/) localization support
- Includes [FluidSynth](https://github.com/FluidSynth/fluidsynth) support
- SoundFont loader for FluidSynth
- Controls for FluidSynth reverb, chorus and channel pressure
- Instrument selector for FluidSynth
- Micro-tonal tuning for FluidSynth
- MIDI Channel selector
- MIDI Bank and Program selector
- Keyboard mapping for QWERZ, QWERTY, AZERTY (fr) and AZERTY (be) selectable from the menu
- Key-map editor to set up a custom key-map
- PC keyboard mapping selector from C0 to C4
- Control dials for sending MIDI Pitch-bend, and MIDI Control Changes (Balance, ModWheel (Modulation), Detune, Expression, Attack, Release and Volume) and setting the Note On velocity
- MIDI Sustain and Sostenuto controller switches
- Connection management menu
- Supports MIDI file loading, saving, recording and loop-playing
- BPM controller for playback speed
- Supports MIDI Timing Clock for playback speed
- MIDI Through: forwarding ALSA MIDI in to JACK
- MIDI Through: forwarding MIDI input to output
- MIDI input notes highlighting on keyboard
- Resizable up to a full-range 127 keys view
- Load MIDI files from the command-line
- Supports JACK transport to start and stop MIDI-Loops
- Supports loading Scala tuning (`*.scl`) files
- Supports loading Scala MIDI keyboard mapping (`*.kbm`) files

## Keyboard Shortcuts

| Shortcut      | Function                 |
| ------------- | ------------------------ |
| `ctrl + 0-4`  |  change octave           |
| `ctrl + t`    |  toggle MIDI Through     |
| `ctrl + g`    |  toggle Grab Keyboard    |
| `ctrl + r`    |  toggle Record Button    |
| `ctrl + p`    |  toggle Play Button      |
| `ctrl + l`    |  open load file dialogue |
| `ctrl + s`    |  open save file dialogue |
| `ctrl + a`    |  show info box           |
| `ctrl + k`    |  show Key-map Editor     |
| `ctrl + q`    |  quit                    |
| `ctrl + c`    |  quit                    |

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

## Prebuild Binary

[Mamba.zip](https://github.com/brummer10/Mamba/releases/download/Latest/Mamba.zip)

## Build

```con
git submodule init
git submodule update
make
sudo make install # will install into /usr/bin
```

### With Localization Support

```con
git submodule init
git submodule update
make nls
sudo make install # will install into /usr/bin
```
