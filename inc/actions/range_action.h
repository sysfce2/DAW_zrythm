// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_RANGE_ACTION_H__
#define __UNDO_RANGE_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/position.h"
#include "dsp/transport.h"
#include "gui/backend/timeline_selections.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum RangeActionType
{
  RANGE_ACTION_INSERT_SILENCE,
  RANGE_ACTION_REMOVE,
} RangeActionType;

static const cyaml_strval_t range_action_type_strings[] = {
  { "Insert silence", RANGE_ACTION_INSERT_SILENCE },
  { "Remove",         RANGE_ACTION_REMOVE         },
};

typedef struct RangeAction
{
  UndoableAction parent_instance;

  /** Range positions. */
  Position start_pos;
  Position end_pos;

  /** Action type. */
  RangeActionType type;

  /** Selections before the action, starting from
   * objects intersecting with the start position and
   * ending in infinity. */
  TimelineSelections * sel_before;

  /** Selections after the action. */
  TimelineSelections * sel_after;

  /** A copy of the transport at the start of the
   * action. */
  Transport * transport;

  /** Whether this is the first run. */
  bool first_run;

} RangeAction;

void
range_action_init_loaded (RangeAction * self);

/**
 * Creates a new action.
 *
 * @param start_pos Range start.
 * @param end_pos Range end.
 */
WARN_UNUSED_RESULT UndoableAction *
range_action_new (
  RangeActionType type,
  Position *      start_pos,
  Position *      end_pos,
  GError **       error);

#define range_action_new_insert_silence(start, end, error) \
  range_action_new (RANGE_ACTION_INSERT_SILENCE, start, end, error)

#define range_action_new_remove(start, end, error) \
  range_action_new (RANGE_ACTION_REMOVE, start, end, error)

NONNULL RangeAction *
range_action_clone (const RangeAction * src);

bool
range_action_perform (
  RangeActionType type,
  Position *      start_pos,
  Position *      end_pos,
  GError **       error);

#define range_action_perform_insert_silence(start, end, error) \
  range_action_perform (RANGE_ACTION_INSERT_SILENCE, start, end, error)

#define range_action_perform_remove(start, end, error) \
  range_action_perform (RANGE_ACTION_REMOVE, start, end, error)

int
range_action_do (RangeAction * self, GError ** error);

int
range_action_undo (RangeAction * self, GError ** error);

char *
range_action_stringize (RangeAction * self);

void
range_action_free (RangeAction * self);

/**
 * @}
 */

#endif
