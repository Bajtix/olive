/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timelineviewghostitem.h"

#include <QPainter>

OLIVE_NAMESPACE_ENTER

TimelineViewGhostItem::TimelineViewGhostItem(QGraphicsItem *parent) :
  TimelineViewRect(parent),
  track_adj_(0),
  stream_(nullptr),
  mode_(Timeline::kNone),
  can_have_zero_length_(true),
  can_move_tracks_(true)
{
  SetInvisible(false);
}

TimelineViewGhostItem *TimelineViewGhostItem::FromBlock(Block *block, const TrackReference& track, int y, int height)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(block->in());
  ghost->SetOut(block->out());
  ghost->SetMediaIn(block->media_in());
  ghost->SetTrack(track);
  ghost->SetYCoords(y, height);
  ghost->setData(kAttachedBlock, Node::PtrToValue(block));

  switch (block->type()) {
  case Block::kClip:
    ghost->can_have_zero_length_ = false;
    break;
  case Block::kTransition:
    ghost->can_have_zero_length_ = false;
    ghost->SetCanMoveTracks(false);
    break;
  case Block::kGap:
    break;
  }

  return ghost;
}

bool TimelineViewGhostItem::CanHaveZeroLength() const
{
  return can_have_zero_length_;
}

bool TimelineViewGhostItem::GetCanMoveTracks() const
{
  return can_move_tracks_;
}

void TimelineViewGhostItem::SetCanMoveTracks(bool e)
{
  can_move_tracks_ = e;
}

void TimelineViewGhostItem::SetInvisible(bool invisible)
{
  setBrush(Qt::NoBrush);

  if (invisible) {
    setPen(Qt::NoPen);
  } else {
    setPen(QPen(Qt::yellow, 2)); // FIXME: Make customizable via CSS
  }
}

const rational &TimelineViewGhostItem::GetIn() const
{
  return in_;
}

const rational &TimelineViewGhostItem::GetOut() const
{
  return out_;
}

const rational &TimelineViewGhostItem::GetMediaIn() const
{
  return media_in_;
}

rational TimelineViewGhostItem::GetLength() const
{
  return out_ - in_;
}

rational TimelineViewGhostItem::GetAdjustedLength() const
{
  return GetAdjustedOut() - GetAdjustedIn();
}

void TimelineViewGhostItem::SetIn(const rational &in)
{
  in_ = in;

  UpdateRect();
}

void TimelineViewGhostItem::SetOut(const rational &out)
{
  out_ = out;

  UpdateRect();
}

void TimelineViewGhostItem::SetMediaIn(const rational &media_in)
{
  media_in_ = media_in;
}

void TimelineViewGhostItem::SetInAdjustment(const rational &in_adj)
{
  in_adj_ = in_adj;

  UpdateRect();
}

void TimelineViewGhostItem::SetOutAdjustment(const rational &out_adj)
{
  out_adj_ = out_adj;

  UpdateRect();
}

void TimelineViewGhostItem::SetTrackAdjustment(const int &track_adj)
{
  track_adj_ = track_adj;
}

void TimelineViewGhostItem::SetMediaInAdjustment(const rational &media_in_adj)
{
  media_in_adj_ = media_in_adj;
}

const rational &TimelineViewGhostItem::GetInAdjustment() const
{
  return in_adj_;
}

const rational &TimelineViewGhostItem::GetOutAdjustment() const
{
  return out_adj_;
}

const rational &TimelineViewGhostItem::GetMediaInAdjustment() const
{
  return media_in_adj_;
}

const int &TimelineViewGhostItem::GetTrackAdjustment() const
{
  return track_adj_;
}

rational TimelineViewGhostItem::GetAdjustedIn() const
{
  return in_ + in_adj_;
}

rational TimelineViewGhostItem::GetAdjustedOut() const
{
  return out_ + out_adj_;
}

rational TimelineViewGhostItem::GetAdjustedMediaIn() const
{
  return media_in_ + media_in_adj_;
}

TrackReference TimelineViewGhostItem::GetAdjustedTrack() const
{
  return TrackReference(track_.type(), track_.index() + track_adj_);
}

const Timeline::MovementMode &TimelineViewGhostItem::GetMode() const
{
  return mode_;
}

void TimelineViewGhostItem::SetMode(const Timeline::MovementMode &mode)
{
  mode_ = mode;
}

bool TimelineViewGhostItem::HasBeenAdjusted() const
{
  return GetInAdjustment() != 0
      || GetOutAdjustment() != 0
      || GetMediaInAdjustment() != 0
      || GetTrackAdjustment() != 0;
}

void TimelineViewGhostItem::UpdateRect()
{
  rational length = GetAdjustedOut() - GetAdjustedIn();

  setRect(0, y_, TimeToScene(length), height_ - 1);

  setPos(TimeToScene(GetAdjustedIn()), 0);
}

OLIVE_NAMESPACE_EXIT
