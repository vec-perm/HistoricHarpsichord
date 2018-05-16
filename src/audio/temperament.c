/* temperament.c
 
 * (c)2007 Richard Shann
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
#include <math.h>
#include <string.h>             /* for strcmp() */
#include <stdlib.h>             /* for abs() */
#include <gtk/gtk.h>
#include "historicHarpsichord/historicHarpsichord.h"
#include "audio/midi.h"
#include "audio/audiointerface.h"

typedef struct notespec
{
  guint step;                   /* 0=C 1=D ... 6=B */
  gint alteration;              /* -2 = bb, -1=b, 0=natural, 1=#, 2=x */
  gint cents;                   //deviation from Equal
} notespec;

typedef struct notepitch
{

  double pitch;                 /* pitch of note of given notespec in range C=260 to B=493 Hz
                                   actual pitch is *=2^octave   */
  notespec spec;
} notepitch;

typedef struct temperament
{
  gchar *name;
  gint unusedsharp;             /* which notepitch is the sharpest in circle of 5ths - initially G# */
  gint unusedflat;              /* which notepitch is the flattest in circle of 5ths - initially Eb */
  notepitch notepitches[12];    /* pitches of C-natural, C#, ... A#, B-natural */
} temperament;


static gint temperament_offset = 0;     /* shift in temperament around circle of 5ths, not currently used */

static temperament Pythagorean = {
  "Pythagorean", 8, 3,
  {
   {261.6, {0, 0, 0}},
   {279.4, {0, 1, 0}},
   {294.3, {1, 0, 0}},
   {310.1, {2, -1,0}},
   {331.1, {2, 0, 0}},
   {348.8, {3, 0, 0}},
   {372.5, {3, 1, 0}},
   {392.4, {4, 0, 0}},
   {419.1, {4, 1, 0}},
   {441.5, {5, 0, 0}},
   {465.1, {6, -1,0}},
   {496.7, {6, 0, 0}}
   }
};


static temperament Rameau = {
  "Rameau", 8, 3,
  {
   {261.6, {0, 0, 0}},
   {276.9, {0, 1, 0}},
   {293.3, {1, 0, 0}},
   {309.7, {2, -1,0}},
   {330.4, {2, 0, 0}},
   {348.8, {3, 0, 0}},
   {369.2, {3, 1, 0}},
   {391.1, {4, 0, 0}},
   {413.9, {4, 1, 0}},
   {438.5, {5, 0, 0}},
   {463.0, {6, -1,0}},
   {493.9, {6, 0, 0}}
   }
};


static temperament Equal = {
  "Equal", 8, 3,
  {
   {261.6, {0, 0, 0}},
   {277.2, {0, 1, 0}},
   {293.7, {1, 0, 0}},
   {311.1, {2, -1, 0}},
   {329.6, {2, 0, 0}},
   {349.2, {3, 0, 0}},
   {370.0, {3, 1, 0}},
   {392.0, {4, 0, 0}},
   {415.3, {4, 1, 0}},
   {440.0, {5, 0, 0}},
   {466.2, {6, -1, 0}},
   {493.9, {6, 0, 0}}
   }
};

static temperament Lehman = {
  "Lehman", 8, 3,
  {
   {262.37 * 1, {0, 0, 0}},
   {262.37 * 1.0590, {0, 1, 0}},
   {262.37 * 1.1203, {1, 0, 0}},
   {262.37 * 1.1889, {2, -1, 0}},
   {262.37 * 1.2551, {2, 0, 0}},
   {262.37 * 1.3360, {3, 0, 0}},
   {262.37 * 1.4120, {3, 1, 0}},
   {262.37 * 1.4968, {4, 0, 0}},
   {262.37 * 1.5869, {4, 1, 0}},
   {262.37 * 1.6770, {5, 0, 0}},
   {262.37 * 1.7816, {6, -1, 0}},
   {262.37 * 1.8827, {6, 0, 0}}
   }
};

static temperament Meantone = {
  "Quarter Comma Meantone", 8, 3,
  {
   {261.6, {0, 0, 0}},
   {272.8, {0, 1, 0}},
   {292.3, {1, 0, 0}},
   {313.2, {2, -1, 0}},
   {326.7, {2, 0, 0}},
   {350.0, {3, 0, 0}},
   {365.0, {3, 1, 0}},
   {391.1, {4, 0, 0}},
   {407.9, {4, 1, 0}},
   {437.0, {5, 0, 0}},
   {468.3, {6, -1, 0}},
   {488.3, {6, 0, 0}}
   }
};

//  261.6 279.4 294.3 314.3 331.1 353.6 372.5 392.4 419.1 441.5 471.5 496.7
  static temperament VanZwolle = {
  "Van Zwolle", 8, 3,
  {
   {261.6, {0, 0, 0}},            /* c */
   {279.4, {0, 1, 0}},
   {294.3, {1, 0, 0}},            /* d */
   {314.3, {2, -1, 0}},           /* Eb */
   {331.1, {2, 0, 0}},            /* e */
   {353.6, {3, 0, 0}},            /* f */
   {372.5, {3, 1, 0}},
   {392.4, {4, 0, 0}},            /* g */
   {419.1, {4, 1, 0}},            /* g# */
   {441.5, {5, 0, 0}},            /* a */
   {471.5, {6, -1, 0}},           /* Bb */
   {496.7, {6, 0, 0}}             /* b */
   }
};

