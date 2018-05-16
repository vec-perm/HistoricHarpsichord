#ifdef _HAVE_PORTAUDIO_
/*
 * portaudiobackend.c
 * PortAudio backend.
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * Copyright (C) 2011  Dominic Sacré
 */
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
#include <glib/gstdio.h>
#include "audio/portaudiobackend.h"
#include "audio/portaudioutil.h"
#ifdef _HAVE_RUBBERBAND_
    #include <rubberband/rubberband-c.h>
#endif
#include "audio/midi.h"
#include "audio/fluid.h"
#include "audio/temperament.h"
#include "audio/audiointerface.h"

#include <portaudio.h>
#include <glib.h>
#include <string.h>
#include "core/utils.h"

static PaStream *stream;
static unsigned long sample_rate;

static unsigned long playback_frame = 0;

static gboolean reset_audio = FALSE;

static gint ready = FALSE;


static double
nframes_to_seconds (unsigned long nframes)
{
  return nframes / (double) sample_rate;
}

static unsigned long
seconds_to_nframes (double seconds)
{
  return (unsigned long) (sample_rate * seconds);
}

#define MAX_MESSAGE_LENGTH (255)        //Allow single sysex blocks, ie 0xF0, length, data...0xF7  where length is one byte.


static int
stream_callback (const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo * time_info, PaStreamCallbackFlags status_flags, void *user_data)
{
  float **buffers = (float **) output_buffer;
  size_t i;
  for (i = 0; i < 2; ++i)
    {
      memset (buffers[i], 0, frames_per_buffer * sizeof (float));
    }
  if (!ready)
    return paContinue;

  if (reset_audio)
    {
      fluidsynth_all_notes_off ();
      reset_synth_channels ();
      reset_audio = FALSE;
      return paContinue;
    }

  unsigned char event_data[MAX_MESSAGE_LENGTH]; //needs to be long enough for variable length messages...
  size_t event_length = MAX_MESSAGE_LENGTH;
  double event_time;
  double until_time = nframes_to_seconds (playback_frame + frames_per_buffer);

  while (read_event_from_queue (AUDIO_BACKEND, event_data, &event_length, &event_time, until_time))
    {//g_print("%x %x %x\n", event_data[0], event_data[1], event_data[2] );
      fluidsynth_feed_midi (event_data, event_length);  //in fluid.c note fluidsynth api ues fluid_synth_xxx these naming conventions are a bit too similar
    }
  fluidsynth_render_audio (frames_per_buffer, buffers[0], buffers[1]);  //in fluid.c calls fluid_synth_write_float()
  return paContinue;
}

static int
actual_portaudio_initialize (HistoricHarpsichordPrefs * config)
{
 sample_rate = config->portaudio_sample_rate;
 preferences_change();
  g_message ("Initializing Fluidsynth");
  if (fluidsynth_init (config, sample_rate))
    {
      g_warning ("Initializing Fluidsynth FAILED!");
      return -1;
    }
  set_tuning ();

  g_message ("Initializing PortAudio backend");
  g_info("PortAudio version: %s", Pa_GetVersionText());

  PaStreamParameters output_parameters;
  PaError err;

  err = Pa_Initialize ();
  if (err != paNoError)
    {
      g_warning ("Initializing PortAudio failed");
      return -1;
    }
 
  output_parameters.device = get_portaudio_device_index (config->portaudio_device->str);

  if (output_parameters.device == paNoDevice)
    {
      output_parameters.device =   get_portaudio_device_index ("default");
       if (output_parameters.device == paNoDevice)
        {
            g_warning("No PortAudio device %s and no default either.", config->portaudio_device->str);
            return -1;
        }
    }

  PaDeviceInfo const *info = Pa_GetDeviceInfo (output_parameters.device);

  if (!info)
    {
      g_warning ("Invalid device '%s'", config->portaudio_device->str);
      return -1;
    }

  char const *api_name = Pa_GetHostApiInfo (info->hostApi)->name;
  g_message ("Opening output device '%s: %s'", api_name, info->name);

  output_parameters.channelCount = 2;
  output_parameters.sampleFormat = paFloat32 | paNonInterleaved;
  output_parameters.suggestedLatency = Pa_GetDeviceInfo (output_parameters.device)->defaultLowOutputLatency;
  output_parameters.hostApiSpecificStreamInfo = NULL;
  err = Pa_OpenStream (&stream, NULL, &output_parameters, config->portaudio_sample_rate, config->portaudio_period_size, paNoFlag /* make this a pref??? paClipOff */ , stream_callback, NULL);
  if (err != paNoError)
    {
      g_warning ("Couldn't open output stream");
      return -1;
    }
  err = Pa_StartStream (stream);
  if (err != paNoError)
    {
      g_warning ("Couldn't start output stream");
      return -1;
    }

  return 0;
}

static int
ready_now ()
{
  ready = TRUE;
  return FALSE;
}

static int
portaudio_initialize (HistoricHarpsichordPrefs * config)
{
  g_idle_add ((GSourceFunc) ready_now, NULL);
  return actual_portaudio_initialize (config);
}

static int
portaudio_destroy ()
{
  g_message ("Destroying PortAudio backend");
  ready = FALSE;
  PaError err;

  err = Pa_CloseStream (stream);
  if (err != paNoError)
    {
      g_warning ("Closing stream failed: %d, %s", err, Pa_GetErrorText (err));
      return -1;
    }

  Pa_Terminate ();

#ifdef _HAVE_FLUIDSYNTH_
  fluidsynth_shutdown ();
#endif

  return 0;
}


static int
portaudio_reconfigure (HistoricHarpsichordPrefs * config)
{
  portaudio_destroy ();
  return portaudio_initialize (config);
}


static int
portaudio_start_playing ()
{
  return 0;
}


static int
portaudio_stop_playing ()
{
  return 0;
}


static int
portaudio_panic ()
{
  reset_audio = TRUE;
  return 0;
}


backend_t portaudio_backend = {
  portaudio_initialize,
  portaudio_destroy,
  portaudio_reconfigure,
  portaudio_start_playing,
  portaudio_stop_playing,
  portaudio_panic,
};
#endif //_HAVE_PORTAUDIO_
