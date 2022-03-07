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

#ifndef VIEWERTEXTEDITOR_H
#define VIEWERTEXTEDITOR_H

#include <QApplication>
#include <QFontComboBox>
#include <QTextEdit>
#include <QPushButton>

#include "widget/slider/floatslider.h"

namespace olive {

class ViewerTextEditorToolBar : public QWidget
{
  Q_OBJECT
public:
  ViewerTextEditorToolBar(QWidget *parent = nullptr);

public slots:
  void SetFontFamily(const QString &s)
  {
    font_combo_->blockSignals(true);
    font_combo_->setCurrentFont(s.isEmpty() ? qApp->font().family() : s);
    font_combo_->blockSignals(false);
  }

  void SetFontSize(double d) { font_sz_slider_->SetValue(d); }
  void SetItalic(bool e) { italic_btn_->setChecked(e); }
  void SetUnderline(bool e) { underline_btn_->setChecked(e); }
  void SetAlignment(Qt::Alignment a);

signals:
  void FamilyChanged(const QString &s);
  void SizeChanged(double d);
  void ItalicChanged(bool e);
  void UnderlineChanged(bool e);
  void AlignmentChanged(Qt::Alignment alignment);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;

  virtual void mouseMoveEvent(QMouseEvent *event) override;

  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void closeEvent(QCloseEvent *event) override;

private:
  QPoint drag_anchor_;

  QFontComboBox *font_combo_;

  FloatSlider *font_sz_slider_;

  QComboBox *weight_combo_;

  QPushButton *italic_btn_;

  QPushButton *underline_btn_;

  QPushButton *align_left_btn_;
  QPushButton *align_center_btn_;
  QPushButton *align_right_btn_;
  QPushButton *align_justify_btn_;

  QPushButton *color_btn_;

};

class ViewerTextEditor : public QTextEdit
{
  Q_OBJECT
public:
  ViewerTextEditor(QWidget *parent = nullptr);

  void ConnectToolBar(ViewerTextEditorToolBar *toolbar);

protected:
  virtual void keyPressEvent(QKeyEvent *event) override;

private:
  static void UpdateToolBar(ViewerTextEditorToolBar *toolbar, const QTextCharFormat &f, Qt::Alignment alignment);

  QVector<ViewerTextEditorToolBar *> toolbars_;

private slots:
  void FocusChanged(QWidget *old, QWidget *now);

  void FormatChanged(const QTextCharFormat &f);

  void SetFamily(const QString &s);

};

}

#endif // VIEWERTEXTEDITOR_H
