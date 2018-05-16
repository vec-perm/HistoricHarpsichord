/* historicHarpsichord.h
 *
 *
 *
 * (c) 1999-2005 Matthew Hiller, Adam Tee
 * (c) 2018 Richard Shann
 *
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
#ifndef HISTORICHARPSICHORD_DATASTRUCTURES
#define HISTORICHARPSICHORD_DATASTRUCTURES
#ifdef __cplusplus
extern "C" {
#endif

/* Include the internationalization stuff.  */
#include <libintl.h>
#include <locale.h>

#define _(String) gettext (String)
#ifndef gettext_noop
# define gettext_noop(String) String
#endif
#ifndef N_
# define N_(String) gettext_noop (String)
#endif


#include <glib.h>
#include <gmodule.h>

#include "historicHarpsichord_types.h"


#define gtk_misc_set_alignment(a,x,y) 

#define PREFS_FILE "HistoricHarpsichordrc"
#if GTK_MAJOR_VERSION == 3
#define gtk_vbox_new(homogeneous, spacing) gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)
#define gtk_hbox_new(homogeneous, spacing) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)
#endif
#define SOUNDFONTS_DIR "soundfonts"

typedef enum{

  HISTORICHARPSICHORD_DIR_SOUNDFONTS,
 
} HistoricHarpsichordDirectory;

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef HISTORICHARPSICHORD_DATASTRUCTURES  */

