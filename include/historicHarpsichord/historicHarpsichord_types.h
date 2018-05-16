/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef HISTORICHARPSICHORDTYPES_H
#define HISTORICHARPSICHORDTYPES_H
#include <gtk/gtk.h>

/**
 * HistoricHarpsichordPrefs holds information on user preferences.
 */
typedef struct HistoricHarpsichordPrefs
{
  // audio and MIDI driver
  GString *audio_driver;  /* the name of the audio driver to be used */
  GString *midi_driver;   /* the name of the MIDI driver to be used */
  // PortAudio options
  GString *portaudio_device;
  unsigned int portaudio_sample_rate;/**< sample rate in Hz > */
  unsigned int portaudio_period_size;/**< The size of the audio buffers (in frames).> */
  // PortMidi options
  GString *portmidi_input_device;
  GString *portmidi_output_device;
  // fluidsynth options
  GString *fluidsynth_soundfont; /**< Default soundfont for fluidsynth */
  gboolean fluidsynth_reverb; /**< Toggle if reverb is applied to fluidsynth */
  gboolean fluidsynth_chorus; /**< Toggle if chorus is applied to fluidsynth */
  gboolean lowpitch; //A440 or A415
  gint dynamic_compression;/**< percent compression of dynamic range desired when listening to MIDI-in */
  gboolean damping;/**< when true notes are re-sounded when left off at a lower velocity depending on their duration */
  GString *temperament; /**< Preferred temperament for tuning to */
} HistoricHarpsichordPrefs;

/**
 * The (singleton) root object for the program
 *
 */
struct HistoricHarpsichordRoot
{
  HistoricHarpsichordPrefs prefs;  /**< Preferences stored on exit and re-loaded on startup */
  gboolean setup; /* Run the synth setup */
  gboolean *verbose; /** Display every message */
}  HistoricHarpsichord; /**< The root object. */

#endif
