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

#include <QAbstractTextDocumentLayout>
#include <QColorDialog>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextFrame>

#include "common/qtutils.h"
#include "ui/icons/icons.h"
#include "widget/colorbutton/colorbutton.h"

namespace olive {

#define super QTextEdit

ViewerTextEditor::ViewerTextEditor(double scale, QWidget *parent) :
  super(parent)
{
  // Ensure default text color is white
  QPalette p = palette();
  p.setColor(QPalette::Text, Qt::white);
  setPalette(p);

  document()->setDefaultStyleSheet(QStringLiteral("body { color: white; }"));

  viewport()->setAutoFillBackground(false);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  horizontalScrollBar()->setEnabled(false);
  verticalScrollBar()->setEnabled(false);

  // Force DPI to the same one that we're using in the actual render
  dpi_force_ = QImage(1, 1, QImage::Format_RGBA8888_Premultiplied);

  const int dpm = 3780 * scale;
  dpi_force_.setDotsPerMeterX(dpm);
  dpi_force_.setDotsPerMeterY(dpm);
  document()->documentLayout()->setPaintDevice(&dpi_force_);

  connect(qApp, &QApplication::focusChanged, this, &ViewerTextEditor::FocusChanged);
  connect(this, &QTextEdit::currentCharFormatChanged, this, &ViewerTextEditor::FormatChanged);
}

void ViewerTextEditor::ConnectToolBar(ViewerTextEditorToolBar *toolbar)
{
  connect(this, &ViewerTextEditor::destroyed, toolbar, &ViewerTextEditorToolBar::deleteLater);

  connect(toolbar, &ViewerTextEditorToolBar::FamilyChanged, this, &ViewerTextEditor::SetFamily);
  connect(toolbar, &ViewerTextEditorToolBar::SizeChanged, this, &ViewerTextEditor::setFontPointSize);
  connect(toolbar, &ViewerTextEditorToolBar::StyleChanged, this, &ViewerTextEditor::SetStyle);
  connect(toolbar, &ViewerTextEditorToolBar::BoldChanged, this, &ViewerTextEditor::SetFontBold);
  connect(toolbar, &ViewerTextEditorToolBar::ItalicChanged, this, &ViewerTextEditor::setFontItalic);
  connect(toolbar, &ViewerTextEditorToolBar::UnderlineChanged, this, &ViewerTextEditor::setFontUnderline);
  connect(toolbar, &ViewerTextEditorToolBar::StrikethroughChanged, this, &ViewerTextEditor::SetFontStrikethrough);
  connect(toolbar, &ViewerTextEditorToolBar::ColorChanged, this, &ViewerTextEditor::setTextColor);
  connect(toolbar, &ViewerTextEditorToolBar::SmallCapsChanged, this, &ViewerTextEditor::SetSmallCaps);
  connect(toolbar, &ViewerTextEditorToolBar::StretchChanged, this, &ViewerTextEditor::SetFontStretch);
  connect(toolbar, &ViewerTextEditorToolBar::KerningChanged, this, &ViewerTextEditor::SetFontKerning);
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
    deleteLater();
  }
}

