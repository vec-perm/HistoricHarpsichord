/* prefdialog.c
 * functions for a preferences dialog
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * (c) 1999-2005 Matthew Hiller, Adam Tee
 * (c) 2011 Richard Shann, Dominic Sacré
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <historicHarpsichord/historicHarpsichord.h>
#include "core/prefops.h"
#include "core/utils.h"
#include "audio/fluid.h"
#include "audio/audiointerface.h"
#include "audio/temperament.h"
#include "audio/portaudioutil.h"
#include "audio/portmidiutil.h"



#if GTK_MAJOR_VERSION==2
#define gtk_combo_box_text_new_with_entry gtk_combo_box_new_text
#define gtk_combo_box_text_append_text gtk_combo_box_append_text
#define gtk_combo_box_text_get_active_text gtk_combo_box_get_active_text
#define GTK_COMBO_BOX_TEXT GTK_COMBO_BOX
#endif

struct callbackdata
{
  HistoricHarpsichordPrefs *prefs;

  GtkWidget *dynamic_compression;
  GtkWidget *damping;
  GtkWidget *lowpitch;
  GtkWidget *audio_driver;
  GtkWidget *midi_driver;

  GtkWidget *portaudio_device;
  GtkWidget *portaudio_sample_rate;
  GtkWidget *portaudio_period_size;
  GtkWidget *maxrecordingtime;

  GtkWidget *portmidi_input_device;
  GtkWidget *portmidi_output_device;

  GtkWidget *fluidsynth_soundfont;
  GtkWidget *fluidsynth_reverb;
  GtkWidget *fluidsynth_chorus;


  GtkWidget *temperament;

  GList *audio_backend_list;
  GList *audio_driver_option_list;
  GList *midi_backend_list;
  GList *midi_driver_option_list;
};

struct audio_callback_data
{
  GtkWidget *dialog;
  GtkWidget *audio_driver;
  GtkWidget *midi_driver;
  GtkWidget *portaudio_settings;
  GtkWidget *portmidi_settings;

};

static void
free_g_lists (struct callbackdata *cbdata)
{
  g_list_free (cbdata->audio_backend_list);
  g_list_free (cbdata->audio_driver_option_list);
  g_list_free (cbdata->midi_backend_list);
  g_list_free (cbdata->midi_driver_option_list);

  cbdata->audio_backend_list = NULL;
  cbdata->audio_driver_option_list = NULL;
  cbdata->midi_backend_list = NULL;
  cbdata->midi_driver_option_list = NULL;
}

static void
set_preferences (struct callbackdata *cbdata)
{
  HistoricHarpsichordPrefs *prefs = cbdata->prefs;
  gboolean midi_in_device_was_default = !strcmp (prefs->portmidi_input_device->str, "default");
#define ASSIGNTEXT(field) \
  g_string_assign (prefs->field,\
    gtk_entry_get_text (GTK_ENTRY (cbdata->field)));
    
#define ASSIGNBOOLEAN(field) \
  prefs->field =\
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(cbdata->field));

#define ASSIGNINT(field) \
   prefs->field =\
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(cbdata->field));

#define ASSIGNDOUBLE(field) \
   prefs->field =\
     gtk_spin_button_get_value (GTK_SPIN_BUTTON(cbdata->field));

#define ASSIGNCOMBO(field) \
  if (cbdata->field)\
   g_string_assign (prefs->field,\
    (gchar *) gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(cbdata->field)));

    g_string_assign (prefs->audio_driver, "portaudio");

    g_string_assign (prefs->midi_driver, "portmidi");
    ASSIGNCOMBO (portaudio_device);
    ASSIGNCOMBO (portmidi_input_device);
  
//  ASSIGNTEXT (temperament) why is this not needed?
 
    ASSIGNBOOLEAN (damping);
    ASSIGNBOOLEAN (lowpitch);
    ASSIGNINT (dynamic_compression);
  
  /* Now write it all to historicHarpsichordrc */
   if (HistoricHarpsichord.setup) // only save prefs as result of setup
      writeXMLPrefs (prefs);
}

static void
midi_audio_tab_update (GtkWidget * box, gpointer data)
{
  struct audio_callback_data *cbdata = (struct audio_callback_data *) data;
  gtk_widget_set_visible (cbdata->portaudio_settings, 1);
  gtk_widget_set_visible (cbdata->portmidi_settings, 1);
  gtk_window_resize (GTK_WINDOW (cbdata->dialog), 1, 1);
}

