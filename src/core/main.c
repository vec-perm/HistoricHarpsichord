/* main.c
 * sets up the GUI and connects the main callback functions.
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * (c) 1999-2005 Matthew Hiller, Adam Tee
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
#include "config.h"
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gtk/gtk.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_WAIT_H
#include <wait.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "historicHarpsichord/historicHarpsichord.h"
#include <sys/types.h>
#include <dirent.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "core/utils.h"
#include "core/prefops.h"
#include "audio/audiointerface.h"

struct HistoricHarpsichordRoot HistoricHarpsichord;

static gchar **
process_command_line (int argc, char **argv, gboolean gtkstatus)
{
  GError *error = NULL;
  GOptionContext *context;
  gchar* scheme_script_name = NULL;
  gboolean version = FALSE;
  gchar **filenames = NULL;

  GOptionEntry entries[] =
  {
    { "setup",              's', 0, G_OPTION_ARG_NONE, &HistoricHarpsichord.setup, _("Setup The Harpsichord Synthesizer"), NULL },
    { "verbose",             'V', 0, G_OPTION_ARG_NONE, &HistoricHarpsichord.verbose, _("Display every messages"), NULL },
    { "audio-options",       'A', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &HistoricHarpsichord.prefs.audio_driver,_("Audio driver options"), _("options") },
    { "midi-options",        'M', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &HistoricHarpsichord.prefs.midi_driver, _("Midi driver options"), _("options") },
    { G_OPTION_REMAINING,    0,   0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, _("[FILE]...") },
    { NULL }
  };
  const gchar* subtitle = _(" ");
  gchar *header = g_strconcat (_("GNU HistoricHarpsichord version"), " ", VERSION, "\n",
                        _("HistoricHarpsichord is a graphical music notation editor.\n"
                          "It uses GNU Lilypond for music typesetting.\n"
                          "HistoricHarpsichord is part of the GNU project."), NULL);
  const gchar* footer = _("Report bugs to http://www.historicHarpsichord.org\n"
                          "GNU HistoricHarpsichord, a free and open music notation editor");

  context = g_option_context_new (subtitle);
  g_option_context_set_summary (context, header);
  g_free(header);
  g_option_context_set_description (context, footer);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  if(gtkstatus)
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
    g_error ("Option parsing failed: %s", error->message);

  if(version)
  {
    gchar *message = g_strconcat (
    _("GNU HistoricHarpsichord version"), " ", VERSION, "\n",
    _("Gtk versions") , " runtime: %u.%u.%u, compiled against: %u.%u.%u, \n",
    _("© 1999-2005, 2009 Matthew Hiller, Adam Tee, and others, 2010-2015 Richard Shann, Jeremiah Benham, Nils Gey and others.\n"),
    _("This program is provided with absolutely NO WARRANTY; see the file COPYING for details.\n"),
    _("This software may be redistributed and modified under the terms of the GNU General Public License; again, see the file COPYING for details.\n"),
    NULL);
    g_print(message, gtk_major_version, gtk_minor_version, gtk_micro_version, GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_free(message);
    exit(EXIT_SUCCESS);
  }

  if(HistoricHarpsichord.prefs.audio_driver)
    g_string_ascii_down (HistoricHarpsichord.prefs.audio_driver);

  if(HistoricHarpsichord.prefs.midi_driver)
    g_string_ascii_down (HistoricHarpsichord.prefs.midi_driver);

  return filenames;
}

static void
localization_init()
{
  setlocale (LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, get_system_locale_dir ());
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
}



int
main (int argc, char *argv[])
{
  gchar** files = NULL;
  gboolean gtk_status = FALSE;

 // g_log_set_default_handler (main_log_handler, NULL);

  if(!(gtk_status = gtk_init_check (&argc, &argv)))
    g_message(_("Could not start graphical interface."));

  files = process_command_line (argc, argv, gtk_status);


  /* initialization of directory relocatability */
  initdir ();

  //check_if_upgrade();
  //init_environment();
  localization_init();
  initprefs (); 
  
  //project Initializations
  if (audio_initialize (&HistoricHarpsichord.prefs))
    {
    g_print ("Failed to initialize audio or MIDI backends");
    gchar *title, *message;
    title =_("This Program is corrupted");
    message = g_strdup_printf ("<span font_desc=\"24\" foreground=\"red\">%s</span>", "Try running Setup and choosing a soundfont with bank 0 program 0 a harpsichord");

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
    }
  else
    {
    gchar *title, *message;
    title =g_strdup_printf( "%s %s %s",_("Harpsichord "), HistoricHarpsichord.prefs.temperament?  HistoricHarpsichord.prefs.temperament->str : "Equal", " Temperament");
    message = g_strdup_printf ("<span font_desc=\"24\" foreground=\"red\">%s %s %s %s %s</span>", _("Click to turn"), "<span font_desc=\"42\">A =", HistoricHarpsichord.prefs.lowpitch? "415" : "440", "</span>", _("harpsichord off"));

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
    }
  gtk_main ();


  return 0;
}
