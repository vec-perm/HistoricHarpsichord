/* prefops.c
 * Functions for initializing preferences
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
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "config.h"
#include <historicHarpsichord/historicHarpsichord.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "core/utils.h"
#include "core/prefops.h"

static gint readxmlprefsFile (gchar * filename);


#define ret (&HistoricHarpsichord.prefs)

/**
 * Initialise user preferences to reasonable defaults
 * read global historicHarpsichordrc file
 * then local preferences file
 *
 */
void
initprefs ()
{
  gchar *dothistoricHarpsichord = (gchar *) get_user_data_dir (TRUE);
  gchar *localrc = dothistoricHarpsichord ? g_build_filename (dothistoricHarpsichord, PREFS_FILE, NULL) : NULL;

  /* Reasonable default values */

 

  ret->temperament = g_string_new ("Equal");




  ret->audio_driver = g_string_new ("default");
  ret->midi_driver = g_string_new ("default");
#ifdef _HAVE_PORTAUDIO_
  ret->audio_driver = g_string_new ("portaudio");
  ret->midi_driver = g_string_new ("portmidi");
#endif


  ret->portaudio_device = g_string_new ("default");
  ret->portaudio_sample_rate = 44100;
  ret->portaudio_period_size = 256;

  ret->portmidi_input_device = g_string_new ("default");
  ret->portmidi_output_device = g_string_new ("default");

  gchar *soundfontpath = g_build_filename (get_system_dir(HISTORICHARPSICHORD_DIR_SOUNDFONTS), "Zell8ft.sf2", NULL);
  g_print ("Default soundfontpath %s\n\n\n\n", soundfontpath);
  ret->fluidsynth_soundfont = g_string_new (soundfontpath);
  
  ret->dynamic_compression = 100;
  ret->damping = 1;
  /* Read values from personal preferences file */

  //readpreffile (localrc, ret);
  if (localrc)
    {
      if (g_file_test (localrc, G_FILE_TEST_EXISTS))
        readxmlprefsFile (localrc);
      else
        {
           if (!HistoricHarpsichord.setup)
              HistoricHarpsichord.setup = TRUE;//force setup if no preferences file exists at startup
          writeXMLPrefs (ret);
        }
    }
  g_free (localrc);

#undef ret


}


/**
 * parseConfig searches the rc file for the configuration settings.
 *
 * @param doc document pointer
 * @param cur pointer to the current XML Node
 * @param prefs pointer to the preferences structure
 *
 */
