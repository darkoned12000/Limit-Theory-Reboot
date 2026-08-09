// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// audio-demo.cpp — Revival demo of Josh Parnell's LT SoundStudio
// (src/old/soundstudio/soundstudio.cpp).
//
// The engine's procedural-audio DSP layer (src/liblt/Audio/) is fully intact but
// exercised by nothing in the current engine. This demo proves the layer works
// end to end by porting SoundStudio's synthesis chain nearly verbatim:
//
//   note tables (minor / minor-pentatonic scales, noteFrequency octave math)
//   additive-synth generators: Glass, MySynth (exponential-decay partials)
//   Wind (fractal value-noise) drone
//   Signal_Instrument -> Signal_Compress -> 10x nested Signal_Delay wash
//   Signal_Render -> WAV_Write (data.wav) + GetSoundEngine()->Play
//
// What was NOT ported: the original UI (CreateUIGroupY/CreateUIPanel/etc. were
// removed from the engine) and the FMOD backend (SoundEngine_Fmod was removed).
// The note grid is now a plain keyboard mapping and audio plays via
// SoundEngine_SFML().
//
// Controls:
//   1-7 Q-U A-J Z-M : play a note (4 rows, low -> high, minor scale)
//   S               : write the last-played piece to data.wav
//   Q / Esc / Ctrl+W: quit

#define LTE_CONSOLE
#include "LTE/LTE.h"

#include "Module/SoundEngine.h"

#include "Audio/Audio.h"
using namespace Audio;

double kFrequencyTable[] = {
  440.00, 466.16, 493.88, 523.25, 554.37, 587.33,
  622.25,  659.26, 698.46, 739.99, 783.99, 830.61,
};

int kMinorScale[] = { 0, 2, 3, 5, 7, 8, 10 };
int kMinorPentaScale[] = { 0, 3, 5, 7, 10 };

inline double noteFrequency(int index) {
  index -= 57;
  double octave = 1.;
  while (index < 0) {
    index += 12;
    octave /= 2.;
  }
  while (index >= 12) {
    index -= 12;
    octave *= 2.;
  }
  return kFrequencyTable[index] * octave;
}

inline double Quantize(double s, double step) {
  return Floor(s / step) * step;
}

inline double noise(double t) {
  return Fract(Sin(t) * 4437.37158237193);
}

inline double cnoise(double t) {
  double f = Floor(t);
  double i = t - f;
  return Mix(noise(f), noise(f + 1.), i);
}

inline double shape(double t) {
  double x = t * .686306;
  double a = 1 + Exp(-.75 * Sqrt(Abs(x)));
  return (Exp(x) - Exp(-x*a)) / (Exp(x) + Exp(-x));
}

struct Glass {
  double operator()(double f, double t) const {
    double s = 1. / f;
    return s * 4.0 * t * Exp(-4.0 * t * f);
  }
};

struct MySynth {
  double operator()(double f, double t) const {
    return (1. / (f * f)) * (Exp(-4. * t  * f) + Exp(-16. * t * f));
  }
};

template <class AmplitudeType>
struct AdditiveSynthType : public GeneratorT {
  AmplitudeType amplitude;

  AdditiveSynthType(AmplitudeType const& amplitude) :
    amplitude(amplitude)
    {}

  double Step(double i) const {
    return 2;
  }

  double Get(Note const& note, double t) const {
    double lt = kTau * note.frequency * t;
    double s = 0;
    double i = 1;
    for (double f = 1; f * note.frequency < 44100.0 / 2.0; f += Step(i++))
      s += Sin(lt * f) * amplitude(f, t);
    return s;
  }
};

template <class T>
GeneratorT* AdditiveSynth(const T& t) {
  return new AdditiveSynthType<T>(t);
}

struct Sine : public GeneratorT {
  double Get(Note const& note, double t) const {
    t *= note.frequency;
    return Sin(kTau * t);
  }
};

struct IdealSquare : public GeneratorT {
  double Get(Note const& note, double t) const {
    return Fract(t * note.frequency) > 0.5 ? 1.0 : -1.0;
  }
};