void ViewerTextEditor::UpdateToolBar(ViewerTextEditorToolBar *toolbar, const QTextCharFormat &f, Qt::Alignment alignment)
{
  QFontDatabase fd;

  QString family = f.fontFamily();
  if (family.isEmpty()) {
    family = qApp->font().family();
  }

  QString style = f.fontStyleName().toString();
  if (style.isEmpty()) {
    QStringList styles = fd.styles(family);
    foreach (const QString &s, styles) {
      QFont test = fd.font(family, s, f.fontPointSize());
      if (test.weight() == f.fontWeight() && test.italic() == f.fontItalic()) {
        style = s;
        break;
      }
    }
  }

  toolbar->SetFontFamily(family);
  toolbar->SetFontSize(f.fontPointSize());
  toolbar->SetStyle(style);
  toolbar->SetBold(fd.bold(family, style));
  toolbar->SetItalic(f.fontItalic());
  toolbar->SetUnderline(f.fontUnderline());
  toolbar->SetStrikethrough(f.fontStrikeOut());
  toolbar->SetAlignment(alignment);
  toolbar->SetColor(f.foreground().color());
  toolbar->SetSmallCaps(f.fontCapitalization() == QFont::SmallCaps);
  toolbar->SetStretch(f.fontStretch() == 0 ? 100 : f.fontStretch());
  toolbar->SetKerning(f.fontLetterSpacing() == 0 ? 100 : f.fontLetterSpacing());
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

void ViewerTextEditor::SetStyle(const QString &s)
{
  ViewerTextEditorToolBar *toolbar = static_cast<ViewerTextEditorToolBar *>(sender());

  QTextCharFormat f;

  QFontDatabase fd;
  QFont test = fd.font(toolbar->GetFontFamily(), s, currentCharFormat().fontPointSize());
  f.setFontWeight(test.weight());
  f.setFontItalic(test.italic());

  mergeCurrentCharFormat(f);
}

void ViewerTextEditor::SetFontBold(bool e)
{
  QTextCharFormat f;
  f.setFontWeight(e ? QFont::Bold : QFont::Normal);
  mergeCurrentCharFormat(f);
}

void ViewerTextEditor::SetFontStrikethrough(bool e)
{
  QTextCharFormat f;
  f.setFontStrikeOut(e);
  mergeCurrentCharFormat(f);
}

void ViewerTextEditor::SetSmallCaps(bool e)
{
  QTextCharFormat f;
  f.setFontCapitalization(e ? QFont::SmallCaps : QFont::MixedCase);
  mergeCurrentCharFormat(f);
}

void ViewerTextEditor::SetFontStretch(int i)
{
  QTextCharFormat f;
  f.setFontStretch(i);
  mergeCurrentCharFormat(f);
}

void ViewerTextEditor::SetFontKerning(int i)
{
  QTextCharFormat f;
  f.setFontLetterSpacing(i);
  mergeCurrentCharFormat(f);
}

ViewerTextEditorToolBar::ViewerTextEditorToolBar(QWidget *parent) :
  QWidget(parent),
  painted_(false)
{
  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setMargin(0);
  outer_layout->setSpacing(0);

  QHBoxLayout *basic_layout = new QHBoxLayout();
  outer_layout->addLayout(basic_layout);

  font_combo_ = new QFontComboBox();
  connect(font_combo_, &QFontComboBox::currentTextChanged, this, &ViewerTextEditorToolBar::FamilyChanged);
  connect(font_combo_, &QFontComboBox::currentTextChanged, this, &ViewerTextEditorToolBar::UpdateFontStyleList);
  basic_layout->addWidget(font_combo_);

  font_sz_slider_ = new FloatSlider();
  font_sz_slider_->SetMinimum(0.1);
  font_sz_slider_->SetMaximum(9999.9);
  font_sz_slider_->SetDecimalPlaces(1);
  font_sz_slider_->SetAlignment(Qt::AlignCenter);
  font_sz_slider_->setFixedWidth(QtUtils::QFontMetricsWidth(font_sz_slider_->fontMetrics(), QStringLiteral("9999.9")));
  connect(font_sz_slider_, &FloatSlider::ValueChanged, this, &ViewerTextEditorToolBar::SizeChanged);
  font_sz_slider_->SetLadderElementCount(2);
  basic_layout->addWidget(font_sz_slider_);

  style_combo_ = new QComboBox();
  connect(style_combo_, &QComboBox::currentTextChanged, this, &ViewerTextEditorToolBar::StyleChanged);
  basic_layout->addWidget(style_combo_);

  bold_btn_ = new QPushButton();
  connect(bold_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::BoldChanged);
  bold_btn_->setCheckable(true);
  bold_btn_->setIcon(icon::TextBold);
  basic_layout->addWidget(bold_btn_);

  italic_btn_ = new QPushButton();
  connect(italic_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::ItalicChanged);
  italic_btn_->setCheckable(true);
  italic_btn_->setIcon(icon::TextItalic);
  basic_layout->addWidget(italic_btn_);

  underline_btn_ = new QPushButton();
  connect(underline_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::UnderlineChanged);
  underline_btn_->setCheckable(true);
  underline_btn_->setIcon(icon::TextUnderline);
  basic_layout->addWidget(underline_btn_);

  strikethrough_btn_ = new QPushButton();
  connect(strikethrough_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::StrikethroughChanged);
  strikethrough_btn_->setCheckable(true);
  strikethrough_btn_->setText(tr("S")); // FIXME: Source icon
  basic_layout->addWidget(strikethrough_btn_);

  basic_layout->addWidget(QtUtils::CreateVerticalLine());

  align_left_btn_ = new QPushButton();
  align_left_btn_->setCheckable(true);
  align_left_btn_->setIcon(icon::TextAlignLeft);
  connect(align_left_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignLeft);});
  basic_layout->addWidget(align_left_btn_);

  align_center_btn_ = new QPushButton();
  align_center_btn_->setCheckable(true);
  align_center_btn_->setIcon(icon::TextAlignCenter);
  connect(align_center_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignCenter);});
  basic_layout->addWidget(align_center_btn_);

  align_right_btn_ = new QPushButton();
  align_right_btn_->setCheckable(true);
  align_right_btn_->setIcon(icon::TextAlignRight);
  connect(align_right_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignRight);});
  basic_layout->addWidget(align_right_btn_);

  align_justify_btn_ = new QPushButton();
  align_justify_btn_->setCheckable(true);
  align_justify_btn_->setIcon(icon::TextAlignJustify);
  connect(align_justify_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignJustify);});
  basic_layout->addWidget(align_justify_btn_);

  basic_layout->addWidget(QtUtils::CreateVerticalLine());

  color_btn_ = new QPushButton();
  color_btn_->setAutoFillBackground(true);
  connect(color_btn_, &QPushButton::clicked, this, [this]{
    QColor c = color_btn_->property("color").value<QColor>();

    QColorDialog cd(c, this);
    if (cd.exec() == QDialog::Accepted) {
      c = cd.selectedColor();
      SetColor(c);
      emit ColorChanged(c);
    }
  });
  basic_layout->addWidget(color_btn_);

  basic_layout->addStretch();

  QHBoxLayout *advanced_layout = new QHBoxLayout();
  outer_layout->addLayout(advanced_layout);

  advanced_layout->addWidget(new QLabel(tr("Stretch:"))); // FIXME: Procure icon

  stretch_slider_ = new IntegerSlider();
  stretch_slider_->SetMinimum(0);
  stretch_slider_->SetDefaultValue(100);
  stretch_slider_->setFixedWidth(QtUtils::QFontMetricsWidth(stretch_slider_->fontMetrics(), QStringLiteral("9999")));
  connect(stretch_slider_, &IntegerSlider::ValueChanged, this, &ViewerTextEditorToolBar::StretchChanged);
  advanced_layout->addWidget(stretch_slider_);

  advanced_layout->addWidget(new QLabel(tr("Kerning:"))); // FIXME: Procure icon

  kerning_slider_ = new IntegerSlider();
  kerning_slider_->SetMinimum(0);
  kerning_slider_->SetDefaultValue(100);
  kerning_slider_->setFixedWidth(QtUtils::QFontMetricsWidth(kerning_slider_->fontMetrics(), QStringLiteral("9999")));
  connect(kerning_slider_, &IntegerSlider::ValueChanged, this, &ViewerTextEditorToolBar::KerningChanged);
  advanced_layout->addWidget(kerning_slider_);

  small_caps_btn_ = new QPushButton(tr("Small Caps")); // FIXME: Procure icon
  small_caps_btn_->setCheckable(true);
  connect(small_caps_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::SmallCapsChanged);
  advanced_layout->addWidget(small_caps_btn_);

  advanced_layout->addStretch();

  setAutoFillBackground(true);
}

void ViewerTextEditorToolBar::SetAlignment(Qt::Alignment a)
{
  align_left_btn_->setChecked(a == Qt::AlignLeft);
  align_center_btn_->setChecked(a == Qt::AlignCenter);
  align_right_btn_->setChecked(a == Qt::AlignRight);
  align_justify_btn_->setChecked(a == Qt::AlignJustify);
}

void ViewerTextEditorToolBar::SetColor(const QColor &c)
{
  color_btn_->setProperty("color", c);
  color_btn_->setStyleSheet(QStringLiteral("QPushButton { background: %1; }").arg(c.name()));
}

void ViewerTextEditorToolBar::closeEvent(QCloseEvent *event)
{
  event->ignore();
}

void ViewerTextEditorToolBar::paintEvent(QPaintEvent *event)
{
  if (!painted_) {
    emit FirstPaint();
    painted_ = true;
  }
  QWidget::paintEvent(event);
}

void ViewerTextEditorToolBar::UpdateFontStyleList(const QString &family)
{
  style_combo_->blockSignals(true);
  style_combo_->clear();
  QStringList l = QFontDatabase().styles(family);
  foreach (const QString &style, l) {
    style_combo_->addItem(style);
  }
  style_combo_->blockSignals(false);
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