//callback for Prefences command
void
preferences_change (void)
{
 
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *separator;
  GtkWidget *main_vbox;

 
  GtkWidget *notebook;
  GtkWidget *hbox;

#ifdef _HAVE_PORTAUDIO_
  GtkWidget *portaudio_settings;
#endif
#ifdef _HAVE_PORTMIDI_
  GtkWidget *portmidi_settings;
#endif
  GList *g;
  gint i;
  static struct callbackdata cbdata;
 

  cbdata.audio_backend_list = NULL;
  cbdata.audio_driver_option_list = NULL;
  cbdata.midi_backend_list = NULL;
  cbdata.midi_driver_option_list = NULL;

  // these lists need to be initialized the first time this function is called
  if (!cbdata.audio_backend_list)
    {
      cbdata.audio_backend_list = g_list_append (cbdata.audio_backend_list, (gpointer) "portaudio");
      cbdata.audio_driver_option_list = g_list_append (cbdata.audio_driver_option_list, (gpointer) "PortAudio");

      cbdata.midi_backend_list = g_list_append (cbdata.midi_backend_list, (gpointer) "portmidi");
      cbdata.midi_driver_option_list = g_list_append (cbdata.midi_driver_option_list, (gpointer) "PortMidi");
    }


  dialog = gtk_dialog_new_with_buttons (_("Synthesizer Set Up"), GTK_WINDOW (NULL), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

  //gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget *warning_message = gtk_label_new ("");
   gtk_label_set_use_markup (GTK_LABEL (warning_message), TRUE);
  gtk_label_set_markup (GTK_LABEL (warning_message), _("<span foreground=\"red\"weight=\"bold\">Make sure your MIDI keyboard is selected! Then press OK.</span>"));
  gtk_container_add (GTK_CONTAINER (content_area), warning_message);
  notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (content_area), notebook);
#define VBOX main_vbox

#define NEWPAGE(thelabel) \
    main_vbox = gtk_vbox_new (FALSE, 1);\
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), main_vbox, NULL);\
    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (notebook), main_vbox, thelabel);

#define BOOLEANENTRY(thelabel, field) \
  GtkWidget *field =\
    gtk_check_button_new_with_label (thelabel); \
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (field),\
                                (gboolean)HistoricHarpsichord.prefs.field);\
  gtk_box_pack_start (GTK_BOX (VBOX), field, FALSE, TRUE, 0);\
  cbdata.field = field;

#define TEXTENTRY(thelabel, field) \
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);\
  label = gtk_label_new (thelabel);\
  gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);\
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);\
  GtkWidget *field = gtk_entry_new ();\
  gtk_entry_set_text (GTK_ENTRY (field), HistoricHarpsichord.prefs.field->str);\
  gtk_box_pack_start (GTK_BOX (hbox), field, TRUE, TRUE, 0);\
  cbdata.field = field;

#define PASSWORDENTRY(thelabel, field) \
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);\
  label = gtk_label_new (thelabel);\
  gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);\
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);\
  GtkWidget *field = gtk_entry_new ();\
  gtk_entry_set_visibility(GTK_ENTRY(field), FALSE);\
  gtk_entry_set_invisible_char(GTK_ENTRY(field), '*');\
  gtk_entry_set_text (GTK_ENTRY (field), HistoricHarpsichord.prefs.field->str);\
  gtk_box_pack_start (GTK_BOX (hbox), field, TRUE, TRUE, 0);\
  cbdata.field = field;

#define INTENTRY(thelabel, field) \
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);\
  label = gtk_label_new (thelabel);\
  gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);\
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);\
  field = gtk_spin_button_new_with_range (1, 50, 1.0);\
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (field), HistoricHarpsichord.prefs.field);\
  gtk_box_pack_start (GTK_BOX (hbox), field, FALSE, FALSE, 0);\
  cbdata.field = field;

#define ENTRY_LIMITS(thelabel, field, min, max, step)   \
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);\
  label = gtk_label_new (thelabel);\
  gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);\
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);\
  GtkWidget *field = gtk_spin_button_new_with_range (min, max, step);\
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (field), HistoricHarpsichord.prefs.field);\
  gtk_box_pack_start (GTK_BOX (hbox), field, FALSE, FALSE, 0);\
  cbdata.field = field;

#define INTENTRY_LIMITS(thelabel, field, min, max) ENTRY_LIMITS(thelabel, field, min, max, 1)

#define DOUBLEENTRY_LIMITS  ENTRY_LIMITS

#define BUTTON(thelabel, field, thecallback, data) \
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);\
  GtkWidget *field = gtk_button_new_with_label(thelabel);\
  gtk_box_pack_start (GTK_BOX (vbox1), field, FALSE, FALSE, 0);\
  g_signal_connect (G_OBJECT (field), "clicked",\
  G_CALLBACK (thecallback), (gpointer) data);

#define CBOX(thelabel, field, thelist, settext)\
 GtkWidget *field = gtk_combo_box_text_new_with_entry ();\
 i=0;\
 for (g=thelist;g;g=g->next){\
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(field), g->data);\
  if (0==strcmp(g->data, settext))\
    gtk_combo_box_set_active(GTK_COMBO_BOX(field), i);\
  i++;\
 }

