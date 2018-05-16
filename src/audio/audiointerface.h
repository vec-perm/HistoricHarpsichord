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

#ifndef AUDIOINTERFACE_H
#define AUDIOINTERFACE_H

#include <historicHarpsichord/historicHarpsichord_types.h>


/**
 * The common interface for all audio and MIDI backends.
 */
typedef struct backend_t
{
  /**
   * Initializes the backend with the given configuration.
   *
   * FIXME: the HistoricHarpsichordPrefs argument is both too general (backend-specific
   * settings would be enough) and redundant (there's a global instance
   * anyway).
   *
   * @param config  the configuration object containing the backend's settings
   *
   * @return        zero on success, a negative error code on failure
   */
  int (*initialize) (HistoricHarpsichordPrefs * config);

  /**
   * Destroys the backend and cleans up all its resources.
   *
   * @return        zero on success, a negative error code on failure
   */
  int (*destroy) ();

  /**
   * Changes the backend's configuration, if possible without destroying and
   * recreating the backend.
   *
   * This is not used by any backends at the moment, but will be needed, for
   * example, to configure the audio input settings.
   *
   * @return        zero on success, a negative error code on failure
   */
  int (*reconfigure) (HistoricHarpsichordPrefs * config);

  /**
   * Called when playback is started.
   *
   * @return        zero on success, a negative error code on failure
   */
  int (*start_playing) ();

  /**
   * Called when playback is stopped.
   *
   * @return        zero on success, a negative error code on failure
   */
  int (*stop_playing) ();

  /**
   * Sends a MIDI panic CC and/or resets the synth engine.
   *
   * @return        zero on success, a negative error code on failure
   */
  int (*panic) ();

} backend_t;


/**
 * An enum that identifies a backend's type.
 */
typedef enum backend_type_t
{
  /**
   * The default backend (either audio or MIDI)
   */
  DEFAULT_BACKEND = -1,
  /**
   * The audio backend
   */
  AUDIO_BACKEND = 0,
  /**
   * The MIDI backend
   */
  MIDI_BACKEND = 1
} backend_type_t;


// we only have two backends at the same time: audio and MIDI
#define NUM_BACKENDS 2


/**
 * An enum that specifies a backend's priority regarding the playback time.
 */
typedef enum backend_timebase_prio_t
{
  /**
   * The backend synchronizes to an external sample clock
   */
  TIMEBASE_PRIO_AUDIO = 3,
  /**
   * The backend is not bound to any external clock
   */
  TIMEBASE_PRIO_MIDI = 2,
  /**
   * The backend requires no timing information
   */
  TIMEBASE_PRIO_DUMMY = 1
} backend_timebase_prio_t;


typedef struct midi_event_t
{
  backend_type_t backend;
  int port;
  int length;
  unsigned char data[3];
} midi_event_t;


/**
 * Initializes the audio/MIDI subsystem.
 *
 * @return        zero on success, a negative error code on failure
 */
int audio_initialize (HistoricHarpsichordPrefs * config);

/**
 * Destroys and cleans up the audio/MIDI subsystem.
 *
 * @return        zero on success, a negative error code on failure
 */
int audio_shutdown ();

/**
 * Plays a single MIDI event.
 *
 * @param backend   the type of backend to be used
 * @param port      the number of the backend's output port to be used. ignored
 *                  by most backends.
 * @param buffer    the MIDI data to be sent
 */
int play_midi_event (backend_type_t backend, int port, unsigned char *buffer);

int panic (backend_type_t backend);

/**
 * Called by a backend to read midi events queued for playback.
 *
 * @param backend             the type of backend
 * @param[out] event_buffer   the event data
 * @param[out] event_length   the length of the event in bytes
 * @param[out] event_time     the time of the event (in seconds from the start
 *                            of the score)
 * @param until_time          the playback time up to which events should be
 *                            returned
 *
 * @return                    TRUE if an event was written to the output
 *                            parameters, FALSE if there is no event to be
 *                            played
 */
gboolean read_event_from_queue (backend_type_t backend, unsigned char *event_buffer, size_t * event_length, double *event_time, double until_time);

/**
 * Called by a backend when an incoming MIDI event was received.
 *
 * @param backend   the backend that received the event
 * @param port      the port that received the event (zero if there is only
 *                  one port)
 * @param buffer    the MIDI event data
 */
void input_midi_event (backend_type_t backend, int port, unsigned char *buffer);

#endif // AUDIOINTERFACE_H
