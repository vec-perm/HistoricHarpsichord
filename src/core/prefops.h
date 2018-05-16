/* prefops.h
 * header file for initializing preferences
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * (c) 1999-2005 Matthew Hiller
 */
#ifndef PREFOPSH
#define PREFOPSH

#include <historicHarpsichord/historicHarpsichord.h>

#define INITIAL_WIDTH 1000
#define INITIAL_HEIGHT 500


void initprefs (void);
gint readxmlprefsString (gchar * xmlprefs);
gint writeXMLPrefs (HistoricHarpsichordPrefs * prefs);


gboolean get_bool_pref (gchar * prefname);
gint get_int_pref (gchar * prefname);
gchar *get_string_pref (gchar * prefname);

void preferences_change (void);//in prefdialog.c
#endif
