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

#include "viewertexteditor.h"

#include <QKeyEvent>
#include <QHBoxLayout>
#include <QScrollBar>

#include "common/qtutils.h"
#include "ui/icons/icons.h"
#include "widget/colorbutton/colorbutton.h"

namespace olive {

#define super QTextEdit

ViewerTextEditor::ViewerTextEditor(QWidget *parent) :
  super(parent)
{
  viewport()->setAutoFillBackground(false);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  horizontalScrollBar()->setEnabled(false);
  verticalScrollBar()->setEnabled(false);

  connect(qApp, &QApplication::focusChanged, this, &ViewerTextEditor::FocusChanged);
  connect(this, &QTextEdit::currentCharFormatChanged, this, &ViewerTextEditor::FormatChanged);
}

void ViewerTextEditor::ConnectToolBar(ViewerTextEditorToolBar *toolbar)
{
  connect(this, &ViewerTextEditor::destroyed, toolbar, &ViewerTextEditorToolBar::deleteLater);

  connect(toolbar, &ViewerTextEditorToolBar::FamilyChanged, this, &ViewerTextEditor::SetFamily);
  connect(toolbar, &ViewerTextEditorToolBar::SizeChanged, this, &QTextEdit::setFontPointSize);
  connect(toolbar, &ViewerTextEditorToolBar::ItalicChanged, this, &QTextEdit::setFontItalic);
  connect(toolbar, &ViewerTextEditorToolBar::UnderlineChanged, this, &QTextEdit::setFontUnderline);
  connect(toolbar, &ViewerTextEditorToolBar::AlignmentChanged, this, [this](Qt::Alignment a){
    this->setAlignment(a);

    // Ensure no buttons are checked that shouldn't be
    static_cast<ViewerTextEditorToolBar*>(sender())->SetAlignment(a);
  });

  UpdateToolBar(toolbar, this->currentCharFormat(), this->alignment());

  toolbars_.append(toolbar);
}

void ViewerTextEditor::keyPressEvent(QKeyEvent *event)
{
  super::keyPressEvent(event);

  if (event->key() == Qt::Key_Escape) {
    clearFocus();
  }
}

void ViewerTextEditor::UpdateToolBar(ViewerTextEditorToolBar *toolbar, const QTextCharFormat &f, Qt::Alignment alignment)
{
  toolbar->SetFontFamily(f.fontFamily());
  toolbar->SetFontSize(f.fontPointSize());
  toolbar->SetItalic(f.fontItalic());
  toolbar->SetUnderline(f.fontUnderline());
  toolbar->SetAlignment(alignment);
}

void ViewerTextEditor::FocusChanged(QWidget *old, QWidget *now)
{
  QWidget *test = now;

  if (!test) {
    // Ignore null focuses because that could be one of the toolbar widgets simply losing focus
    // and that would be undesirable to close the text editor from
    return;
  }

  while (test) {
    if (test == this
        || dynamic_cast<ViewerTextEditorToolBar*>(test)
        || dynamic_cast<SliderLadder*>(test)) {
      return;
    }

    test = test->parentWidget();
  }

  // If we didn't return in the loop, the user must have focused on something else
  deleteLater();
}

void ViewerTextEditor::FormatChanged(const QTextCharFormat &f)
{
  foreach (ViewerTextEditorToolBar *toolbar, toolbars_) {
    UpdateToolBar(toolbar, f, this->alignment());
  }
}

void ViewerTextEditor::SetFamily(const QString &s)
{
  QTextCharFormat f;
  f.setFontFamily(s);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
  f.setFontFamilies({s});
#endif
  mergeCurrentCharFormat(f);
}

ViewerTextEditorToolBar::ViewerTextEditorToolBar(QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);

  font_combo_ = new QFontComboBox();
  connect(font_combo_, &QFontComboBox::currentTextChanged, this, &ViewerTextEditorToolBar::FamilyChanged);
  layout->addWidget(font_combo_);

  font_sz_slider_ = new FloatSlider();
  font_sz_slider_->SetMinimum(0);
  connect(font_sz_slider_, &FloatSlider::ValueChanged, this, &ViewerTextEditorToolBar::SizeChanged);
  font_sz_slider_->SetLadderElementCount(2);
  layout->addWidget(font_sz_slider_);

  weight_combo_ = new QComboBox();
  weight_combo_->addItem(tr("Normal"));
  layout->addWidget(weight_combo_);

  italic_btn_ = new QPushButton();
  connect(italic_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::ItalicChanged);
  italic_btn_->setCheckable(true);
  italic_btn_->setIcon(icon::TextItalic);
  layout->addWidget(italic_btn_);

  underline_btn_ = new QPushButton();
  connect(underline_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::UnderlineChanged);
  underline_btn_->setCheckable(true);
  underline_btn_->setIcon(icon::TextUnderline);
  layout->addWidget(underline_btn_);

  layout->addWidget(QtUtils::CreateVerticalLine());

  align_left_btn_ = new QPushButton();
  align_left_btn_->setCheckable(true);
  align_left_btn_->setIcon(icon::TextAlignLeft);
  connect(align_left_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignLeft);});
  layout->addWidget(align_left_btn_);

  align_center_btn_ = new QPushButton();
  align_center_btn_->setCheckable(true);
  align_center_btn_->setIcon(icon::TextAlignCenter);
  connect(align_center_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignCenter);});
  layout->addWidget(align_center_btn_);

  align_right_btn_ = new QPushButton();
  align_right_btn_->setCheckable(true);
  align_right_btn_->setIcon(icon::TextAlignRight);
  connect(align_right_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignRight);});
  layout->addWidget(align_right_btn_);

  align_justify_btn_ = new QPushButton();
  align_justify_btn_->setCheckable(true);
  align_justify_btn_->setIcon(icon::TextAlignJustify);
  connect(align_justify_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignJustify);});
  layout->addWidget(align_justify_btn_);

  layout->addWidget(QtUtils::CreateVerticalLine());

  color_btn_ = new QPushButton(tr("Color"));
  layout->addWidget(color_btn_);

  setAutoFillBackground(true);
}

void ViewerTextEditorToolBar::SetAlignment(Qt::Alignment a)
{
  align_left_btn_->setChecked(a == Qt::AlignLeft);
  align_center_btn_->setChecked(a == Qt::AlignCenter);
  align_right_btn_->setChecked(a == Qt::AlignRight);
  align_justify_btn_->setChecked(a == Qt::AlignJustify);
}

void ViewerTextEditorToolBar::closeEvent(QCloseEvent *event)
{
  event->ignore();
}

void ViewerTextEditorToolBar::mousePressEvent(QMouseEvent *event)
{
  QWidget::mousePressEvent(event);

  if (event->button() == Qt::LeftButton) {
    drag_anchor_ = event->pos();
  }
}

void ViewerTextEditorToolBar::mouseMoveEvent(QMouseEvent *event)
{
  QWidget::mouseMoveEvent(event);

  if (event->buttons() & Qt::LeftButton) {
    this->move(mapToParent(QPoint(event->pos() - drag_anchor_)));
  }
}

void ViewerTextEditorToolBar::mouseReleaseEvent(QMouseEvent *event)
{
  QWidget::mouseReleaseEvent(event);
}

}