//261.6 275.0 293.0 312.2 328.1 349.6 367.5 391.6 411.6 438.5 467.2 491.1
static temperament SilbermannI = {
  "Silbermann I", 8, 3,
  {
   {261.6, {0, 0, 0}},            /* c */
   {275.0, {0, 1, 0}},
   {293.0, {1, 0, 0}},            /* d */
   {312.2, {2, -1, 0}},           /* Eb */
   {328.1, {2, 0, 0}},            /* e */
   {349.6, {3, 0, 0}},            /* f */
   {367.5, {3, 1, 0}},
   {391.6, {4, 0, 0}},            /* g */
   {411.6, {4, 1, 0}},            /* g# */
   {438.5, {5, 0, 0}},            /* a */
   {467.2, {6, -1, 0}},           /* Bb */
   {491.1, {6, 0, 0}}             /* b */
   }
};

// 261.6 276.2 293.0 312.2 329.6 349.6 369.2 391.6 413.4 438.5 467.2 493.3
static temperament SilbermannII = {
  "Silbermann II", 8, 3,
  {
   {261.6, {0, 0, 0}},            /* c */
   {276.2, {0, 1, 0}},
   {293.0, {1, 0, 0}},            /* d */
   {312.2, {2, -1, 0}},           /* Eb */
   {329.6, {2, 0, 0}},            /* e */
   {349.6, {3, 0, 0}},            /* f */
   {369.2, {3, 1, 0}},
   {391.6, {4, 0, 0}},            /* g */
   {413.4, {4, 1, 0}},            /* g# */
   {438.5, {5, 0, 0}},            /* a */
   {467.2, {6, -1, 0}},           /* Bb */
   {493.3, {6, 0, 0}}             /* b */
   }
};

//261.6 277.5 293.3 311.1 328.9 350.0 370.0 391.1 414.8 438.5 466.7 493.3
static temperament WerckmeisterIII = {
  "Werckmeister III", 8, 3,
  {
   {261.6, {0, 0, 0}},            /* c */
   {277.5, {0, 1, 0}},
   {293.3, {1, 0, 0}},            /* d */
   {311.1, {2, -1, 0}},           /* Eb */
   {328.9, {2, 0, 0}},            /* e */
   {350.0, {3, 0, 0}},            /* f */
   {370.0, {3, 1, 0}},
   {391.1, {4, 0, 0}},            /* g */
   {414.8, {4, 1, 0}},            /* g# */
   {438.5, {5, 0, 0}},            /* a */
   {466.7, {6, -1, 0}},           /* Bb */
   {493.3, {6, 0, 0}}             /* b */
   }
};

static temperament WerckmeisterIV = {
  "Werckmeister IV", 8, 3,
  {
   {263.11, {0, 0, 0}},            /* c */
   {275.93, {0, 1, 0}},
   {294.66, {1, 0, 0}},            /* d */
   {311.83, {2, -1, 0}},           /* Eb */
   {330.00, {2, 0, 0}},            /* e */
   {350.81, {3, 0, 0}},            /* f */
   {369.58, {3, 1, 0}},
   {392.88, {4, 0, 0}},            /* g */
   {413.90, {4, 1, 0}},            /* g# */
   {440.00, {5, 0, 0}},            /* a */
   {469.86, {6, -1, 0}},           /* Bb */
   {492.77, {6, 0, 0}}             /* b */
   }
};


static temperament *PR_temperament = &Equal;    /* the currently used temperament */

static temperament *temperaments[] = { &Equal, &Meantone, &WerckmeisterIII,  &WerckmeisterIV, &Lehman, &Rameau, &Pythagorean, &SilbermannII, &SilbermannI, &VanZwolle};

/* return an array of values representing deviations from equal temperament for 12 notes from C for the passed temperament. Returned value is read only */
static gdouble *
get_cents (temperament * t)
{
  static gdouble array[12];
  int i, j;
  for (i = 0; i < 12; i++)
    {
      j = (i + temperament_offset) % 12;
      //g_print("cents tempered %d to %d unshifted %f shifted %f\n", i, j, 1200 * log2(t->notepitches[i].pitch/Equal.notepitches[i].pitch), 1200 * log2(t->notepitches[j].pitch/Equal.notepitches[j].pitch));
      array[i] = 1200 * log2 (t->notepitches[j].pitch / Equal.notepitches[j].pitch);
    }
  return array;
}


void
set_tuning (void)
{
  change_tuning (get_cents (PR_temperament));
}

#define COLUMN_NAME (0)
#define COLUMN_PTR (1)

static void
temperament_changed_callback (GtkComboBox * combobox, GtkListStore * list_store)
{
  GtkTreeIter iter;
  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &iter);
  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, COLUMN_PTR, &PR_temperament, -1);
  set_tuning ();                //note synth may not be attached...
  g_string_assign (HistoricHarpsichord.prefs.temperament, PR_temperament->name);
}


GtkWidget *
get_temperament_combo (void)
{
  static GtkWidget *combobox;
  if (combobox == NULL)
    {
      GtkListStore *list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
      GtkCellRenderer *renderer;
      combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list_store));
      gtk_widget_set_tooltip_text (combobox, _("Set the musical temperament (tuning) to be used for playback."));
      g_object_ref (combobox);
      PR_temperament = &Equal;
      int i;
      for (i = 0; i < (gint) G_N_ELEMENTS (temperaments); i++)
        {
          GtkTreeIter iter;
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (list_store, &iter, COLUMN_NAME, temperaments[i]->name, COLUMN_PTR, temperaments[i], -1);

          if ((i == 0) || (HistoricHarpsichord.prefs.temperament && !strcmp (HistoricHarpsichord.prefs.temperament->str, temperaments[i]->name)))
            {
              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
              PR_temperament = temperaments[i];
            }
        }
      renderer = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
      gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combobox), renderer, "text", COLUMN_NAME);
      g_signal_connect (G_OBJECT (combobox), "changed", G_CALLBACK (temperament_changed_callback), list_store);
      //set_tuning();
    }
  return combobox;
}


