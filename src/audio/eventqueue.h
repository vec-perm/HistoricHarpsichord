/*
 * eventqueue.h
 * event queue for audio/MIDI backends.
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

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include "audio/audiointerface.h"
#ifdef _HAVE_JACK_
#include <jack/ringbuffer.h>
#else
#include "audio/ringbuffer.h"
#endif



/**
 * Event queue structure for input/output of MIDI events to/from backends.
 */
typedef struct event_queue_t
{

  /**
   * The queue for immediate event output. Events written to this queue will
   * be played back as soon as possible.
   */
  jack_ringbuffer_t *immediate;
  /**
   * The input queue.
   */
  jack_ringbuffer_t *input;



} event_queue_t;


/**
 * Creates a new event queue.
 *
 * @param playback_queue_size   the maximum number of events in the playback
 *                              queue
 * @param immediate_queue_size  the maximum number of events in the immediate
 *                              playback queue
 * @param input_queue_size      the maximum number of events in the input
 *                              queue
 *
 * @return                      the new event queue
 */
event_queue_t *event_queue_new (size_t playback_queue_size, size_t immediate_queue_size, size_t input_queue_size, size_t mixer_queue_size
#ifdef _HAVE_RUBBERBAND_
, size_t rubberband_queue_size
#endif
);

/**
 * Frees the given queue.
 */
void event_queue_free (event_queue_t * queue);

/**
 * Clears the playback queue.
 */
void event_queue_reset_playback (event_queue_t * queue);
/**
 * Clears the mixer queue.
 */
void event_queue_reset_mixer (event_queue_t * queue);

/**
 * Writes an event to the immmediate playback queue.
 *
 * @param data   the event or extended_event to be written to the queue. The event data will be
 *                copied.
 * @param length  length of the event or extended_event to be written to the queue.
 *
 * @return        TRUE if the event was successfully written to the queue
 */
gboolean event_queue_write_immediate (event_queue_t * queue, guchar * data, guint length);


/**
 * Reads an event from one of the output queues.
 *
 * @param[out] event_buffer   the event data
 * @param[out] event_length   the length of the event in bytes
 * @param[out] event_time     the time of the event (in seconds from the start
 *                            of the score)
 * @param until_time          the playback time up to which events should be
 *                            returned
 *
 * @return                    TRUE if an event was written to the output
 *                            parameters
 */
gboolean event_queue_read_output (event_queue_t * queue, unsigned char *event_buffer, size_t * event_length, double *event_time, double until_time);


/**
 * Writes an event to the input queue.
 *
 * @param event   the event to be written to the queue. The event data will be
 *                copied.
 *
 * @return        TRUE if the event was successfully written to the queue
 */
gboolean event_queue_write_input (event_queue_t * queue, midi_event_t const *event);

/**
 * Reads an event from the input queue.
 *
 * @return  a pointer to a newly allocated structure containing the event data.
 *          The caller is responsible for calling g_free() on this pointer.
 */
midi_event_t *event_queue_read_input (event_queue_t * queue);


#endif // EVENTQUEUE_H
