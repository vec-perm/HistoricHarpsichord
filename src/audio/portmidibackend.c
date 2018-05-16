#ifdef _HAVE_PORTMIDI_
/*
 * portmidibackend.c
 * PortMidi backend.
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * Copyright (C) 2011  Dominic Sacré
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "audio/portmidibackend.h"
#include "audio/portmidiutil.h"
#include "audio/midi.h"
#include <portmidi.h>
#include <porttime.h>
#include <glib.h>


#define TIMER_RESOLUTION 5
#define INPUT_BUFFER_SIZE 0
#define OUTPUT_BUFFER_SIZE 256

static PmStream *input_stream = NULL;
static PmStream *output_stream = NULL;

static gboolean initialized = FALSE;

static gboolean reset = FALSE;

static double playback_start_time;


static int portmidi_destroy ();


static void
process_midi (PtTimestamp timestamp, void *user_data)
{
  if (!g_atomic_int_get (&initialized))
    {
      return;
    }

  if (!input_stream)
    {
      return;
    }

  PmEvent event;
  int r;
  while (Pm_Poll (input_stream) == TRUE)
    {

      r = Pm_Read (input_stream, &event, 1);

      // we should only ever get one event.
      // if we get a sysex, just skip it.
      if (r != 1 || (Pm_MessageStatus (event.message) & 0xf0) == 0xf0)
        {
          continue;
        }

      unsigned char buffer[3] = {
        Pm_MessageStatus (event.message),
        Pm_MessageData1 (event.message),
        Pm_MessageData2 (event.message)
      };

      input_midi_event (MIDI_BACKEND, 0, buffer);
    }
}


static int
error_shutdown (void)
{
    gchar *title, *message;
    title =_("Harpsichord Synthesizer Error");
    message = g_strdup_printf ("<span font_desc=\"24\" foreground=\"red\">%s\n%s</span>", _("No MIDI keyboard found"), "<span font_desc=\"18\" foreground=\"blue\">Did you forget to switch it on?\nOr if it's a new one\nRun the Setup program</span>");
 
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), title);
    GtkWidget *button = gtk_button_new_with_label ("");
    GtkWidget *label = gtk_bin_get_child (GTK_BIN(button));
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_label_set_markup(GTK_LABEL (label), message);
    gtk_container_add (GTK_CONTAINER (window), button);
    g_signal_connect (button, "clicked", G_CALLBACK (exit), NULL);
    g_signal_connect (button, "destroy", G_CALLBACK (exit), NULL);
    gtk_widget_show_all (window);
    return -1;
}
static int
portmidi_initialize (HistoricHarpsichordPrefs * config)
{
  g_message ("Initializing PortMidi backend");

  PtError pterr = Pt_Start (TIMER_RESOLUTION, &process_midi, NULL);
  if (pterr != ptNoError)
    {
      g_warning ("Couldn't start timer");
      return error_shutdown ();
    }

  PmError err;
  int id;
  PmDeviceInfo const *info;

  err = Pm_InitializeWrapper ();
  if (err != pmNoError)
    {
      g_warning ("Couldn't initialize PortMidi");
      portmidi_destroy ();
      return error_shutdown ();
    }
 

  
  
  if (g_strcmp0 (config->portmidi_input_device->str, "none") != 0)
    {
      id = get_portmidi_device_id (config->portmidi_input_device->str, FALSE);

      info = Pm_GetDeviceInfo (id);
      if (info == NULL)
        {
          id = get_portmidi_device_id ("default", FALSE);
          info = Pm_GetDeviceInfo (id);
           if (info == NULL)
                {
                g_warning ("No MIDI input device, and no default device");
                portmidi_destroy ();
                return error_shutdown ();
                }
        }

      g_message ("Opening input device '%s: %s'", info->interf, info->name);

      err = Pm_OpenInput (&input_stream, id, NULL, INPUT_BUFFER_SIZE, NULL, NULL);
      if (err != pmNoError)
        {
          g_warning ("Couldn't open input stream");
          portmidi_destroy ();
          return error_shutdown ();
        }
    }
  else
    {
      g_message ("Input device is disabled");
      input_stream = NULL;
    }
      output_stream = NULL;

  g_atomic_int_set (&initialized, TRUE);

  return input_stream == NULL?-1:0;
}


static int
portmidi_destroy ()
{
  g_message ("Destroying PortMidi backend");

  Pt_Stop ();

  g_atomic_int_set (&initialized, FALSE);

  if (input_stream)
    {
      Pm_Close (input_stream);
    }
  if (output_stream)
    {
      Pm_Close (output_stream);
    }

  Pm_TerminateWrapper ();

  return 0;
}


static int
portmidi_reconfigure (HistoricHarpsichordPrefs * config)
{
  portmidi_destroy ();
  return portmidi_initialize (config);
}


static int
portmidi_start_playing ()
{
  return 0;
}


static int
portmidi_stop_playing ()
{
  return 0;
}


static int
portmidi_panic ()
{
  reset = TRUE;
  return 0;
}


backend_t portmidi_backend = {
  portmidi_initialize,
  portmidi_destroy,
  portmidi_reconfigure,
  portmidi_start_playing,
  portmidi_stop_playing,
  portmidi_panic,
};

#endif //_HAVE_PORTMIDI_
