// clang-format off
// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert.panovics at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *      Copyright 2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  ---
 */
// clang-format on

#include "dsp/channel.h"
#include "dsp/exporter.h"
#include "dsp/midi_event.h"
#include "dsp/midi_note.h"
#include "dsp/midi_region.h"
#include "dsp/region.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "io/midi_file.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include <ext/midilib/src/midifile.h>
#include <ext/midilib/src/midiutil.h>
#include <tgmath.h>

ZRegion *
midi_region_new (
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane)
{
  ZRegion * self = object_new (ZRegion);

  self->id.type = REGION_TYPE_MIDI;

  region_init (
    self, start_pos, end_pos, track_name_hash, lane_pos, idx_inside_lane);

  return self;
}

/**
 * Create a region from the chord descriptor.
 *
 * Default size will be timeline snap and default
 * notes size will be editor snap.
 */
ZRegion *
midi_region_new_from_chord_descr (
  const Position *  pos,
  ChordDescriptor * descr,
  unsigned int      track_name_hash,
  int               lane_pos,
  int               idx_inside_lane)
{
  int r_length_ticks = snap_grid_get_default_ticks (SNAP_GRID_TIMELINE);
  int mn_length_ticks = snap_grid_get_default_ticks (SNAP_GRID_EDITOR);

  /* get region end pos */
  Position r_end_pos;
  position_from_ticks (&r_end_pos, pos->ticks + (double) r_length_ticks);

  /* create region */
  ZRegion * r = midi_region_new (
    pos, &r_end_pos, track_name_hash, lane_pos, idx_inside_lane);

  /* get midi note positions */
  Position mn_pos, mn_end_pos;
  position_init (&mn_pos);
  position_from_ticks (&mn_end_pos, mn_length_ticks);

  /* create midi notes */
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (descr->notes[i])
        {
          MidiNote * mn = midi_note_new (
            &r->id, &mn_pos, &mn_end_pos, i + 36, VELOCITY_DEFAULT);
          midi_region_add_midi_note (r, mn, F_NO_PUBLISH_EVENTS);
        }
    }

  return r;
}

/**
 * Prints the MidiNotes in the Region.
 *
 * Used for debugging.
 */
void
midi_region_print_midi_notes (ZRegion * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];
      g_message ("Note at %d", i);
      midi_note_print (mn);
    }
}

/**
 * Inserts the MidiNote to the given ZRegion.
 *
 * @param idx Index to insert at.
 * @param pub_events Publish UI events or not.
 */
void
midi_region_insert_midi_note (
  ZRegion *  self,
  MidiNote * midi_note,
  int        idx,
  int        pub_events)
{
  g_return_if_fail (self->id.type == REGION_TYPE_MIDI);

  array_double_size_if_full (
    self->midi_notes, self->num_midi_notes, self->midi_notes_size, MidiNote *);
  array_insert (self->midi_notes, self->num_midi_notes, idx, midi_note);

  for (int i = idx; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];
      midi_note_set_region_and_index (mn, self, i);
    }

  if (pub_events)
    {
      EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, midi_note);
    }
}

/**
 * Returns the midi note with the given pitch from
 * the unended notes.
 *
 * Used when recording.
 *
 * @param pitch The pitch. If -1, it returns any
 *   unended note. This is useful when the loop
 *   point is met and we want to end them all.
 */
MidiNote *
midi_region_pop_unended_note (ZRegion * self, int pitch)
{
  MidiNote * match = NULL;
  for (int i = 0; i < self->num_unended_notes; i++)
    {
      MidiNote * mn = self->unended_notes[i];
      if (pitch == -1 || mn->val == pitch)
        {
          match = mn;
          break;
        }
    }

  if (match)
    {
      /* pop it from the array */
      array_delete (self->unended_notes, self->num_unended_notes, match);

      return match;
    }

  return NULL;
}

/**
 * Gets first midi note
 */
MidiNote *
midi_region_get_first_midi_note (ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0; i < region->num_midi_notes; i++)
    {
      ArrangerObject * cur_mn_obj = (ArrangerObject *) region->midi_notes[i];
      ArrangerObject * result_obj = (ArrangerObject *) result;
      if (!result || result_obj->end_pos.ticks > cur_mn_obj->end_pos.ticks)
        {
          result = region->midi_notes[i];
        }
    }
  return result;
}
/**
 * Gets last midi note
 */
