/* utils.c
 * Functions useful across the different modules of
 * drawing and non-drawing code.
 *
 * HistoricHarpsichord - a synthesizer for an historic harpsichord sound
 * (c) 1999-2005 Matthew Hiller
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
#include <stdio.h>
#include <string.h>             /*for SIGTERM */
#include <ctype.h>
#include <math.h>
#include <fontconfig/fontconfig.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h> //for g_remove. Use GFile instead?
#include <stdlib.h>
#include <historicHarpsichord/historicHarpsichord.h>
#include "core/utils.h"
#include <signal.h>             /*for SIGTERM */

#include "config.h"
#ifdef G_OS_WIN32
#include "windows.h"
#else
#include "core/binreloc.h"
#endif
#include "audio/temperament.h"
#ifdef _MACH_O_
#include <mach-o/dyld.h>
#endif

/**
 * This checks to see if there's a .historicHarpsichord/ directory in the user's
 * home directory,
 * if create tries to create one if there isn't, and returns the
 * path to it
 * else returns NULL
 *
 * .historicHarpsichord/ is used for holding configuration files, templates, and so on.
 *
 * On windows the home directory is the one containing the My Documents folder.
 */

const gchar *
get_user_data_dir (gboolean create)
{
  static gchar *dothistoricHarpsichord = NULL;

  gboolean err;
  if (!dothistoricHarpsichord)
    {
      dothistoricHarpsichord = g_build_filename (g_get_home_dir (), ".historicHarpsichord-" PACKAGE_VERSION, NULL);
    }
  if ((!create) && !g_file_test (dothistoricHarpsichord, G_FILE_TEST_IS_DIR))
    return NULL;
  err = g_mkdir_with_parents (dothistoricHarpsichord, 0770);
 
  return dothistoricHarpsichord;
}

// Create or remove a unique temporary directory
// If removal is FALSE, the directory will be newly
// created or the existing temporary directory will
// be returned.
// If removal is TRUE, the directory gets removed and NULL is returned.
gchar *
make_temp_dir (gboolean removal)
{
  static gchar *tmpdir = NULL;
  if (!removal)
    {
      // Either create a new directory or get the path
      if (!tmpdir)
        {
          gchar *newdir = g_build_filename (g_get_tmp_dir (), "HistoricHarpsichord_XXXXXX", NULL);
#ifdef G_OS_WIN32
          // Windows does not delete the temporary directory, use a constant one.
          if (!g_mkdir_with_parents (newdir, 0700))
            g_warning ("Creation of temp dir %s failed\n", newdir);
#else
          if (!g_mkdtemp (newdir))
            g_warning ("Creation of temp dir %s failed\n", newdir);
#endif
          tmpdir = newdir;
        }
    }
  else
    {
      // The directory should be removed.
      // Remove all files in the directory before deleting it.
      GError *error = NULL;
      GDir *dir = g_dir_open (tmpdir, 0, &error);
      if (!error)
        {
          const gchar *filename;
          while (filename = g_dir_read_name (dir))
            {
              gchar *fullpath = g_build_filename (tmpdir, filename, NULL);
              g_remove (fullpath);
              g_free (fullpath);
            }
          g_dir_close (dir);
        }
      g_remove (tmpdir);
      tmpdir = NULL;
    }
    return tmpdir;
}

// Return a path to a temporary directory to be used for print intermediate files
const gchar *
locateprintdir (void)
{
  return make_temp_dir (FALSE);
}

// Remove the temporary directory
void
removeprintdir (void)
{
  make_temp_dir (TRUE);
}

//copies all files in source_dir to dest_dir creating the latter if need be
void copy_files (gchar *source_dir, gchar *dest_dir)
{
 GError *error = NULL;
 GDir *thedir;
 const gchar *thefile;
 gsize length;
 if (-1 == g_mkdir_with_parents (dest_dir, 0770))
    {
        g_warning ("Could not create %s\n", dest_dir);
        return;
    }
 thedir = g_dir_open (source_dir, 0, &error);
 if (error)
    {
        g_warning ("Could not open %s\n", source_dir);
        return;
    }
 if (thedir)
    {
    while (thefile = g_dir_read_name (thedir))
        {
           gchar *contents;
           gchar *path = g_build_filename (source_dir, thefile, NULL);
           gchar *newfile = g_build_filename (dest_dir, thefile, NULL);
           if (g_file_get_contents (path, &contents, &length, &error))
                 g_file_set_contents (newfile, contents, length, &error);
          if (error)
            g_warning ("Failed to copy file %s to %s message: %s\n", thefile, newfile, error->message);
          g_free (contents);
          g_free (newfile);
          g_free (path);
        }
    g_dir_close (thedir);
   }
}


