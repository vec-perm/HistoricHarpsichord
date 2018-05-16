/* utils.h
 * Header file for functions useful across the different modules of
 * drawing and non-drawing code.
 *
 * also includes useful constants
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * (c) 1999-2005 Matthew Hiller, 2008, 2009 Richard Shann
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
#ifndef UTILS_H
#define UTILS_H

//FIXME, these two need to be an enum in historicHarpsichord_types.h and the field showaccidental changed from boolean to this type.
#define HISTORICHARPSICHORD_REMINDER (2)
#define HISTORICHARPSICHORD_CAUTIONARY (3)

#define HIGHLIGHT_OFFSET (20)   /* Fairly arbitrary value to transform codes '0', '1' ... to a new range with the meaning highlight the whole-note, half-note, ..., glyph */
#define MAXEXTRASPACE (150)     /* maximum space for ledger lines, for sanity */

#define LINE_SPACE 10
#define HALF_LINE_SPACE 5
#define NO_OF_LINES 5
#define STAFF_HEIGHT (LINE_SPACE * (NO_OF_LINES - 1))
#define LYRICS_HEIGHT (STAFF_HEIGHT/2)
#define MID_STAFF_HEIGHT 2*LINE_SPACE
/*#define LEFT_MARGIN 20 now variable for braces to take space */
//#define KEY_MARGIN (LEFT_MARGIN+35)
#define SPACE_FOR_TIME 35
#define RIGHT_MARGIN 20
#define SPACE_FOR_BARLINE 10
#define HALF_BARLINE_SPACE 5
#define WHOLE_NUMTICKS 1536
#define FONT "Sans 9"
#define TIMESIGFONT "Sans 24"

#ifndef g_info
#ifdef G_HAVE_ISO_VARARGS
#define g_info(...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, __VA_ARGS__)
#elif defined(G_HAVE_GNUC_VARARGS)
#define g_info(format) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif
#endif

const gchar *get_user_data_dir (gboolean create);


gchar *pretty_name (gchar * lilynote);


void freeit (gpointer data, gpointer user_data);



void initdir (void);
const gchar *get_system_data_dir (void);
/* UNUSED
const gchar *get_system_conf_dir (void);
*/
const gchar *get_system_locale_dir (void);
const gchar *get_system_bin_dir (void);
const gchar *get_system_font_dir (void);
const gchar *get_executable_dir (void);
/* get directory of current HistoricHarpsichord.project or home directory if untitled. User must free the returned string */
gchar *get_project_dir (void);
const gchar *get_local_dir (HistoricHarpsichordDirectory dir);
gchar *get_system_dir (HistoricHarpsichordDirectory dir);

void copy_files (gchar *source_dir, gchar *dest_dir);//copies all files in source_dir to dest_dir creating the latter if need be

void nullify_gstring (GString ** s);
gchar *choose_file (gchar * title, gchar * startdir, GList * extensions);


void add_font_directory (gchar * fontpath);
void add_font_file (gchar * fontpath);
const gchar *get_prefix_dir (void);
gboolean run_file_association (gchar * filenam);
gchar *remove_extension (gchar * name);
gchar *substitute_extension (gchar * name, gchar * extension);
void init_historicHarpsichord_notenames (void);
gint get_widget_height (GtkWidget * w);
gint get_widget_width (GtkWidget * w);
void switch_back_to_main_window (void);
void use_markup (GtkWidget * widget);

void write_input_status ();
enum clefs cleftypefromname (gchar * str);
gchar *find_dir_for_file (gchar * filename, GList * dirs);
gchar *find_dir_for_files (GList * files, GList * dirs);
gchar *find_path_for_file (gchar * filename, GList * dirs);
gchar *find_historicHarpsichord_file (HistoricHarpsichordDirectory dir, gchar * filename);
gchar *escape_scheme (gchar * input);
gchar *time_spent_editing (void);
void reset_editing_timer (void);
gboolean shift_held_down (void);

#if GTK_MAJOR_VERSION == 2
#define GdkRGBA GdkColor
#define gtk_widget_override_color gtk_widget_modify_fg
#define gtk_widget_override_background_color gtk_widget_modify_bg
#define GTK_STATE_FLAG_NORMAL (0)
void get_color (GdkColor * color, gdouble r, gdouble g, gdouble b, gdouble a);
#else
void get_color (GdkRGBA * color, gdouble r, gdouble g, gdouble b, gdouble a);
#define gtk_widget_override_background_color(w,f,c) {gchar *color = gdk_rgba_to_string(c);set_background_color(w,color);g_free(color);}
#endif
void set_foreground_color(GtkWidget *w, gchar *color);
void set_background_color(GtkWidget *w, gchar *color);

gchar *format_tooltip (const gchar *tip);

#endif /* UTILS_H */
