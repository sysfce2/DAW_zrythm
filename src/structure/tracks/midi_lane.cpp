// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/laned_track.h"

#include "./midi_lane.h"

namespace zrythm::structure::tracks
{
MidiLane::MidiLane (LanedTrackImpl<MidiLane> * track)
    : TrackLaneImpl<MidiRegion> (track)
{
}

void
MidiLane::init_after_cloning (const MidiLane &other, ObjectCloneType clone_type)

{
  ArrangerObjectOwner::copy_members_from (other, clone_type);
  TrackLaneImpl::copy_members_from (other, clone_type);
}
}