static void
parseConfig (xmlDocPtr doc, xmlNodePtr cur, HistoricHarpsichordPrefs * prefs)
{
  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
#define READXMLENTRY(field)  \
      else if (0 == xmlStrcmp (cur->name, (const xmlChar *) #field))\
    {\
      xmlChar *tmp = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);\
      if(tmp)\
        {\
          prefs->field =\
        g_string_assign (prefs->field, (gchar *) tmp);\
          xmlFree (tmp);\
        }\
    }

#define READINTXMLENTRY(field) \
      else if (0 ==\
           xmlStrcmp (cur->name, (const xmlChar *) #field))\
    {\
      xmlChar *tmp = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);\
      if(tmp)\
        {\
          prefs->field = atoi ((gchar *) tmp);\
          xmlFree (tmp);\
        }\
    }

#define READDOUBLEXMLENTRY(field) \
      else if (0 ==\
           xmlStrcmp (cur->name, (const xmlChar *) #field))\
    {\
      xmlChar *tmp = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);\
      if(tmp)\
        {\
          prefs->field = atof ((gchar *) tmp);\
          xmlFree (tmp);\
        }\
    }


#define READBOOLXMLENTRY(field) \
      else if (0 ==\
           xmlStrcmp (cur->name, (const xmlChar *) #field))\
    {\
      xmlChar *tmp = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);\
      if(tmp)\
        {\
          prefs->field = atoi ((gchar *) tmp);\
          xmlFree (tmp);\
        }\
    }

  if (0 == xmlStrcmp (cur->name, (const xmlChar *) "temperament"))
    {
      xmlChar *tmp = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      if(tmp)
        {
          prefs->temperament = g_string_assign (prefs->temperament, (gchar *) tmp);
          xmlFree (tmp);
        }
    }
    READBOOLXMLENTRY (lowpitch)
  
    READXMLENTRY (audio_driver)
    READXMLENTRY (midi_driver)
    READXMLENTRY (portaudio_device)
    READINTXMLENTRY (portaudio_sample_rate)
    READINTXMLENTRY (portaudio_period_size)
    READXMLENTRY (portmidi_input_device)
    READXMLENTRY (portmidi_output_device)
    READXMLENTRY (fluidsynth_soundfont)
    READBOOLXMLENTRY (fluidsynth_reverb)
    READBOOLXMLENTRY (fluidsynth_chorus)
    cur = cur->next;
    }
  return;
}

#undef READXMLENTRY
#undef READBOOLXMLENTRY
#undef READINTXMLENTRY
#undef READDOUBLEXMLENTRY



#define GETBOOLPREF(field) \
  else if (!strcmp(prefname, #field))\
    return HistoricHarpsichord.prefs.field;


gboolean
get_bool_pref (gchar * prefname)
{
  if (*prefname == 0)
    return FALSE;
    GETBOOLPREF (lowpitch)
    GETBOOLPREF (damping)
    GETBOOLPREF (fluidsynth_reverb)
    GETBOOLPREF (fluidsynth_chorus)
    return FALSE;
}

#undef GETBOOLPREF
#define GETINTPREF(field) \
  else if (!strcmp(prefname, #field))\
    return HistoricHarpsichord.prefs.field;

gint
get_int_pref (gchar * prefname)
{
  if (*prefname == 0)
    return 0;

  GETINTPREF (portaudio_sample_rate)
  GETINTPREF (portaudio_period_size)
  GETINTPREF (dynamic_compression)
  
  return 0;
}

#undef GETINTPREF
#define GETSTRINGPREF(field) \
  else if (!strcmp(prefname, #field))\
    return HistoricHarpsichord.prefs.field->str;

gchar *
get_string_pref (gchar * prefname)
{
  if (*prefname == 0)
    return NULL;

    GETSTRINGPREF (temperament)
    GETSTRINGPREF (audio_driver)
    GETSTRINGPREF (midi_driver)

    GETSTRINGPREF (portaudio_device)
    GETSTRINGPREF (portmidi_input_device)
    GETSTRINGPREF (portmidi_output_device)
    GETSTRINGPREF (fluidsynth_soundfont) return NULL;
}



static gint readxmlprefs (gchar * xmlsource, gboolean from_file);

/**
 * Read a historicHarpsichord preferences xml file.
 * @param filename - historicHarpsichordrc file name
 * @param prefs - struct to populate data into
 *
 */

static gint
readxmlprefsFile (gchar * filename)
{
  return readxmlprefs (filename, TRUE);
}

/**
 * Read historicHarpsichord preferences from an xml string.
 * @param content - a string containing prefs in xml
 *
 */
gint
readxmlprefsString (gchar * content)
{
  gchar *xml = g_strconcat ("<?xml version=\"1.0\"?><HistoricHarpsichord><Config>", content, "</Config></HistoricHarpsichord>", NULL);
  gint ret = readxmlprefs (xml, FALSE);
  g_free (xml);
  return ret;
}



static gint
readxmlprefs (gchar * xmlsource, gboolean from_file)
{
  HistoricHarpsichordPrefs *prefs = &HistoricHarpsichord.prefs;
  gint ret = -1;
  xmlDocPtr doc = NULL;
  xmlNodePtr rootElem;
  if (from_file)
    g_message ("Loading preference file: %s", xmlsource);
  if (from_file)
    doc = xmlParseFile (xmlsource);
  else
    doc = xmlReadMemory (xmlsource, strlen (xmlsource), "noname.xml", NULL, 0);
  if (doc == NULL)
    {
      g_warning ("Could not read XML %s %s", from_file ? "File: " : ":", xmlsource);
      return ret;
    }

  rootElem = xmlDocGetRootElement (doc);
  if (rootElem == NULL)
    {
      g_warning ("Empty Document");
      xmlFreeDoc (doc);
      return ret;
    }
  //g_debug ("RootElem %s\n", rootElem->name);
  if (xmlStrcmp (rootElem->name, (const xmlChar *) "HistoricHarpsichord"))
    {
      g_warning ("Document has wrong type");
      xmlFreeDoc (doc);
      return ret;
    }
  rootElem = rootElem->xmlChildrenNode;
  while (rootElem != NULL)
    {
      g_debug ("RootElem 2 %s\n", rootElem->name);

      if (0 == xmlStrcmp (rootElem->name, (const xmlChar *) "Config"))
        {

          parseConfig (doc, rootElem, prefs);
        }

      rootElem = rootElem->next;

    }

  xmlFreeDoc (doc);

  ret = 0;
  return ret;
}

/**
 * Output an integer child as a child of the given node.
 *
 * @param parent - pointer to child nodes parent
 * @param name -  name for the child node
 * @param content - data to add to the child node
 */
static xmlNodePtr
newXMLIntChild (xmlNodePtr parent, const xmlChar * name, gint content)
{
  static gchar sIntBuf[12];     /* enough for -2000000000 + '\0' */
  sprintf (sIntBuf, "%d", content);
  return xmlNewChild (parent, NULL, name, (xmlChar *) sIntBuf);
}

/**
 * Output an double child as a child of the given node.
 *
 * @param parent - pointer to child nodes parent
 * @param name -  name for the child node
 * @param content - data to add to the child node
 */
static xmlNodePtr
newXMLDoubleChild (xmlNodePtr parent, const xmlChar * name, gdouble content)
{
  static GString *str;
  if (str == NULL)
    str = g_string_new ("");
  g_string_printf (str, "%f", content);
  return xmlNewChild (parent, NULL, name, (xmlChar *) str->str);
}


gint
writeXMLPrefs (HistoricHarpsichordPrefs * prefs)
{
  gint ret = -1;
  xmlDocPtr doc;
  xmlNodePtr parent, child;
  static GString *localrc = NULL;
  if (!localrc)
    {
      localrc = g_string_new (g_build_filename (get_user_data_dir (TRUE), PREFS_FILE, NULL));
    }

  doc = xmlNewDoc ((xmlChar *) "1.0");
  doc->xmlRootNode = parent = xmlNewDocNode (doc, NULL, (xmlChar *) "HistoricHarpsichord", NULL);
  child = xmlNewChild (parent, NULL, (xmlChar *) "Config", NULL);

#define WRITEXMLENTRY(field) \
  if (prefs->field){\
    gchar *def = g_strdup("Holds the value of the user's " #field " preference");\
    gchar *curname = g_strdup_printf("HistoricHarpsichordPref_%s", #field);\
    g_free(curname);\
    g_free(def);\
    xmlNewChild (child, NULL, (xmlChar *) #field,\
         (xmlChar *) prefs->field->str);}

#define WRITEXMLENTRY2(field) \
  if (prefs->field){\
    gchar *def = g_strdup("Holds the value of the user's " #field " preference");\
    gchar *curname = g_strdup_printf("HistoricHarpsichordPref_%s", #field);\
    g_free(curname);\
    g_free(def);\
    xmlNewChild (child, NULL, (xmlChar *) #field,\
         (xmlChar *) prefs->field);}

#define WRITEINTXMLENTRY(field){ \
    gchar *def = g_strdup("Holds the integer value of the user's " #field " preference");\
    gint value = prefs->field;\
    gchar *curname = g_strdup_printf("HistoricHarpsichordPref_%s", #field);\
    g_free(curname);\
    g_free(def);\
  newXMLIntChild (child, (xmlChar *) #field,\
          prefs->field);}
#define WRITEDOUBLEXMLENTRY(field){ \
    gchar *def = g_strdup("Holds the integer value of the user's " #field " preference");\
    gdouble value = prefs->field;\
    gchar *curname = g_strdup_printf("HistoricHarpsichordPref_%s", #field);\
    g_free(curname);\
    g_free(def);\
  newXMLDoubleChild (child, (xmlChar *) #field,\
          prefs->field);}
#define WRITEBOOLXMLENTRY(field){ \
    gchar *def = g_strdup("Holds #t or #f, the user's " #field " preference");\
    gboolean value = prefs->field;\
    gchar *curname = g_strdup_printf("HistoricHarpsichordPref_%s", #field);\
    g_free(curname);\
    g_free(def);\
  newXMLIntChild (child, (xmlChar *) #field,\
          prefs->field);}

    WRITEXMLENTRY (temperament)
    WRITEBOOLXMLENTRY (lowpitch) 
    WRITEXMLENTRY (audio_driver)
    WRITEXMLENTRY (midi_driver)

    WRITEXMLENTRY (portaudio_device)
    WRITEINTXMLENTRY (portaudio_sample_rate)
    WRITEINTXMLENTRY (portaudio_period_size)
    
    WRITEXMLENTRY (portmidi_input_device)
    WRITEXMLENTRY (portmidi_output_device)
    WRITEXMLENTRY (fluidsynth_soundfont)
    WRITEBOOLXMLENTRY (fluidsynth_reverb)
    WRITEBOOLXMLENTRY (fluidsynth_chorus)
    WRITEINTXMLENTRY (dynamic_compression)
    WRITEBOOLXMLENTRY (damping)
    
  xmlSaveFormatFile (localrc->str, doc, 1);
  xmlFreeDoc (doc);
  ret = 0;
  return ret;
}