struct Wind : public GeneratorT {
  double Get(Note const& note, double t) const {
    return 2.0 * Fractal(ValueNoise1D, (float)(t * 440.0), 12, 1.8f) - 1.0;
  }
};

struct AudioDemo : public Program {
  Module soundEngine;
  AutoPtr<Array<float>> lastBuf;
  int autoNote;

  AudioDemo(int autoNote) : autoNote(autoNote) {
    window = Window_Create("LT Audio Demo", V2U(480, 480), false, false);
    Renderer_Initialize();
  }

  void OnInitialize() override {
    LTE_Initialize();

    soundEngine = SoundEngine_SFML();
    Module_RegisterGlobal(soundEngine);

    printf("=== LT Audio Demo (from SoundStudio) ===\n");
    printf("Rows of keys play notes:\n");
    printf("  1-7   low octave\n");
    printf("  Q-U   ...\n");
    printf("  A-J   ...\n");
    printf("  Z-M   high octave\n");
    printf("  S     write last piece to data.wav\n");
    printf("  Q/Esc Ctrl+W  quit\n");

    /* Headless mode: audio-demo <note> plays one note, writes data.wav, exits. */
    if (autoNote >= 0) {
      Play(autoNote);
      SaveWav();
      deleted = true;
    }
  }

  void OnDelete() override {
    soundEngine = nullptr;
  }

  void Play(const int& note) {
    printf("Playing note %d\n", note);

    Pattern p;
    p
      << Note(0, noteFrequency(note + 3), 1000000)
      << Note(0, noteFrequency(note + 10), 1000000)
      << Note(50000, noteFrequency(note + 0), 100000)
      << Note(100000, noteFrequency(note + 7), 100000)
      << Note(150000, noteFrequency(note + 10), 100000);

    Signal s = Signal_Instrument(AdditiveSynth(Glass()), p);
    s = Signal_Sum(s, Signal_Instrument(new Wind, Pattern(Note(0, 0, 1000000))));

    s = Signal_Compress(s, 1.0);

    for (int i = 0; i < 10; ++i)
      s = Signal_Sum(s, Signal_Delay(s, 5761 + rand() % 25000, 0.5, 0.5));

    AutoPtr<Array<float>> buf = Signal_Render(s, 8);
    GetSoundEngine()->Play(*buf);
    lastBuf = buf;
  }

  void SaveWav() {
    if (!lastBuf) {
      printf("No piece rendered yet -- play a note first.\n");
      return;
    }

    Array<int> iBuf(lastBuf->size());
    for (size_t i = 0; i < lastBuf->size(); ++i)
      iBuf[i] = (int)(INT_MAX * (*lastBuf)[i]);
    WAV_Write("data.wav", iBuf, 44100, 1);
    printf("Wrote data.wav (%zu samples)\n", lastBuf->size());
  }

  void OnUpdate() override {
    if (Keyboard_Pressed(Key_Q) || Keyboard_Pressed(Key_Escape) ||
        (Keyboard_Control() && Keyboard_Pressed(Key_W)))
    {
      deleted = true;
      return;
    }

    if (Keyboard_Pressed(Key_S)) {
      SaveWav();
      return;
    }

    /* Note grid: 4 rows x 7 keys, minor scale. */
    static Key const rows[4][7] = {
      { Key_N1, Key_N2, Key_N3, Key_N4, Key_N5, Key_N6, Key_N7 },
      { Key_Q,  Key_W,  Key_E,  Key_R,  Key_T,  Key_Y,  Key_U  },
      { Key_A,  Key_S,  Key_D,  Key_F,  Key_G,  Key_H,  Key_J  },
      { Key_Z,  Key_X,  Key_C,  Key_V,  Key_B,  Key_N,  Key_M  },
    };

    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 7; ++j)
        if (Keyboard_Pressed(rows[i][j])) {
          Play(20 + i * 12 + kMinorScale[j]);
          return;
        }
  }
};

int main(int argc, char const* argv[]) {
  int autoNote = -1;
  if (argc > 1)
    autoNote = atoi(argv[1]);
  AudioDemo(autoNote).Execute();
  return 0;
}
