/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "cropdistortnode.h"

#include "common/util.h"
#include "core.h"
#include "widget/slider/floatslider.h"

namespace olive {

const QString CropDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString CropDistortNode::kLeftInput = QStringLiteral("left_in");
const QString CropDistortNode::kTopInput = QStringLiteral("top_in");
const QString CropDistortNode::kRightInput = QStringLiteral("right_in");
const QString CropDistortNode::kBottomInput = QStringLiteral("bottom_in");
const QString CropDistortNode::kFeatherInput = QStringLiteral("feather_in");

CropDistortNode::CropDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  CreateCropSideInput(kLeftInput);
  CreateCropSideInput(kTopInput);
  CreateCropSideInput(kRightInput);
  CreateCropSideInput(kBottomInput);

  AddInput(kFeatherInput, NodeValue::kFloat, 0.0);
  SetInputProperty(kFeatherInput, QStringLiteral("min"), 0.0);

  // Initiate gizmos
  point_gizmo_[kGizmoScaleTopLeft] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleTopLeft]->AddInput(NodeInput(this, kLeftInput));
  point_gizmo_[kGizmoScaleTopLeft]->AddInput(NodeInput(this, kTopInput));

  point_gizmo_[kGizmoScaleTopCenter] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleTopCenter]->AddInput(NodeInput(this, kTopInput));

  point_gizmo_[kGizmoScaleTopRight] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleTopRight]->AddInput(NodeInput(this, kRightInput));
  point_gizmo_[kGizmoScaleTopRight]->AddInput(NodeInput(this, kTopInput));

  point_gizmo_[kGizmoScaleBottomLeft] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleBottomLeft]->AddInput(NodeInput(this, kLeftInput));
  point_gizmo_[kGizmoScaleBottomLeft]->AddInput(NodeInput(this, kBottomInput));

  point_gizmo_[kGizmoScaleBottomCenter] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleBottomCenter]->AddInput(NodeInput(this, kBottomInput));

  point_gizmo_[kGizmoScaleBottomRight] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleBottomRight]->AddInput(NodeInput(this, kRightInput));
  point_gizmo_[kGizmoScaleBottomRight]->AddInput(NodeInput(this, kBottomInput));

  point_gizmo_[kGizmoScaleCenterLeft] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleCenterLeft]->AddInput(NodeInput(this, kLeftInput));

  point_gizmo_[kGizmoScaleCenterRight] = AddGizmo<PointGizmo>();
  point_gizmo_[kGizmoScaleCenterRight]->AddInput(NodeInput(this, kRightInput));

  poly_gizmo_ = AddGizmo<PolygonGizmo>();;
  poly_gizmo_->AddInput(NodeInput(this, kLeftInput));
  poly_gizmo_->AddInput(NodeInput(this, kTopInput));
  poly_gizmo_->AddInput(NodeInput(this, kRightInput));
  poly_gizmo_->AddInput(NodeInput(this, kBottomInput));
}

void CropDistortNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kLeftInput, tr("Left"));
  SetInputName(kTopInput, tr("Top"));
  SetInputName(kRightInput, tr("Right"));
  SetInputName(kBottomInput, tr("Bottom"));
  SetInputName(kFeatherInput, tr("Feather"));
}

void CropDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  if (!job.GetValue(kTextureInput).data().isNull()) {
    if (!qIsNull(job.GetValue(kLeftInput).data().toDouble())
        || !qIsNull(job.GetValue(kRightInput).data().toDouble())
        || !qIsNull(job.GetValue(kTopInput).data().toDouble())
        || !qIsNull(job.GetValue(kBottomInput).data().toDouble())) {
      table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table->Push(NodeValue::kTexture, job.GetValue(kTextureInput).data(), this);
    }
  }
}

ShaderCode CropDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/crop.frag")));
}

void CropDistortNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  const QVector2D &resolution = globals.resolution();

  double left_pt = resolution.x() * row[kLeftInput].data().toDouble();
  double top_pt = resolution.y() * row[kTopInput].data().toDouble();
  double right_pt = resolution.x() * (1.0 - row[kRightInput].data().toDouble());
  double bottom_pt = resolution.y() * (1.0 - row[kBottomInput].data().toDouble());
  double center_x_pt = mid(left_pt, right_pt);
  double center_y_pt = mid(top_pt, bottom_pt);

  point_gizmo_[kGizmoScaleTopLeft]->SetPoint(QPointF(left_pt, top_pt));
  point_gizmo_[kGizmoScaleTopCenter]->SetPoint(QPointF(center_x_pt, top_pt));
  point_gizmo_[kGizmoScaleTopRight]->SetPoint(QPointF(right_pt, top_pt));
  point_gizmo_[kGizmoScaleBottomLeft]->SetPoint(QPointF(left_pt, bottom_pt));
  point_gizmo_[kGizmoScaleBottomCenter]->SetPoint(QPointF(center_x_pt, bottom_pt));
  point_gizmo_[kGizmoScaleBottomRight]->SetPoint(QPointF(right_pt, bottom_pt));
  point_gizmo_[kGizmoScaleCenterLeft]->SetPoint(QPointF(left_pt, center_y_pt));
  point_gizmo_[kGizmoScaleCenterRight]->SetPoint(QPointF(right_pt, center_y_pt));

  poly_gizmo_->SetPolygon(QRectF(left_pt, top_pt, right_pt - left_pt, bottom_pt - top_pt));
}

void CropDistortNode::GizmoDragMove(double x_diff, double y_diff, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  QVector2D res = gizmo->GetGlobals().resolution();
  x_diff /= res.x();
  y_diff /= res.y();

  for (int j=0; j<gizmo->GetDraggers().size(); j++) {
    NodeInputDragger& i = gizmo->GetDraggers()[j];
    double s = i.GetStartValue().toDouble();
    if (i.GetInput().input().input() == kLeftInput) {
      i.Drag(s + x_diff);
    } else if (i.GetInput().input().input() == kTopInput) {
      i.Drag(s + y_diff);
    } else if (i.GetInput().input().input() == kRightInput) {
      i.Drag(s - x_diff);
    } else if (i.GetInput().input().input() == kBottomInput) {
      i.Drag(s - y_diff);
    }
  }
}

void CropDistortNode::CreateCropSideInput(const QString &id)
{
  AddInput(id, NodeValue::kFloat, 0.0);
  SetInputProperty(id, QStringLiteral("min"), 0.0);
  SetInputProperty(id, QStringLiteral("max"), 1.0);
  SetInputProperty(id, QStringLiteral("view"), FloatSlider::kPercentage);
}

template<typename T>
T *CropDistortNode::AddGizmo()
{
  T *gizmo = new T(this);
  gizmo->SetDragValueBehavior(PointGizmo::kDeltaFromStart);
  connect(gizmo, &DraggableGizmo::HandleMovement, this, &CropDistortNode::GizmoDragMove);
  return gizmo;
}

}