#define COMBOBOX(thelabel, field, thelist, settext, editable)\
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);\
  label = gtk_label_new (thelabel);\
  gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);\
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);\
  hbox = gtk_hbox_new (FALSE, 8);\
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, TRUE, TRUE, 0);\
  CBOX(thelabel, field, thelist, settext)\
  gtk_box_pack_start (GTK_BOX (hbox), field, TRUE, TRUE, 0);\
  gtk_widget_show (field);\
  cbdata.field = field;

#if GTK_MAJOR_VERSION == 2
#define SEPARATOR()\
  separator = gtk_hseparator_new();\
  gtk_box_pack_start (GTK_BOX (VBOX), separator, FALSE, TRUE, 4);
#else
#define SEPARATOR()\
  separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);\
  gtk_box_pack_start (GTK_BOX (VBOX), separator, FALSE, TRUE, 4);
#endif
 

  static struct audio_callback_data audio_cbdata;

  NEWPAGE (_("Harpsichord Synthesizer"));
  GList *item = g_list_find_custom (cbdata.midi_backend_list, HistoricHarpsichord.prefs.midi_driver->str, (GCompareFunc) strcmp);
  gint index = g_list_position (cbdata.midi_backend_list, item);
  if (index < 0)
    index = 0;
  gchar *driver = g_list_nth_data (cbdata.midi_driver_option_list, index);


#ifdef _HAVE_PORTMIDI_

#undef VBOX
#define VBOX portmidi_settings
  portmidi_settings = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), portmidi_settings, FALSE, TRUE, 0);

  GList *input_devices = get_portmidi_devices (FALSE);
  GList *output_devices = get_portmidi_devices (TRUE);

  COMBOBOX (_("MIDI keyboard: "), portmidi_input_device, input_devices, HistoricHarpsichord.prefs.portmidi_input_device->str, FALSE);

  free_portmidi_devices (input_devices);
  free_portmidi_devices (output_devices);

#undef VBOX
#define VBOX main_vbox

#endif

  item = g_list_find_custom (cbdata.audio_backend_list, HistoricHarpsichord.prefs.audio_driver->str, (GCompareFunc) strcmp);
  index = g_list_position (cbdata.audio_backend_list, item);
  if (index < 0)
    index = 0;
  driver = g_list_nth_data (cbdata.audio_driver_option_list, index);

#ifdef _HAVE_PORTAUDIO_

#undef VBOX
#define VBOX portaudio_settings
  portaudio_settings = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), portaudio_settings, FALSE, TRUE, 0);

  GList *devices = get_portaudio_devices ();

#ifndef G_OS_WIN32
  /* if default is requested choose first in portaudio list, rather than rely on portaudio which fails to select a default */
  if ((!strcmp (HistoricHarpsichord.prefs.portaudio_device->str, "default")) && (g_list_length (devices) > 1))
    g_string_assign (HistoricHarpsichord.prefs.portaudio_device, (gchar *) (devices->next->data));
#endif
  COMBOBOX (_("Audio Output Device: "), portaudio_device, devices, HistoricHarpsichord.prefs.portaudio_device->str, FALSE);
  free_portaudio_devices (devices);

#undef VBOX
#define VBOX main_vbox

#endif // _HAVE_PORTAUDIO_

  SEPARATOR ();

  /*
   * FluidSynth settings
   */
  TEXTENTRY (_("Soundfont"), fluidsynth_soundfont) 
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);
  GtkWidget *button = gtk_button_new_with_label (_("Choose Soundfont"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (choose_sound_font), fluidsynth_soundfont);

  gtk_widget_show (button);
  
  SEPARATOR ();

  GtkWidget *temperament = get_temperament_combo ();
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (VBOX), hbox, FALSE, TRUE, 0);
  label = gtk_label_new ("Temperament: ");
  gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);\
  gtk_box_pack_start (GTK_BOX (hbox), temperament, FALSE, TRUE, 0);

  INTENTRY_LIMITS (_("Compress Variations in Dynamics by %"), dynamic_compression, 0, 100);
  BOOLEANENTRY (_("Avoid abrupt damping"), damping);
  BOOLEANENTRY (_("Low Pitch"), lowpitch);


  gtk_widget_show_all (dialog);
  audio_cbdata.dialog = dialog;

  audio_cbdata.audio_driver = cbdata.audio_driver;
  audio_cbdata.midi_driver = cbdata.midi_driver;

#ifdef _HAVE_PORTAUDIO_
  audio_cbdata.portaudio_settings = portaudio_settings;
#endif
#ifdef _HAVE_PORTMIDI_
  audio_cbdata.portmidi_settings = portmidi_settings;
#endif

  midi_audio_tab_update (NULL, (gpointer *) & audio_cbdata);


#define SETCALLBACKDATA(field) \
  cbdata.field = field;

  cbdata.prefs = &HistoricHarpsichord.prefs;

 

    if (HistoricHarpsichord.setup)
      {
      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        {
          set_preferences (&cbdata);
          free_g_lists (&cbdata);
        }
      else
        {
          exit(0);
        }
      }
    gtk_widget_hide (dialog);
}