MidiNote *
midi_region_get_last_midi_note (ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0; i < region->num_midi_notes; i++)
    {
      if (
        !result
        || ((ArrangerObject *) result)->end_pos.ticks
             < ((ArrangerObject *) region->midi_notes[i])->end_pos.ticks)
        {
          result = region->midi_notes[i];
        }
    }
  return result;
}

/**
 * Gets highest midi note
 */
MidiNote *
midi_region_get_highest_midi_note (ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0; i < region->num_midi_notes; i++)
    {
      if (!result || result->val < region->midi_notes[i]->val)
        {
          result = region->midi_notes[i];
        }
    }
  return result;
}

/**
 * Gets lowest midi note
 */
MidiNote *
midi_region_get_lowest_midi_note (ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0; i < region->num_midi_notes; i++)
    {
      if (!result || result->val > region->midi_notes[i]->val)
        {
          result = region->midi_notes[i];
        }
    }

  return result;
}

/**
 * Removes the MIDI note from the Region.
 *
 * @param free Also free the MidiNote.
 * @param pub_event Publish an event.
 */
void
midi_region_remove_midi_note (
  ZRegion *  region,
  MidiNote * midi_note,
  int        free,
  int        pub_event)
{
  if (MA_SELECTIONS)
    {
      arranger_selections_remove_object (
        (ArrangerSelections *) MA_SELECTIONS, (ArrangerObject *) midi_note);
    }

  array_delete (region->midi_notes, region->num_midi_notes, midi_note);

  for (int i = 0; i < region->num_midi_notes; i++)
    {
      midi_note_set_region_and_index (region->midi_notes[i], region, i);
    }

  if (free)
    {
      arranger_object_free ((ArrangerObject *) midi_note);
    }

  if (pub_event)
    {
      EVENTS_PUSH (ET_ARRANGER_OBJECT_REMOVED, ARRANGER_OBJECT_TYPE_MIDI_NOTE);
    }
}

/**
 * Creates a MIDI region from the given MIDI
 * file path, starting at the given Position.
 *
 * @param idx The index of this track, starting from
 *   0. This will be sequential, ie, if idx 1 is
 *   requested and the MIDI file only has tracks
 *   5 and 7, it will use track 7.
 */
