/*
 * eventqueue.c
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

#include "audio/eventqueue.h"
#include "audio/midi.h"

#include <glib.h>
#include <string.h>


event_queue_t *
event_queue_new (size_t playback_queue_size, size_t immediate_queue_size, size_t input_queue_size, size_t mixer_queue_size)
{
  event_queue_t *queue = g_malloc0 (sizeof (event_queue_t));

  if (immediate_queue_size)
    {
      queue->immediate = jack_ringbuffer_create (immediate_queue_size * sizeof (midi_event_t));
      jack_ringbuffer_reset (queue->immediate);
    }

  if (input_queue_size)
    {
      queue->input = jack_ringbuffer_create (input_queue_size * sizeof (midi_event_t));
      jack_ringbuffer_reset (queue->input);
    }


  return queue;
}


void
event_queue_free (event_queue_t * queue)
{
 

  if (queue->immediate)
    {
      jack_ringbuffer_free (queue->immediate);
    }

  if (queue->input)
    {
      jack_ringbuffer_free (queue->input);
    }

  g_free (queue);
}



gboolean
event_queue_write_immediate (event_queue_t * queue, guchar * data, guint length)
{
  if (!queue->immediate || jack_ringbuffer_write_space (queue->immediate) < length)
    {
      return FALSE;
    }
  size_t n = jack_ringbuffer_write (queue->immediate, (char const *) data, length);

  return n == length;

}


gboolean
event_queue_read_output (event_queue_t * queue, unsigned char *event_buffer, size_t * event_length, double *event_time, double until_time)
{
  if (jack_ringbuffer_read_space (queue->immediate))
    {
      jack_ringbuffer_data_t vec[2];
      jack_ringbuffer_get_read_vector (queue->immediate, vec);
      if (vec[0].len)
        {
          guchar length;
          jack_ringbuffer_read (queue->immediate, (char*) &length, 1);
          //g_assert (length < 255);
          jack_ringbuffer_read (queue->immediate, (char*) event_buffer, length);
          *event_length = length;
          *event_time = 0.0;
          return TRUE;
        }
    }
 
}


gboolean
event_queue_write_input (event_queue_t * queue, midi_event_t const *event)
{
  if (!queue->input || jack_ringbuffer_write_space (queue->input) < sizeof (midi_event_t))
    {
      return FALSE;
    }

  size_t n = jack_ringbuffer_write (queue->input, (char *) event, sizeof (midi_event_t));

  return n == sizeof (midi_event_t);
}


midi_event_t *
event_queue_read_input (event_queue_t * queue)
{
  if (!queue->input)
    {
      return NULL;
    }

  if (jack_ringbuffer_read_space (queue->input))
    {
      midi_event_t *ev = g_malloc (sizeof (midi_event_t));
      jack_ringbuffer_read (queue->input, (char *) ev, sizeof (midi_event_t));
      return ev;
    }
  else
    {
      return NULL;
    }
}