void
add_font_directory (gchar * fontpath)
{
#ifdef G_OS_WIN32
  if (0 == AddFontResource (fontpath))
    g_warning ("Failed to add font dir %s.", fontpath);
  SendMessage (HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
#endif
  if (FcConfigAppFontAddDir (NULL, (FcChar8 *) fontpath) == FcFalse)
    g_warning ("Failed to add font dir %s.", fontpath);
}

void
add_font_file (gchar * fontname)
{
#ifdef G_OS_WIN32
  if (0 == AddFontResource (fontname))
    g_warning ("Failed to add font file %s.", fontname);
  SendMessage (HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
#endif
  if (FcConfigAppFontAddFile (NULL, (FcChar8 *) fontname) == FcFalse)
    g_warning ("Failed to add font file %s.", fontname);
}


/**
 * g_list_foreach helper function to free the given data
 * @param data the list elements data
 * @param user_data any user supplied data (not used in this case)
 */
void
freeit (gpointer data, gpointer user_data)
{
  g_free (data);
}



/**
 * Function that initializes the code needed for the directory relocation.
 * @return none
 */
void
initdir ()
{
#ifndef G_OS_WIN32
  GError *error = NULL;
  if (!gbr_init (&error) && (error != (GError *) GBR_INIT_ERROR_DISABLED))
    {
      g_debug ("BinReloc failed to initialize:\n");
      g_debug ("Domain: %d (%s)\n", (int) error->domain, g_quark_to_string (error->domain));
      g_debug ("Code: %d\n", error->code);
      g_debug ("Message: %s\n", error->message);
      g_error_free (error);
      g_debug ("----------------\n");
    }
#endif /* not G_OS_WIN32 */
}

extern gchar *gbr_find_pkg_data_dir (const gchar * default_pkg_data_dir, const gchar * pkg_name);

static gchar *HISTORICHARPSICHORD_datadir = NULL;
const gchar *
get_system_data_dir ()
{
  //HISTORICHARPSICHORD_datadir?g_debug("datadir is %s at %p", HISTORICHARPSICHORD_datadir, HISTORICHARPSICHORD_datadir):g_debug("datadir not yet set");
  if (HISTORICHARPSICHORD_datadir == NULL)
    {
#ifdef G_OS_WIN32
      gchar *rootdir = g_win32_get_package_installation_directory (NULL, NULL);
      HISTORICHARPSICHORD_datadir = g_build_filename (rootdir, "share", "historicHarpsichord", NULL);
      g_message ("rootdir=%s", rootdir);
      g_message ("datadir=%s", HISTORICHARPSICHORD_datadir);
      g_free (rootdir);
#else /* not G_OS_WIN32 */

#ifdef _MACH_O_

      {
        char path[1024];
        guint size = sizeof (path);
        _NSGetExecutablePath (path, &size);
        gchar *bindir = (gchar *) g_malloc (size);
        if (_NSGetExecutablePath (bindir, &size) == 0)
          g_message ("Using bin path %s", bindir);
        else
          g_critical ("Cannot get bin dir");
        HISTORICHARPSICHORD_datadir = g_build_filename (g_path_get_dirname (bindir), "..", "share", "historicHarpsichord", NULL);
        g_message ("OSX set data dir to %s", HISTORICHARPSICHORD_datadir);
      }
#else
  HISTORICHARPSICHORD_datadir = g_build_filename (PREFIX, "share", "historicHarpsichord", NULL);
#endif //_MACH_O_
#endif /* not G_OS_WIN32 */
    }
    g_print ("Set soundfont dir to %s/soundfonts\n\n\n", HISTORICHARPSICHORD_datadir);
  return HISTORICHARPSICHORD_datadir;
}


gchar *
get_system_dir (HistoricHarpsichordDirectory dir)
{
  switch (dir)
    {

    case HISTORICHARPSICHORD_DIR_SOUNDFONTS:
      return g_build_filename (get_system_data_dir (), "soundfonts", NULL);

    default:
      return NULL;
    }
}

const gchar *
get_system_locale_dir ()
{
  static gchar *localedir = NULL;
  if (localedir == NULL)
    {
#ifdef G_OS_WIN32
      gchar *rootdir = g_win32_get_package_installation_directory (NULL, NULL);
      localedir = g_build_filename (rootdir, "share", "locale", NULL);
      g_free (rootdir);
#else /* not G_OS_WIN32 */
#ifdef _MACH_O_

      {
        char path[1024];
        guint size = sizeof (path);
        _NSGetExecutablePath (path, &size);
        gchar *bindir = (gchar *) g_malloc (size);
        if (_NSGetExecutablePath (bindir, &size) == 0)
          g_message ("Using bin path %s", bindir);
        else
          g_critical ("Cannot get bin dir");
        localedir = g_build_filename (g_path_get_dirname (bindir), "..", "share", "locale", NULL);
        g_message ("OSX set locale dir to %s", localedir);
      }
#else
#ifndef ENABLE_BINRELOC
      /* it seems to be the standard way (no binreloc)
       * to set the path of translations this way:
       * messages are in $LOCALEDIR/$LANG/denemo
       */
      localedir = g_strdup (LOCALEDIR);
#else /* ENABLE_BINRELOC */
      /* binreloc says it is disabled even with built thanks to
       * --enable-binreloc... So, searhing falls back to
       *  $LOCALEDIR/denemo/$LANG which is not a valid path
       */
      localedir = gbr_find_locale_dir (LOCALEDIR);
#endif /* ENABLE_BINRELOC */
#endif
#endif /* not G_OS_WIN32 */
    }
  return localedir;
}
