/*
 * audiointerface.h
 * Interface definition for audio and MIDI backends.
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
#include "audio/audiointerface.h"
#include "audio/eventqueue.h"
#include "audio/dummybackend.h"

#ifdef _HAVE_PORTAUDIO_
#include "audio/portaudiobackend.h"
#endif
#ifdef _HAVE_PORTMIDI_
#include "audio/portmidibackend.h"
#endif
#ifdef _HAVE_ALSA_
#include "audio/alsabackend.h"
#endif

#include "audio/midi.h"
#include "audio/temperament.h"

#include <glib.h>
#include <string.h>
#include <stdint.h>

static backend_t *backends[NUM_BACKENDS] = { NULL };

#define PLAYBACK_QUEUE_SIZE 0
#define IMMEDIATE_QUEUE_SIZE 32
#define INPUT_QUEUE_SIZE 256
#define MIXER_QUEUE_SIZE 0
#define RUBBERBAND_QUEUE_SIZE 50000


// the time in µs after which the queue thread wakes up, whether it has been
// signalled or not
#define QUEUE_TIMEOUT 100000

static event_queue_t *event_queues[NUM_BACKENDS] = { NULL };


static GThread *queue_thread;
static GCond queue_cond;
static GMutex queue_mutex;


static volatile double playback_time;

static gboolean quit_thread;
static gboolean signalled = FALSE;


#ifndef _HAVE_RUBBERBAND_
gdouble get_playback_speed (void)
{
    return 1.0; //Rubberband can do slowdown, backend should define its own version of this
}
#endif

static gpointer queue_thread_func (gpointer data);
static void signal_queue ();



static backend_t *
get_backend (backend_type_t backend)
{
  if (backend == DEFAULT_BACKEND)
    {
      // FIXME: this should be configurable
      //return backends[MIDI_BACKEND];
      return backends[AUDIO_BACKEND];
    }
  else
    {
      return backends[backend];
    }
}

static event_queue_t *
get_event_queue (backend_type_t backend)
{
  if (backend == DEFAULT_BACKEND)
    {
      // FIXME
      // return event_queues[MIDI_BACKEND];
      return event_queues[AUDIO_BACKEND];
    }
  else
    {
      return event_queues[backend];
    }
}

static int
initialize_audio (HistoricHarpsichordPrefs * config)
{
  char const *driver = config->audio_driver->str;

  g_message ("Audio driver is '%s'", driver);

  if (strcmp (driver, "portaudio") == 0)
    {
#ifdef _HAVE_PORTAUDIO_
      backends[AUDIO_BACKEND] = &portaudio_backend;
#else
      g_warning ("PortAudio backend is not enabled");
#endif
    }
  else if (strcmp (driver, "dummy") == 0)
    {
      // do nothing
    }
  else
    {
      g_warning ("Unknown audio backend '%s'", driver);
    }

  if (backends[AUDIO_BACKEND] == NULL)
    {
      backends[AUDIO_BACKEND] = &dummy_audio_backend;
    }

  //event_queues[AUDIO_BACKEND] = event_queue_new(PLAYBACK_QUEUE_SIZE, IMMEDIATE_QUEUE_SIZE, 0);

  int ret = get_backend (AUDIO_BACKEND)->initialize (config);

  if (ret)
    {
      g_warning ("Initializing audio backend '%s' failed, falling back to dummy", driver);
      backends[AUDIO_BACKEND] = &dummy_audio_backend;
      ret = get_backend (AUDIO_BACKEND)->initialize (config);
      ret = -1;
    }

  return ret;
}


static int
initialize_midi (HistoricHarpsichordPrefs * config)
{
  char const *driver = config->midi_driver->str;

  g_message ("MIDI driver is '%s'", driver);

 if (strcmp (driver, "portmidi") == 0)
    {
#ifdef _HAVE_PORTMIDI_
      backends[MIDI_BACKEND] = &portmidi_backend;
#else
      g_warning ("PortMidi backend is not enabled");
#endif
    }
  else if (strcmp (driver, "alsa") == 0)
    {
#ifdef _HAVE_ALSA_
      backends[MIDI_BACKEND] = &alsa_seq_midi_backend;
#else
      g_warning ("ALSA backend is not enabled");
#endif
    }
  else if (strcmp (driver, "dummy") == 0)
    {
      // do nothing
    }
  else
    {
      g_warning ("Unknown MIDI backend '%s'", driver);
    }

  if (backends[MIDI_BACKEND] == NULL)
    {
      backends[MIDI_BACKEND] = &dummy_midi_backend;
    }

  //event_queues[MIDI_BACKEND] = event_queue_new(PLAYBACK_QUEUE_SIZE, IMMEDIATE_QUEUE_SIZE, INPUT_QUEUE_SIZE);

  int ret = get_backend (MIDI_BACKEND)->initialize (config);

  if (ret)
    {
      g_warning ("Initializing MIDI backend '%s' failed, falling back to dummy", driver);
      backends[MIDI_BACKEND] = &dummy_midi_backend;
      ret = get_backend (MIDI_BACKEND)->initialize (config);
      ret = -1;//dummy is of no use
    }

  return ret;
}

int
audio_initialize (HistoricHarpsichordPrefs * config)
{
  queue_thread = NULL;
  quit_thread = FALSE;
 

  //&queue_cond = g_cond_new (); since GLib 2.32 no longer needed, static declaration is enough
  //&queue_mutex = g_mutex_new (); since GLib 2.32 no longer needed, static declaration is enough
  event_queues[AUDIO_BACKEND] = event_queue_new (PLAYBACK_QUEUE_SIZE, IMMEDIATE_QUEUE_SIZE, 0, MIXER_QUEUE_SIZE
#ifdef _HAVE_RUBBERBAND_
, RUBBERBAND_QUEUE_SIZE
#endif
);
  event_queues[MIDI_BACKEND] = event_queue_new (PLAYBACK_QUEUE_SIZE, IMMEDIATE_QUEUE_SIZE, INPUT_QUEUE_SIZE, 0
#ifdef _HAVE_RUBBERBAND_
, 0
#endif
);
  if (initialize_audio (config) || initialize_midi(config))
    {
      audio_shutdown ();
      return -1;
    }

  queue_thread = g_thread_try_new ("Queue Thread", queue_thread_func, NULL, NULL);

  if (queue_thread == NULL)
    {
      audio_shutdown();
      return -1;
    }

  return 0;
}

static int
destroy (backend_type_t backend)
{
  get_backend (backend)->destroy ();
  backends[backend] = NULL;

  event_queue_free (event_queues[backend]);

  return 0;
}


int
audio_shutdown ()
{
  g_atomic_int_set (&quit_thread, TRUE);

  if (queue_thread)
    {
      signal_queue ();

      g_thread_join (queue_thread);
    }

  if (get_backend (AUDIO_BACKEND))
    {
      destroy (AUDIO_BACKEND);
    }

  if (get_backend (MIDI_BACKEND))
    {
      destroy (MIDI_BACKEND);
    }

 //g_cond_free (&queue_cond); since GLib 2.32 no longer needed, static declaration is enough
 //g_mutex_free (&queue_mutex); since GLib 2.32 no longer needed, static declaration is enough

  return 0;
}

static gboolean do_handle_midi_event (gchar *data) {
  handle_midi_event (data);
  g_free(data);
  return FALSE;
}
static gboolean
handle_midi_event_callback (gpointer data)
{

  midi_event_t *ev = (midi_event_t *) data;

  // TODO: handle backend type and port
  gchar *evdata = g_malloc (sizeof (ev->data));
  memcpy (evdata, ev->data, sizeof (ev->data));
  g_main_context_invoke (NULL, (GSourceFunc)do_handle_midi_event, evdata);

  g_free (ev);

  return FALSE;
}



gboolean
read_event_from_queue (backend_type_t backend, unsigned char *event_buffer, size_t * event_length, double *event_time, double until_time)
{
 

  return event_queue_read_output (get_event_queue (backend), event_buffer, event_length, event_time, until_time);
}


GMutex smfmutex;// = G_STATIC_MUTEX_INIT;
static gpointer
queue_thread_func (gpointer data)
{
  g_mutex_lock (&queue_mutex);

  for (;;)
    {
      if (!g_atomic_int_get (&signalled))
        {
          gint64 end_time = g_get_monotonic_time () +  (QUEUE_TIMEOUT * G_TIME_SPAN_SECOND)/1000000;
          g_cond_wait_until (&queue_cond, &queue_mutex, end_time);
          signalled = FALSE;
        }

      if (g_atomic_int_get (&quit_thread))
        {
          g_message ("That's it, I quit!");
          break;
        }


      // TODO: audio capture

      midi_event_t *ev;

      while ((ev = event_queue_read_input (get_event_queue (MIDI_BACKEND))) != NULL)
        {
          g_idle_add_full (G_PRIORITY_HIGH_IDLE, handle_midi_event_callback, (gpointer) ev, NULL);
        }

    }

  g_mutex_unlock (&queue_mutex);

  return NULL;
}


static void
signal_queue ()
{
  g_mutex_lock (&queue_mutex);
  g_atomic_int_set (&signalled, TRUE);
  g_cond_signal (&queue_cond);
  g_mutex_unlock (&queue_mutex);
}


static gboolean
try_signal_queue ()
{
  if (g_mutex_trylock (&queue_mutex))
    {
      g_atomic_int_set (&signalled, TRUE);
      g_cond_signal (&queue_cond);
      g_mutex_unlock (&queue_mutex);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}
static gboolean time_reset = FALSE;

void
update_playback_time (backend_timebase_prio_t prio, double new_time)
{
  if (!((prio == TIMEBASE_PRIO_AUDIO) || (prio == TIMEBASE_PRIO_MIDI && get_backend (AUDIO_BACKEND) == &dummy_audio_backend) || (get_backend (AUDIO_BACKEND) == &dummy_audio_backend && get_backend (MIDI_BACKEND) == &dummy_midi_backend)))
    {
      // ignore new playback time if another backend has higher priority
      return;
    }
  if(time_reset)
    {
        time_reset = FALSE;
        return;
    }
  if (new_time != playback_time)
    {
      playback_time = new_time;
      // midi_play tries to set playback_time, which then gets overriden by the call in the portaudio callback.
      // if the lock fails, the playback time update will be delayed until the
      // queue thread wakes up on its own
      if (!try_signal_queue ())
        {
          ;//this is continuously emitted by windows which has debug on. g_debug ("Couldn't signal playback time update to queue");
        }
    }
}

double
get_playback_time (void)
{
  return playback_time;
}


void
midi_play (gchar * callback)
{
 
}

void
audio_play (void)
{
 

}

void
midi_stop (void)
{
  g_message ("Stopping playback");

}


#define MIDI_EOX (0xF7)

int
play_midi_event (backend_type_t backend, int port, unsigned char *buffer)
{
  guchar ev[1 + 255];           /* 1 length byte plus up to 255 data bytes */
  gint i = 3;
  if (buffer[0] == SYS_EXCLUSIVE_MESSAGE1)
    {
      for (i = 0; i < 255; i++)
        if (buffer[i] == MIDI_EOX)
          break;
      if (i == 255)
        return FALSE;
    }
  *ev = i;
  memcpy (ev + 1, buffer, i);//g_print (" %d midibytes 0x%hhX 0x%hhX 0x%hhX\n", i,  *(buffer+0), *(buffer+1), *(buffer+2));

  return event_queue_write_immediate (get_event_queue (backend), ev, i + 1);
}



int
panic (backend_type_t backend)
{
 // g_critical ("Panicking");
  return get_backend (backend)->panic ();
}


void
input_midi_event (backend_type_t backend, int port, unsigned char *buffer)
{
  midi_event_t ev;
  ev.backend = backend;
  ev.port = port;
  // FIXME: size might be less than 3
  memcpy (&ev.data, buffer, 3);

  // normalize events: replace note-on with zero velocity by note-off
  if ((ev.data[0] & 0xf0) == MIDI_NOTE_ON && ev.data[2] == 0)
    {
      ev.data[0] = (ev.data[0] & 0x0f) | MIDI_NOTE_OFF;
    }

  event_queue_write_input (get_event_queue (backend), &ev);

 
}


