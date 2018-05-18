#ifdef _HAVE_FLUIDSYNTH_
/*
 * fluid.c
 * JACK audio and MIDI backends.
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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "audio/fluid.h"
#include "audio/midi.h"
#include "audio/temperament.h"

#include <fluidsynth.h>
#include <glib.h>
#include "core/utils.h"

static fluid_settings_t *settings = NULL;
static fluid_synth_t *synth = NULL;
static int sfont_id = -1;

void reset_synth_channels (void)
{
  // select bank 0 and preset 0 in the soundfont we just loaded on channel 0
  fluid_synth_program_select (synth, 0, sfont_id, 0, 0);
  gint i;
  for (i = 0; i < 16; i++)
    fluid_synth_program_change (synth, i, 0);
  set_tuning ();
}

int
fluidsynth_init (HistoricHarpsichordPrefs * config, unsigned int samplerate)
{
  g_debug ("Starting FLUIDSYNTH");

  settings = new_fluid_settings ();
  if (!settings)
    {
      g_warning ("Failed to create the settings");
      return -1;
    }

  fluid_settings_setnum (settings, "synth.sample-rate", (double) samplerate);

  fluid_settings_setint (settings, "synth.reverb.active", config->fluidsynth_reverb ? 1 : 0);
  fluid_settings_setint (settings, "synth.chorus.active", config->fluidsynth_chorus ? 1 : 0);

  // create the synthesizer
  synth = new_fluid_synth (settings);
  if (!synth)
    {
      g_warning ("Failed to create the settings");
      fluidsynth_shutdown ();
      return -1;
    }

  if(g_file_test(config->fluidsynth_soundfont->str, G_FILE_TEST_EXISTS))
    sfont_id = fluid_synth_sfload (synth, config->fluidsynth_soundfont->str, FALSE);

  if (sfont_id == -1)
    {
      g_print ("Failed to load the user soundfont %s. Now trying the default soundfont.", config->fluidsynth_soundfont->str);
      gchar *default_soundfont = g_build_filename (get_system_data_dir (), "soundfonts", "HarpsichordSoundfont.sf2", NULL);
      if(default_soundfont)
        sfont_id = fluid_synth_sfload (synth, default_soundfont, FALSE);
      g_string_assign (HistoricHarpsichord.prefs.fluidsynth_soundfont, default_soundfont);
      g_free (default_soundfont);
    }
  else
     g_print ("Using soundfont %s.\n", config->fluidsynth_soundfont->str);
  if (sfont_id == -1)
    {
      fluidsynth_shutdown ();
      g_warning ("The Harpsichord Synthesizer will not work!!!!\n\n\n");
      return -1;
    }

 reset_synth_channels ();

  return 0;
}


void
fluidsynth_shutdown ()
{
    g_unlink (g_build_filename (get_user_data_dir (TRUE), PREFS_FILE, NULL));
}


void
fluidsynth_feed_midi (unsigned char *event_data, size_t event_length)
{
  int channel = (event_data[0] & 0x0f);
  int type = (event_data[0] & 0xf0);
//g_print("length of %d message %x\n", event_length, type);
  switch (type)
    {
    case MIDI_NOTE_ON:
      {
        int velocity = ((int) (event_data[2]));
        if (velocity > 0x7F)
          velocity = 0x7F;
        fluid_synth_noteon (synth, channel, event_data[1] - HistoricHarpsichord.prefs.lowpitch, velocity);
      }
      break;
    case MIDI_NOTE_OFF:
      fluid_synth_noteoff (synth, channel, event_data[1] - HistoricHarpsichord.prefs.lowpitch);
      break;
    case SYS_EXCLUSIVE_MESSAGE1:
      
      fluid_synth_sysex (synth, (const char*) event_data + 1, event_length - 1, NULL, 0, NULL, FALSE);
      break;
    }
}


static void
fluid_all_notes_off_channel (gint chan)
{
//      fluid_event_all_sounds_off (fluid_event_t *evt, int channel)!!!!!!!!!!!
//      fluid_event_t evt;
//      fluid_event_all_notes_off (&evt, chan)
  gint i;
  for (i = 0; i < 128; i++)
    fluid_synth_noteoff (synth, chan, i);
}

static void
fluid_all_notes_off (void)
{
  gint chan;
  for (chan = 0; chan < 16; chan++)
    fluid_all_notes_off_channel (chan);
}


void
fluidsynth_all_notes_off ()
{
  // FIXME: this call has the potential to cause an xrun and/or disconnect us from JACK
  //FIXME: this unsets the channel settings for immediate playback (fixed below) and more ...????
  //fluid_synth_system_reset(synth);
  fluid_all_notes_off ();
  // if(HistoricHarpsichord.prefs.pitchspellingchannel)
  //  fluid_synth_program_change(synth, HistoricHarpsichord.prefs.pitchspellingchannel, HistoricHarpsichord.prefs.pitchspellingprogram);
}

void
fluidsynth_render_audio (unsigned int nframes, float *left_channel, float *right_channel)
{
  //printf("\nsynth == %d, nframes == %d, left_channel == %f right_channel == %f\n",synth, nframes, left_channel, right_channel);
  fluid_synth_write_float (synth, nframes, left_channel, 0, 1, right_channel, 0, 1);
}

/**
 * Select the soundfont to use for playback
 */
void
choose_sound_font (GtkWidget * widget, GtkWidget * fluidsynth_soundfont)
{
  GtkWidget *sf;
  GtkFileFilter *filter;

  sf = gtk_file_chooser_dialog_new (_("Choose SoundFont File"), GTK_WINDOW (NULL), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_REJECT, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

  //TODO Should we filter????
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "Soundfont file");
  gtk_file_filter_add_pattern (filter, "*.sf2");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (sf), filter);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (sf), get_system_dir(HISTORICHARPSICHORD_DIR_SOUNDFONTS));
            
  gtk_widget_show_all (sf);
  if (gtk_dialog_run (GTK_DIALOG (sf)) == GTK_RESPONSE_ACCEPT)
    {
      g_string_assign (HistoricHarpsichord.prefs.fluidsynth_soundfont, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sf)));
      /* this will only work for 1 sound font */
      gtk_entry_set_text (GTK_ENTRY (fluidsynth_soundfont), HistoricHarpsichord.prefs.fluidsynth_soundfont->str);

    }
  gtk_widget_destroy (sf);
}

#endif //_HAVE_FLUIDSYNTH_