ZRegion *
midi_region_new_from_midi_file (
  const Position * start_pos,
  const char *     abs_path,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane,
  int              idx)
{
  g_message ("%s: reading from %s...", __func__, abs_path);

  ZRegion *        self = object_new (ZRegion);
  ArrangerObject * r_obj = (ArrangerObject *) self;

  self->id.type = REGION_TYPE_MIDI;

  MIDI_FILE * mf = midiFileOpen (abs_path);
  g_return_val_if_fail (mf, NULL);

  char         str[128];
  char         txt[60000];
  int          ev;
  MIDI_MSG     msg;
  unsigned int j;
  Position     pos, global_pos;
  MidiNote *   mn;
  double       ticks;

  Position end_pos;
  position_from_ticks (&end_pos, start_pos->ticks + 1);
  region_init (
    self, start_pos, &end_pos, track_name_hash, lane_pos, idx_inside_lane);

  midiReadInitMessage (&msg);
  int    num_tracks = midiReadGetNumTracks (mf);
  double ppqn = (double) midiFileGetPPQN (mf);
  double transport_ppqn = transport_get_ppqn (TRANSPORT);

  int actual_iter = 0;

  for (int i = 0; i < num_tracks; i++)
    {
      if (!midi_file_track_has_data (abs_path, i))
        {
          continue;
        }

      if (actual_iter != idx)
        {
          actual_iter++;
          continue;
        }

      g_message ("%s: reading MIDI Track %d", __func__, i);
      g_return_val_if_fail (i < 1000, NULL);
      while (midiReadGetNextMessage (mf, i, &msg))
        {
          /* convert time to zrythm time */
          ticks = ((double) msg.dwAbsPos * transport_ppqn) / ppqn;
          position_from_ticks (&pos, ticks);
          position_from_ticks (&global_pos, r_obj->pos.ticks + ticks);
          g_debug ("dwAbsPos: %d ", msg.dwAbsPos);

          int bars = position_get_bars (&pos, true);
          if (ZRYTHM_HAVE_UI && bars > TRANSPORT->total_bars - 8)
            {
              transport_update_total_bars (
                TRANSPORT, bars + 8, F_PUBLISH_EVENTS);
            }

          if (msg.bImpliedMsg)
            {
              ev = msg.iImpliedMsg;
            }
          else
            {
              ev = msg.iType;
            }

          if (muGetMIDIMsgName (str, ev))
            {
              g_debug ("MIDI msg name: %s", str);
            }
          switch (ev)
            {
            case msgNoteOff:
handle_note_off:
              g_debug (
                "Note off at %d "
                "[ch %d pitch %d]",
                msg.dwAbsPos, msg.MsgData.NoteOff.iChannel,
                msg.MsgData.NoteOff.iNote);
              mn =
                midi_region_pop_unended_note (self, msg.MsgData.NoteOff.iNote);
              if (mn)
                {
                  arranger_object_end_pos_setter ((ArrangerObject *) mn, &pos);
                }
              else
                {
                  g_message (
                    "Found a Note off event without "
                    "a corresponding Note on. "
                    "Skipping...");
                }
              break;
            case msgNoteOn:
              /* 0 velocity is a note off */
              if (msg.MsgData.NoteOn.iVolume == 0)
                {
                  msg.MsgData.NoteOff.iChannel = msg.MsgData.NoteOn.iChannel;
                  msg.MsgData.NoteOff.iNote = msg.MsgData.NoteOn.iNote;
                  goto handle_note_off;
                }

              g_debug (
                "Note on at %d "
                "[ch %d pitch %d vel %d]",
                msg.dwAbsPos, msg.MsgData.NoteOn.iChannel,
                msg.MsgData.NoteOn.iNote, msg.MsgData.NoteOn.iVolume);
              midi_region_start_unended_note (
                self, &pos, NULL, msg.MsgData.NoteOn.iNote,
                msg.MsgData.NoteOn.iVolume, 0);
              break;
            case msgNoteKeyPressure:
              muGetNameFromNote (str, msg.MsgData.NoteKeyPressure.iNote);
              g_debug (
                "(%.2d) %s %d", msg.MsgData.NoteKeyPressure.iChannel, str,
                msg.MsgData.NoteKeyPressure.iPressure);
              break;
            case msgSetParameter:
              muGetControlName (str, msg.MsgData.NoteParameter.iControl);
              g_debug (
                "(%.2d) %s -> %d", msg.MsgData.NoteParameter.iChannel, str,
                msg.MsgData.NoteParameter.iParam);
              break;
            case msgSetProgram:
              muGetInstrumentName (str, msg.MsgData.ChangeProgram.iProgram);
              g_debug ("(%.2d) %s", msg.MsgData.ChangeProgram.iChannel, str);
              break;
            case msgChangePressure:
              muGetControlName (str, msg.MsgData.ChangePressure.iPressure);
              g_debug ("(%.2d) %s", msg.MsgData.ChangePressure.iChannel, str);
              break;
            case msgSetPitchWheel:
              g_debug (
                "(%.2d) %d", msg.MsgData.PitchWheel.iChannel,
                msg.MsgData.PitchWheel.iPitch);
              break;
            case msgMetaEvent:
              g_debug ("---- meta events");
              switch (msg.MsgData.MetaEvent.iType)
                {
                case metaMIDIPort:
                  g_debug (
                    "MIDI Port = %d", msg.MsgData.MetaEvent.Data.iMIDIPort);
                  break;
                case metaSequenceNumber:
                  g_debug (
                    "Sequence Number = %d",
                    msg.MsgData.MetaEvent.Data.iSequenceNumber);
                  break;
                case metaTextEvent:
                  g_debug ("Text = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
                  break;
                case metaCopyright:
                  g_debug (
                    "Copyright = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
                  break;
                case metaTrackName:
                  {
                    char tmp[6000];
                    strncpy (
                      tmp, (char *) msg.MsgData.MetaEvent.Data.Text.pData,
                      msg.iMsgSize - 3);
                    tmp[msg.iMsgSize - 3] = '\0';
                    arranger_object_set_name (
                      (ArrangerObject *) self, tmp, F_NO_PUBLISH_EVENTS);
                    g_warn_if_fail (self->name);
                    g_message (
                      "[data sz %d] Track name = '%s'", msg.iMsgSize - 3,
                      self->name);
                  }
                  break;
                case metaInstrument:
                  g_message (
                    "Instrument = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
                  break;
                case metaLyric:
                  g_message (
                    "Lyric = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
                  break;
                case metaMarker:
                  g_message (
                    "Marker = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
                  break;
                case metaCuePoint:
                  g_message (
                    "Cue point = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
                  break;
                case metaEndSequence:
                  g_message ("End Sequence");
                  if (position_is_equal (&pos, start_pos))
                    {
                      /* this is an empty track,
                       * so return NULL
                       * instead */
                      return NULL;
                    }
                  arranger_object_end_pos_setter (r_obj, &global_pos);
                  arranger_object_loop_end_pos_setter (r_obj, &global_pos);
                  break;
                case metaSetTempo:
                  g_message ("tempo %d", msg.MsgData.MetaEvent.Data.Tempo.iBPM);
                  break;
                case metaSMPTEOffset:
                  g_message (
                    "SMPTE offset = %d:%d:%d.%d %d",
                    msg.MsgData.MetaEvent.Data.SMPTE.iHours,
                    msg.MsgData.MetaEvent.Data.SMPTE.iMins,
                    msg.MsgData.MetaEvent.Data.SMPTE.iSecs,
                    msg.MsgData.MetaEvent.Data.SMPTE.iFrames,
                    msg.MsgData.MetaEvent.Data.SMPTE.iFF);
                  break;
                case metaTimeSig:
                  g_message (
                    "Time sig = %d/%d", msg.MsgData.MetaEvent.Data.TimeSig.iNom,
                    msg.MsgData.MetaEvent.Data.TimeSig.iDenom
                      / MIDI_NOTE_CROCHET);
                  break;
                case metaKeySig:
                  if (
                    muGetKeySigName (str, msg.MsgData.MetaEvent.Data.KeySig.iKey))
                    g_message ("Key sig = %s", str);
                  break;
                case metaSequencerSpecific:
                  g_message ("Sequencer specific = ");
                  /*HexList(msg.MsgData.MetaEvent.Data.Sequencer.pData,
                   * msg.MsgData.MetaEvent.Data.Sequencer.iSize);*/
                  break;
                }
              break;

            case msgSysEx1:
            case msgSysEx2:
              g_message ("Sysex = ");
              /*HexList(msg.MsgData.SysEx.pData, msg.MsgData.SysEx.iSize);*/
              break;
            }

          /* print the hex */
          if (
            ev == msgSysEx1
            || (ev == msgMetaEvent && msg.MsgData.MetaEvent.iType == metaSequencerSpecific))
            {
              /* Already done a hex dump */
            }
          else
            {
              char tmp[100];
              strcpy (txt, "[");
              if (msg.bImpliedMsg)
                {
                  sprintf (tmp, "%.2x!", msg.iImpliedMsg);
                  strcat (txt, tmp);
                }
              for (j = 0; j < msg.iMsgSize; j++)
                {
                  sprintf (tmp, " %.2x ", msg.data[j]);
                  strcat (txt, tmp);
                }
              strcat (txt, "]");
              g_debug ("%s", txt);
            }
        }

      if (actual_iter == idx)
        {
          break;
        }
    }

  midiReadFreeMessage (&msg);
  midiFileClose (mf);

  if (self->num_unended_notes != 0)
    {
      g_warning ("unended notes found: %d", self->num_unended_notes);

      double length =
        arranger_object_get_length_in_ticks ((ArrangerObject *) self);
      position_from_ticks (&end_pos, length);

      while (self->num_unended_notes > 0)
        {
          mn = midi_region_pop_unended_note (self, -1);
          arranger_object_end_pos_setter ((ArrangerObject *) mn, &end_pos);
        }
    }

  g_return_val_if_fail (
    position_is_before (&self->base.pos, &self->base.end_pos), NULL);

  g_message ("%s: done ~ %d MIDI notes read", __func__, self->num_midi_notes);

  return self;
}

/**
 * Starts an unended note with the given pitch and
 * velocity and adds it to \ref ZRegion.midi_notes.
 *
 * If another note exists with the same pitch, this
 * will be ignored.
 *
 * @param end_pos If this is NULL, it will be set to
 *   1 tick after the start_pos.
 */
void
midi_region_start_unended_note (
  ZRegion *  self,
  Position * start_pos,
  Position * _end_pos,
  int        pitch,
  int        vel,
  int        pub_events)
{
  g_return_if_fail (self && start_pos);

  /* set end pos */
  Position end_pos;
  if (_end_pos)
    {
      position_set_to_pos (&end_pos, _end_pos);
    }
  else
    {
      position_set_to_pos (&end_pos, start_pos);
      position_add_ticks (&end_pos, 1);
    }

  MidiNote * mn = midi_note_new (&self->id, start_pos, &end_pos, pitch, vel);
  midi_region_add_midi_note (self, mn, pub_events);

  /* add to unended notes */
  array_append (self->unended_notes, self->num_unended_notes, mn);
}

/**
 * Exports the ZRegion to an existing MIDI file
 * instance.
 *
 * This must only be called when exporting single
 * regions.
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 * @param lanes_as_tracks Export lanes as separate
 *   tracks (only possible with MIDI type 1). This
 *   will calculate a unique MIDI track number for
 *   the region's lane.
 * @param use_track_or_lane_pos Whether to use the
 *   track/lane position in the MIDI data. The
 *   MIDI track will be set to 1 if false.
 */
void
midi_region_write_to_midi_file (
  const ZRegion * self,
  MIDI_FILE *     mf,
  const bool      add_region_start,
  bool            export_full)
{
  MidiEvents * events = midi_events_new ();
  midi_region_add_events (
    self, events, NULL, NULL, add_region_start, export_full);

  midiFileSetTracksDefaultChannel (mf, 1, MIDI_CHANNEL_1);
  midiTrackAddText (mf, 1, textTrackName, self->name);

  midi_events_write_to_midi_file (events, mf, 1);
  object_free_w_func_and_null (midi_events_free, events);
}

/**
 * Exports the ZRegion to a specified MIDI file.
 *
 * @param full_path Absolute path to the MIDI file.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
void
midi_region_export_to_midi_file (
  const ZRegion * self,
  const char *    full_path,
  int             midi_version,
  const bool      export_full)
{
  MIDI_FILE * mf;

  if ((mf = midiFileCreate (full_path, TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (
        mf, 1, (int) tempo_track_get_current_bpm (P_TEMPO_TRACK));

      /* All data is written out to _tracks_ not
       * channels. We therefore
      ** set the current channel before writing
      data out. Channel assignments
      ** can change any number of times during the
      file, and affect all
      ** tracks messages until it is changed. */
      midiFileSetTracksDefaultChannel (mf, 1, MIDI_CHANNEL_1);

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar,
        math_round_double_to_signed_32 (TRANSPORT->ticks_per_beat));

      midi_region_write_to_midi_file (self, mf, false, export_full);

      midiFileClose (mf);
    }
}

/**
 * Returns the MIDI channel that this region should
 * be played on, starting from 1.
 */
uint8_t
midi_region_get_midi_ch (const ZRegion * self)
{
  g_return_val_if_fail (self, 1);
  uint8_t     ret;
  TrackLane * lane = region_get_lane (self);
  g_return_val_if_fail (lane, 1);
  if (lane->midi_ch > 0)
    ret = lane->midi_ch;
  else
    {
      Track * track = track_lane_get_track (lane);
      g_return_val_if_fail (track, 1);
      ret = track->midi_ch;
    }

  g_return_val_if_fail (ret > 0, 1);

  return ret;
}

/**
 * Returns whether the given note is not muted and starts within any
 * playable part of the region.
 */
bool
midi_region_is_note_playable (const ZRegion * self, const MidiNote * midi_note)
{
  ArrangerObject * self_obj = (ArrangerObject *) self;

  ArrangerObject * mn_obj = (ArrangerObject *) midi_note;

  if (arranger_object_get_muted (mn_obj, false))
    {
      return false;
    }

  if (
    !position_is_between (
      &mn_obj->pos, &self_obj->loop_start_pos, &self_obj->loop_end_pos)
    && !position_is_between (
      &mn_obj->pos, &self_obj->clip_start_pos, &self_obj->loop_start_pos))
    {
      return false;
    }

  return true;
}

/**
 * Set positions to the exact values in the export
 * region as it is played inside Zrythm.
 *
 * @param[in,out] start_pos start position of the event
 * @param[in,out] end_pos end position of the event
 * @param repeat_index repetition counter for loop offset
 */
static void
get_note_positions_in_export (
  const ZRegion * self,
  Position *      start_pos,
  Position *      end_pos,
  int             repeat_index)
{
  ArrangerObject * self_obj = (ArrangerObject *) self;

  double loop_length_in_ticks =
    arranger_object_get_loop_length_in_ticks (self_obj);
  POSITION_INIT_ON_STACK (export_start_pos);
  POSITION_INIT_ON_STACK (export_end_pos);
  position_add_ticks (
    &export_end_pos, arranger_object_get_length_in_ticks (self_obj));

  position_set_to_pos (end_pos, position_min (&self_obj->loop_end_pos, end_pos));

  if (position_is_before (start_pos, &self_obj->clip_start_pos))
    {
      ++repeat_index;
    }

  position_add_ticks (
    start_pos,
    loop_length_in_ticks * repeat_index
      - position_to_ticks (&self_obj->clip_start_pos));
  position_add_ticks (
    end_pos,
    loop_length_in_ticks * repeat_index
      - position_to_ticks (&self_obj->clip_start_pos));
  position_set_to_pos (start_pos, position_max (start_pos, &export_start_pos));
  position_set_to_pos (end_pos, position_min (end_pos, &export_end_pos));
}

/**
 * Returns if the given positions are in a given
 * region as it is played inside Zrythm.
 *
 * @param offset_in_ticks Offset value if note is
 * repeated inside a loop
 */
static bool
is_note_export_start_pos_in_full_region (
  const ZRegion *  self,
  const Position * start_pos)
{
  ArrangerObject * self_obj = (ArrangerObject *) self;

  POSITION_INIT_ON_STACK (export_start_pos);
  POSITION_INIT_ON_STACK (export_end_pos);
  position_add_ticks (
    &export_end_pos, arranger_object_get_length_in_ticks (self_obj));

  return position_is_between (start_pos, &export_start_pos, &export_end_pos);
}

/**
 * Adds the contents of the region converted into events.
 *
 * @param add_region_start Add the region start offset to the
 *   positions.
 * @param export_full Traverse loops and export the MIDI file
 *   as it would be played inside Zrythm. If this is 0, only
 *   the original region (from true start to true end) is
 *   exported.
 * @param start Events before this (global) position will be
 *   skipped.
 * @param end Events after this (global) position will be
 *   skipped.
 */
void
midi_region_add_events (
  const ZRegion *  self,
  MidiEvents *     events,
  const Position * start,
  const Position * end,
  const bool       add_region_start,
  const bool       full)
{
  ArrangerObject * self_obj = (ArrangerObject *) self;

  double region_start = 0;
  if (add_region_start)
    region_start = self_obj->pos.ticks;

  double loop_length_in_ticks =
    arranger_object_get_loop_length_in_ticks (self_obj);
  int number_of_loop_repeats = (int) ceil (
    (arranger_object_get_length_in_ticks (self_obj)
     - position_to_ticks (&self_obj->loop_start_pos)
     + position_to_ticks (&self_obj->clip_start_pos))
    / loop_length_in_ticks);

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote *       mn = self->midi_notes[i];
      ArrangerObject * mn_obj = (ArrangerObject *) mn;

      if (full && !midi_region_is_note_playable (self, mn))
        {
          continue;
        }

      int  repeat_counter = 0;
      bool write_only_once = true;

      do
        {
          Position mn_pos = mn_obj->pos;
          Position mn_end_pos = mn_obj->end_pos;

          if (full)
            {
              if (
                position_is_between (
                  &mn_obj->pos, &self_obj->loop_start_pos,
                  &self_obj->loop_end_pos))
                {
                  write_only_once = false;
                }

              get_note_positions_in_export (
                self, &mn_pos, &mn_end_pos, repeat_counter);

              if (!is_note_export_start_pos_in_full_region (self, &mn_pos))
                {
                  continue;
                }
            }

          double note_global_start_ticks =
            position_to_ticks (&mn_pos) + region_start;
          double note_global_end_ticks =
            position_to_ticks (&mn_end_pos) + region_start;
          bool write_note = true;
          if (start && note_global_end_ticks < start->ticks)
            write_note = false;
          if (end && note_global_start_ticks > end->ticks)
            write_note = false;

          if (write_note)
            {
              if (start)
                {
                  note_global_start_ticks -= start->ticks;
                  note_global_end_ticks -= start->ticks;
                }
              midi_events_add_note_on (
                events, 1, mn->val, mn->vel->vel,
                (midi_time_t) (note_global_start_ticks), F_NOT_QUEUED);
              midi_events_add_note_off (
                events, 1, mn->val, (midi_time_t) (note_global_end_ticks),
                F_NOT_QUEUED);
            }
        }
      while (++repeat_counter < number_of_loop_repeats && !write_only_once);
    }

  midi_events_sort (events, F_NOT_QUEUED);
}

/**
 * Sends MIDI note off events or an "all notes off" event at the current time.
 *
 * @param is_note_off_for_loop_or_region_end Whether this is called to send
 *   note off events for notes at the loop end/region end boundary (as
 *   opposed to a transport loop boundary).
 *   If true, separate MIDI note off events will be sent for notes at the
 *   border. Otherwise, a single all notes off event will be sent.
 */
OPTIMIZE_O3
REALTIME
static void
send_note_offs (
  ZRegion *                           self,
  MidiEvents *                        midi_events,
  const EngineProcessTimeInfo * const time_nfo,
  bool                                is_note_off_for_loop_or_region_end)
{
  midi_byte_t channel = 1;
  if (self->id.type == REGION_TYPE_MIDI)
    {
      channel = midi_region_get_midi_ch (self);
    }
  else if (self->id.type == REGION_TYPE_CHORD)
    {
      /* FIXME set channel */
    }

  /* -1 to send event 1 sample before the end point */
  const midi_time_t midi_time_for_note_off =
    (midi_time_t) ((time_nfo->local_offset + time_nfo->nframes) - 1);
  const signed_frame_t frame_for_note_off =
    (signed_frame_t) (time_nfo->g_start_frame_w_offset + time_nfo->nframes) - 1;
  if (is_note_off_for_loop_or_region_end && self->id.type == REGION_TYPE_MIDI)
    {
      for (int i = 0; i < self->num_midi_notes; i++)
        {
          MidiNote * mn = self->midi_notes[i];
          if (midi_note_hit (mn, frame_for_note_off))
            {
              midi_events_add_note_off (
                midi_events, channel, mn->val, midi_time_for_note_off, F_QUEUED);
            }
        }
    }
  else
    {
      midi_events_add_all_notes_off (
        midi_events, channel, midi_time_for_note_off, F_QUEUED);
    }
}

void
midi_region_fill_midi_events (
  ZRegion *                           self,
  const EngineProcessTimeInfo * const time_nfo,
  bool                                note_off_at_end,
  bool                                is_note_off_for_loop_or_region_end,
  MidiEvents *                        midi_events)
{
  ArrangerObject * r_obj = (ArrangerObject *) self;
  Track *          track = arranger_object_get_track (r_obj);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  /* send note offs if needed */
  if (note_off_at_end)
    {
      send_note_offs (
        self, midi_events, time_nfo, is_note_off_for_loop_or_region_end);
    }

  const signed_frame_t r_local_pos = region_timeline_frames_to_local (
    self, (signed_frame_t) time_nfo->g_start_frame_w_offset, F_NORMALIZE);

#if 0
  if (time_nfo->g_start_frame_w_offset == 0)
    {
      g_debug (
        "%s: fill midi events - g start %ld - "
        "local start %"PRIu32" - time_nfo->nframes %"PRIu32" - "
        "notes off at end %u - "
        "r local pos %ld",
        __func__, time_nfo->g_start_frame_w_offset,
        time_nfo->local_offset, time_nfo->nframes,
        note_off_at_end, r_local_pos);
    }
#endif

  /* go through each note */
  int num_objs =
    track->type == TRACK_TYPE_CHORD
      ? self->num_chord_objects
      : self->num_midi_notes;
  for (int i = 0; i < num_objs; i++)
    {
      ArrangerObject *  mn_obj = NULL;
      MidiNote *        mn = NULL;
      ChordObject *     co = NULL;
      ChordDescriptor * descr = NULL;
      if (track->type == TRACK_TYPE_CHORD)
        {
          co = self->chord_objects[i];
          descr = chord_object_get_chord_descriptor (co);
          mn_obj = (ArrangerObject *) co;
        }
      else
        {
          mn = self->midi_notes[i];
          mn_obj = (ArrangerObject *) mn;
        }
      if (arranger_object_get_muted (mn_obj, false))
        {
          continue;
        }

      /* if object starts inside the current
       * range */
      if (
        mn_obj->pos.frames >= 0 && mn_obj->pos.frames >= r_local_pos
        && mn_obj->pos.frames < r_local_pos + (signed_frame_t) time_nfo->nframes)
        {
          midi_time_t _time =
            (midi_time_t) (time_nfo->local_offset
                           + (mn_obj->pos.frames - r_local_pos));
          /*g_message ("normal note on at %u", time);*/

          if (mn)
            {
              midi_events_add_note_on (
                midi_events, midi_region_get_midi_ch (self), mn->val,
                mn->vel->vel, _time, F_QUEUED);
            }
          else if (co)
            {
              midi_events_add_note_ons_from_chord_descr (
                midi_events, descr, 1, VELOCITY_DEFAULT, _time, F_QUEUED);
            }
        }

      signed_frame_t mn_obj_end_frames =
        (track->type == TRACK_TYPE_CHORD
           ? math_round_double_to_signed_frame_t (
               mn_obj->pos.frames
               + TRANSPORT->ticks_per_beat * AUDIO_ENGINE->frames_per_tick)
           : mn_obj->end_pos.frames);

      /* if note ends within the cycle */
      if (
        mn_obj_end_frames >= r_local_pos
        && (mn_obj_end_frames <= (r_local_pos + time_nfo->nframes)))
        {
          midi_time_t _time =
            (midi_time_t) (time_nfo->local_offset
                           + (mn_obj_end_frames - r_local_pos));

          /* note actually ends 1 frame before
           * the end point, not at the end
           * point */
          if (_time > 0)
            {
              _time--;
            }

#if 0
          if (time_nfo->g_start_frame_w_offset == 0)
            {
              g_debug (
                "note ends within cycle (end frames %ld - note off time: %u",
                mn_obj_end_frames, _time);
            }
#endif

          if (mn)
            {
              midi_events_add_note_off (
                midi_events, midi_region_get_midi_ch (self), mn->val, _time,
                F_QUEUED);
            }
          else if (co)
            {
              for (int l = 0; l < CHORD_DESCRIPTOR_MAX_NOTES; l++)
                {
                  if (descr->notes[l])
                    {
                      midi_events_add_note_off (
                        midi_events, 1, l + 36, _time, F_QUEUED);
                    }
                }
            }
        }
    } /* foreach midi note */
}

/**
 * Fills in the array with all the velocities in
 * the project that are within or outside the
 * range given.
 *
 * @param inside Whether to find velocities inside
 *   the range (1) or outside (0).
 */
void
midi_region_get_velocities_in_range (
  const ZRegion *  self,
  const Position * start_pos,
  const Position * end_pos,
  Velocity ***     velocities,
  int *            num_velocities,
  size_t *         velocities_size,
  int              inside)
{
  Position global_start_pos;
  for (int k = 0; k < self->num_midi_notes; k++)
    {
      MidiNote * mn = self->midi_notes[k];
      midi_note_get_global_start_pos (mn, &global_start_pos);

#define ADD_VELOCITY \
  array_double_size_if_full ( \
    *velocities, *num_velocities, *velocities_size, Velocity *); \
  (*velocities)[(*num_velocities)++] = mn->vel

      if (
        inside && position_is_after_or_equal (&global_start_pos, start_pos)
        && position_is_before_or_equal (&global_start_pos, end_pos))
        {
          ADD_VELOCITY;
        }
      else if (
        !inside
        && (position_is_before (&global_start_pos, start_pos) || position_is_after (&global_start_pos, end_pos)))
        {
          ADD_VELOCITY;
        }
#undef ADD_VELOCITY
    }
}

/**
 * Frees members only but not the midi region itself.
 *
 * Regions should be free'd using region_free().
 */
void
midi_region_free_members (ZRegion * self)
{
  g_return_if_fail (IS_REGION (self));

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      arranger_object_free ((ArrangerObject *) self->midi_notes[i]);
    }
}
