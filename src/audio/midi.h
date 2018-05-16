/*
 * midi.h
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

#ifndef MIDI_H
#define MIDI_H

#include <historicHarpsichord/historicHarpsichord.h>

#define MIDI_NOTE_OFF         0x80
#define MIDI_NOTE_ON          0x90
#define MIDI_KEY_PRESSURE     0xA0
#define MIDI_CONTROL_CHANGE   0xB0
#define MIDI_PROGRAM_CHANGE   0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_BEND       0xE0
#define SYS_EXCLUSIVE_MESSAGE1  0xF0


void generate_midi (void);
gdouble get_time (void);
gdouble get_playuntil (void);
void adjust_midi_velocity (gchar * buf, gint percent);
void add_after_touch (gchar * buf);
void change_tuning (gdouble * cents);
gdouble get_midi_on_time (GList * events);
gdouble get_midi_off_time (GList * events);



void start_playing (gchar * callback);
void pause_playing ();

void stop_playing ();
gboolean is_playing ();
gboolean is_paused ();
double get_start_time ();
double get_end_time ();
void update_start_time (double adjust);
void initialize_until_time (void);


void handle_midi_event (gchar * buf);


gboolean intercept_midi_event (gint * midi);



void change_tuning (gdouble * cents);
void toggle_paused ();
void play_adjusted_midi_event (gchar * buf);
gboolean set_midi_capture (gboolean set);
void process_midi_event (gchar * buf);

void new_midi_recording (void);
#endif // MIDI_H
