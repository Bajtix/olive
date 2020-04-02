#include "export.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStandardPaths>

#include "project/item/sequence/sequence.h"
#include "project/project.h"
#include "render/pixelformat.h"
#include "ui/icons/icons.h"

ExportDialog::ExportDialog(ViewerOutput *viewer_node, QWidget *parent) :
  QDialog(parent),
  viewer_node_(viewer_node),
  previously_selected_format_(0),
  exporter_(nullptr),
  cancelled_(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  QWidget* outer_preferences_area = new QWidget();
  QVBoxLayout* outer_preferences_layout = new QVBoxLayout(outer_preferences_area);

  preferences_area_ = new QWidget();
  outer_preferences_layout->addWidget(preferences_area_);
  QGridLayout* preferences_layout = new QGridLayout(preferences_area_);
  preferences_layout->setMargin(0);

  int row = 0;

  QLabel* fn_lbl = new QLabel(tr("Filename:"));
  fn_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(fn_lbl, row, 0);

  filename_edit_ = new QLineEdit();
  preferences_layout->addWidget(filename_edit_, row, 1, 1, 2);

  QPushButton* file_browse_btn = new QPushButton();
  file_browse_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  file_browse_btn->setIcon(icon::Folder);
  file_browse_btn->setToolTip(tr("Browse for exported file filename"));
  connect(file_browse_btn, SIGNAL(clicked(bool)), this, SLOT(BrowseFilename()));
  preferences_layout->addWidget(file_browse_btn, row, 3);

  row++;

  QLabel* preset_lbl = new QLabel(tr("Preset:"));
  preset_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_lbl, row, 0);
  QComboBox* preset_combobox = new QComboBox();
  preset_combobox->addItem(tr("Same As Source - High Quality"));
  preset_combobox->addItem(tr("Same As Source - Medium Quality"));
  preset_combobox->addItem(tr("Same As Source - Low Quality"));
  preferences_layout->addWidget(preset_combobox, row, 1);

  QPushButton* preset_load_btn = new QPushButton();
  preset_load_btn->setIcon(icon::Open);
  preset_load_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_load_btn, row, 2);

  QPushButton* preset_save_btn = new QPushButton();
  preset_save_btn->setIcon(icon::Save);
  preset_save_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_save_btn, row, 3);

  row++;

  QFrame* horizontal_line = new QFrame();
  horizontal_line->setFrameShape(QFrame::HLine);
  horizontal_line->setFrameShadow(QFrame::Sunken);
  preferences_layout->addWidget(horizontal_line, row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  format_combobox_ = new QComboBox();
  connect(format_combobox_, SIGNAL(currentIndexChanged(int)), this, SLOT(FormatChanged(int)));
  preferences_layout->addWidget(format_combobox_, row, 1, 1, 3);

  row++;

  QHBoxLayout* av_enabled_layout = new QHBoxLayout();

  video_enabled_ = new QCheckBox(tr("Export Video"));
  video_enabled_->setChecked(true);
  av_enabled_layout->addWidget(video_enabled_);

  audio_enabled_ = new QCheckBox(tr("Export Audio"));
  audio_enabled_->setChecked(true);
  av_enabled_layout->addWidget(audio_enabled_);

  preferences_layout->addLayout(av_enabled_layout, row, 0, 1, 4);

  row++;

  QTabWidget* preferences_tabs = new QTabWidget();
  QScrollArea* video_area = new QScrollArea();
  color_manager_ = static_cast<Sequence*>(viewer_node_->parent())->project()->color_manager();
  video_tab_ = new ExportVideoTab(color_manager_);
  video_area->setWidgetResizable(true);
  video_area->setWidget(video_tab_);
  preferences_tabs->addTab(video_area, tr("Video"));
  QScrollArea* audio_area = new QScrollArea();
  audio_tab_ = new ExportAudioTab();
  audio_area->setWidgetResizable(true);
  audio_area->setWidget(audio_tab_);
  preferences_tabs->addTab(audio_area, tr("Audio"));
  preferences_layout->addWidget(preferences_tabs, row, 0, 1, 4);

  QHBoxLayout* progress_bar_layout = new QHBoxLayout();
  progress_bar_layout->setMargin(0);
  outer_preferences_layout->addLayout(progress_bar_layout);

  progress_bar_ = new QProgressBar();
  progress_bar_->setEnabled(false);
  progress_bar_->setValue(0);
  progress_bar_layout->addWidget(progress_bar_);

  export_cancel_btn_ = new QPushButton();
  export_cancel_btn_->setIcon(icon::Error);
  export_cancel_btn_->setEnabled(false);
  connect(export_cancel_btn_, &QPushButton::clicked, this, &ExportDialog::CancelExport);
  progress_bar_layout->addWidget(export_cancel_btn_);

  buttons_ = new QDialogButtonBox();
  buttons_->setCenterButtons(true);
  buttons_->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
  buttons_->addButton(QDialogButtonBox::Cancel);
  connect(buttons_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons_, SIGNAL(rejected()), this, SLOT(reject()));
  outer_preferences_layout->addWidget(buttons_);

  splitter->addWidget(outer_preferences_area);

  QWidget* preview_area = new QWidget();
  QVBoxLayout* preview_layout = new QVBoxLayout(preview_area);
  preview_layout->addWidget(new QLabel(tr("Preview")));
  preview_viewer_ = new ViewerWidget();
  preview_viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  preview_layout->addWidget(preview_viewer_);
  splitter->addWidget(preview_area);

  // Set default filename
  // FIXME: Use Sequence name and project filename to construct this
  SetDefaultFilename();

  // Set up available export formats
  SetUpFormats();

  // Populate combobox formats
  foreach (const ExportFormat& format, formats_) {
    format_combobox_->addItem(format.name());
  }

  // Set defaults
  format_combobox_->setCurrentIndex(kFormatMPEG4);
  video_tab_->width_slider()->SetValue(viewer_node_->video_params().width());
  video_tab_->height_slider()->SetValue(viewer_node_->video_params().height());
  video_tab_->set_frame_rate(viewer_node_->video_params().time_base().flipped());
  audio_tab_->set_sample_rate(viewer_node_->audio_params().sample_rate());
  audio_tab_->set_channel_layout(viewer_node_->audio_params().channel_layout());

  video_aspect_ratio_ = static_cast<double>(viewer_node_->video_params().width()) / static_cast<double>(viewer_node_->video_params().height());

  connect(video_tab_->width_slider(), SIGNAL(ValueChanged(int64_t)), this, SLOT(ResolutionChanged()));
  connect(video_tab_->height_slider(), SIGNAL(ValueChanged(int64_t)), this, SLOT(ResolutionChanged()));
  connect(video_tab_->scaling_method_combobox(), SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateViewerDimensions()));
  connect(video_tab_->maintain_aspect_checkbox(), SIGNAL(toggled(bool)), this, SLOT(ResolutionChanged()));
  connect(video_tab_->codec_combobox(), SIGNAL(currentIndexChanged(int)), this, SLOT(VideoCodecChanged()));
  connect(video_tab_,
          &ExportVideoTab::DisplayColorSpaceChanged,
          preview_viewer_,
          static_cast<void (ViewerWidget::*)(const QString&, const QString&, const QString&)>(&ViewerWidget::SetOCIOParameters));

  // Set viewer to view the node
  preview_viewer_->ConnectViewerNode(viewer_node_);
  preview_viewer_->SetColorMenuEnabled(false);
  preview_viewer_->SetOCIOParameters(video_tab_->CurrentOCIODisplay(), video_tab_->CurrentOCIOView(), video_tab_->CurrentOCIOLook());

  // Update renderer
  // FIXME: This is going to be VERY slow since it will need to hash every single frame. It would be better to have a
  //        the renderer save the map as some sort of file that this can load.
  preview_viewer_->video_renderer()->InvalidateCache(TimeRange(0, viewer_node_->Length()));
}

void ExportDialog::accept()
{
  if (!video_enabled_->isChecked() && !audio_enabled_->isChecked()) {
    QMessageBox::critical(this,
                         tr("Invalid parameters"),
                         tr("Both video and audio are disabled. There's nothing to export."),
                         QMessageBox::Ok);
    return;
  }

  // Validate if the entered filename contains the correct extension (the extension is necessary for both FFmpeg and
  // OIIO to determine the output format)
  QString necessary_ext = QStringLiteral(".%1").arg(formats_.at(format_combobox_->currentIndex()).extension());

  // If it doesn't, see if the user wants to append it automatically. If not, we don't abort the export.
  if (!filename_edit_->text().endsWith(necessary_ext, Qt::CaseInsensitive)) {
    if (QMessageBox::warning(this,
                             tr("Invalid filename"),
                             tr("The filename must contain the extension \".%1\". Would you like to append it automatically?"),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      filename_edit_->setText(filename_edit_->text().append(necessary_ext));
    } else {
      return;
    }
  }

  // Validate the intended path
  QFileInfo file_info(filename_edit_->text());
  QFileInfo dir_info(file_info.path());

  // If the directory does not exist, try to create it
  if (!QDir(file_info.path()).mkpath(QStringLiteral("."))) {
    QMessageBox::critical(this,
                          tr("Failed to create output directory"),
                          tr("The intended output directory doesn't exist and Olive couldn't create it. Please choose a different filename."),
                          QMessageBox::Ok);
    return;
  }

  // Validate if the file exists and whether the user wishes to overwrite it
  if (file_info.exists()
      && QMessageBox::warning(this,
                              tr("Confirm Overwrite"),
                              tr("The file \"%1\" already exists. Do you want to overwrite it?").arg(filename_edit_->text()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
    return;
  }

  QMatrix4x4 transform;

  int dest_width = static_cast<int>(video_tab_->width_slider()->GetValue());
  int dest_height = static_cast<int>(video_tab_->height_slider()->GetValue());

  if (video_tab_->scaling_method_combobox()->isEnabled()) {
    int source_width = viewer_node_->video_params().width();
    int source_height = viewer_node_->video_params().height();

    transform = GenerateMatrix(static_cast<ExportVideoTab::ScalingMethod>(video_tab_->scaling_method_combobox()->currentData().toInt()),
                               source_width,
                               source_height,
                               dest_width,
                               dest_height);
  }

  RenderMode::Mode render_mode = RenderMode::kOnline;

  VideoRenderingParams video_render_params(dest_width,
                                           dest_height,
                                           video_tab_->frame_rate().flipped(),
                                           PixelFormat::instance()->GetConfiguredFormatForMode(render_mode),
                                           render_mode);

  AudioRenderingParams audio_render_params(audio_tab_->sample_rate_combobox()->currentData().toInt(),
                                           audio_tab_->channel_layout_combobox()->currentData().toULongLong(),
                                           SampleFormat::GetConfiguredFormatForMode(render_mode));

  ColorProcessorPtr color_processor = ColorProcessor::Create(color_manager_->GetConfig(),
                                                             color_manager_->GetReferenceColorSpace(),
                                                             video_tab_->CurrentOCIODisplay(),
                                                             video_tab_->CurrentOCIOView(),
                                                             video_tab_->CurrentOCIOLook());

  // Set up encoder
  EncodingParams encoding_params;
  encoding_params.SetFilename(filename_edit_->text());
  encoding_params.SetExportLength(viewer_node_->Length());

  if (video_enabled_->isChecked()) {
    const ExportCodec& video_codec = codecs_.at(video_tab_->codec_combobox()->currentData().toInt());
    encoding_params.EnableVideo(video_render_params,
                                video_codec.id());

    video_tab_->GetCodecSection()->AddOpts(&encoding_params);
  }

  if (audio_enabled_->isChecked()) {
    const ExportCodec& audio_codec = codecs_.at(audio_tab_->codec_combobox()->currentData().toInt());
    encoding_params.EnableAudio(audio_render_params,
                                audio_codec.id());
  }

  Encoder* encoder = Encoder::CreateFromID("ffmpeg", encoding_params);

  exporter_ = new Exporter(viewer_node_, encoder);

  if (video_enabled_->isChecked()) {
    exporter_->EnableVideo(video_render_params, transform, color_processor);
  }

  if (audio_enabled_->isChecked()) {
    exporter_->EnableAudio(audio_render_params);
  }

  connect(exporter_, &Exporter::ExportEnded, this, &ExportDialog::ExporterIsDone);
  connect(exporter_, &Exporter::ProgressChanged, progress_bar_, &QProgressBar::setValue);

  QMetaObject::invokeMethod(exporter_, "StartExporting", Qt::QueuedConnection);

  SetUIElementsEnabled(false);
}

void ExportDialog::closeEvent(QCloseEvent *e)
{
  if (exporter_) {
    if (QMessageBox::question(this,
                              tr("Still Exporting"),
                              tr("This sequence is still being exported. Do you wish to cancel it?"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      CancelExport();
    } else {
      e->ignore();
      return;
    }
  }

  preview_viewer_->ConnectViewerNode(nullptr);

  QDialog::closeEvent(e);
}

void ExportDialog::BrowseFilename()
{
  const ExportFormat& current_format = formats_.at(format_combobox_->currentIndex());

  QString browsed_fn = QFileDialog::getSaveFileName(this,
                                                    "",
                                                    filename_edit_->text(),
                                                    QStringLiteral("%1 (*.%2)").arg(current_format.name(), current_format.extension()),
                                                    nullptr,

                                                    // We don't confirm overwrite here because we do it later
                                                    QFileDialog::DontConfirmOverwrite);

  if (!browsed_fn.isEmpty()) {
    filename_edit_->setText(browsed_fn);
  }
}

void ExportDialog::FormatChanged(int index)
{
  QString current_filename = filename_edit_->text();
  const QString& previously_selected_ext = formats_.at(previously_selected_format_).extension();
  const ExportFormat& current_format = formats_.at(index);
  const QString& currently_selected_ext = current_format.extension();

  // If the previous extension was added, remove it
  if (current_filename.endsWith(previously_selected_ext, Qt::CaseInsensitive)) {
    current_filename.resize(current_filename.size() - previously_selected_ext.size() - 1);
  }

  // Add the extension and set it
  current_filename.append('.');
  current_filename.append(currently_selected_ext);
  filename_edit_->setText(current_filename);

  previously_selected_format_ = index;

  // Update video and audio comboboxes
  video_tab_->codec_combobox()->clear();
  foreach (int vcodec, current_format.video_codecs()) {
    video_tab_->codec_combobox()->addItem(codecs_.at(vcodec).name(), vcodec);
  }
  VideoCodecChanged();

  audio_tab_->codec_combobox()->clear();
  foreach (int acodec, current_format.audio_codecs()) {
    audio_tab_->codec_combobox()->addItem(codecs_.at(acodec).name(), acodec);
  }
}

void ExportDialog::ResolutionChanged()
{
  if (video_tab_->maintain_aspect_checkbox()->isChecked()) {
    // Keep aspect ratio maintained
    if (sender() == video_tab_->height_slider()) {
      video_tab_->width_slider()->SetValue(qRound(static_cast<double>(video_tab_->height_slider()->GetValue()) * video_aspect_ratio_));
    } else {
      // This catches both the width slider changing and the maintain aspect ratio checkbox changing
      video_tab_->height_slider()->SetValue(qRound(static_cast<double>(video_tab_->width_slider()->GetValue()) / video_aspect_ratio_));
    }
  }

  UpdateViewerDimensions();
}

void ExportDialog::VideoCodecChanged()
{
  int codec_index = video_tab_->codec_combobox()->currentData().toInt();
  const ExportCodec& codec = codecs_.at(codec_index);

  if (codec_index == kCodecH264) {
    video_tab_->SetCodecSection(video_tab_->h264_section());
  } else if (codec.flags() & ExportCodec::kStillImage) {
    video_tab_->SetCodecSection(video_tab_->image_section());
  }
}

void ExportDialog::SetUpFormats()
{
  // Set up codecs
  for (int i=0;i<kCodecCount;i++) {
    switch (static_cast<Codec>(i)) {
    case kCodecDNxHD:
      codecs_.append(ExportCodec(tr("DNxHD"), "dnxhd"));
      break;
    case kCodecH264:
      codecs_.append(ExportCodec(tr("H.264"), "libx264"));
      break;
    case kCodecH265:
      codecs_.append(ExportCodec(tr("H.265"), "libx265"));
      break;
    case kCodecOpenEXR:
      codecs_.append(ExportCodec(tr("OpenEXR"), "exr", ExportCodec::kStillImage));
      break;
    case kCodecPNG:
      codecs_.append(ExportCodec(tr("PNG"), "png", ExportCodec::kStillImage));
      break;
    case kCodecProRes:
      codecs_.append(ExportCodec(tr("ProRes"), "prores"));
      break;
    case kCodecTIFF:
      codecs_.append(ExportCodec(tr("TIFF"), "tiff", ExportCodec::kStillImage));
      break;
    case kCodecMP2:
      codecs_.append(ExportCodec(tr("MP2"), "mp2"));
      break;
    case kCodecMP3:
      codecs_.append(ExportCodec(tr("MP3"), "libmp3lame"));
      break;
    case kCodecAAC:
      codecs_.append(ExportCodec(tr("AAC"), "aac"));
      break;
    case kCodecPCM:
      codecs_.append(ExportCodec(tr("PCM (Uncompressed)"), "pcm"));
      break;
    case kCodecCount:
      break;
    }
  }

  // Set up formats
  for (int i=0;i<kFormatCount;i++) {
    switch (static_cast<Format>(i)) {
    case kFormatDNxHD:
      formats_.append(ExportFormat(tr("DNxHD"), "mxf", "ffmpeg", {kCodecDNxHD}, {kCodecPCM}));
      break;
    case kFormatMatroska:
      formats_.append(ExportFormat(tr("Matroska Video"), "mkv", "ffmpeg", {kCodecH264, kCodecH265}, {kCodecAAC, kCodecMP2, kCodecMP3, kCodecPCM}));
      break;
    case kFormatMPEG4:
      formats_.append(ExportFormat(tr("MPEG-4 Video"), "mp4", "ffmpeg", {kCodecH264, kCodecH265}, {kCodecAAC, kCodecMP2, kCodecMP3, kCodecPCM}));
      break;
    case kFormatOpenEXR:
      formats_.append(ExportFormat(tr("OpenEXR"), "exr", "oiio", {kCodecOpenEXR}, {}));
      break;
    case kFormatQuickTime:
      formats_.append(ExportFormat(tr("QuickTime"), "mov", "ffmpeg", {kCodecH264, kCodecH265, kCodecProRes}, {kCodecAAC, kCodecMP2, kCodecMP3, kCodecPCM}));
      break;
    case kFormatPNG:
      formats_.append(ExportFormat(tr("PNG"), "png", "oiio", {kCodecPNG}, {}));
      break;
    case kFormatTIFF:
      formats_.append(ExportFormat(tr("TIFF"), "tiff", "oiio", {kCodecTIFF}, {}));
      break;
    case kFormatCount:
      break;
    }
  }
}

void ExportDialog::LoadPresets()
{

}

void ExportDialog::SetDefaultFilename()
{
  Sequence* s = static_cast<Sequence*>(viewer_node_->parent());
  Project* p = s->project();

  QDir doc_location;

  if (p->filename().isEmpty()) {
    doc_location.setPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  } else {
    doc_location = QFileInfo(p->filename()).dir();
  }

  QString file_location = doc_location.filePath(s->name());
  filename_edit_->setText(file_location);
}

QMatrix4x4 ExportDialog::GenerateMatrix(ExportVideoTab::ScalingMethod method, int source_width, int source_height, int dest_width, int dest_height)
{
  QMatrix4x4 preview_matrix;

  if (method == ExportVideoTab::kStretch) {
    return preview_matrix;
  }

  float export_ar = static_cast<float>(dest_width) / static_cast<float>(dest_height);
  float source_ar = static_cast<float>(source_width) / static_cast<float>(source_height);

  if (qFuzzyCompare(export_ar, source_ar)) {
    return preview_matrix;
  }

  if ((export_ar > source_ar) == (method == ExportVideoTab::kFit)) {
    preview_matrix.scale(source_ar / export_ar, 1.0F);
  } else {
    preview_matrix.scale(1.0F, export_ar / source_ar);
  }

  return preview_matrix;
}

void ExportDialog::SetUIElementsEnabled(bool enabled)
{
  preferences_area_->setEnabled(enabled);
  buttons_->setEnabled(enabled);

  export_cancel_btn_->setEnabled(!enabled);
}

void ExportDialog::UpdateViewerDimensions()
{
  preview_viewer_->SetOverrideSize(static_cast<int>(video_tab_->width_slider()->GetValue()),
                                   static_cast<int>(video_tab_->height_slider()->GetValue()));

  preview_viewer_->SetMatrix(GenerateMatrix(static_cast<ExportVideoTab::ScalingMethod>(video_tab_->scaling_method_combobox()->currentData().toInt()),
                                            viewer_node_->video_params().width(),
                                            viewer_node_->video_params().height(),
                                            static_cast<int>(video_tab_->width_slider()->GetValue()),
                                            static_cast<int>(video_tab_->height_slider()->GetValue())));
}

void ExportDialog::ExporterIsDone()
{
  if (exporter_->GetExportStatus()) {
    QMessageBox::information(this,
                             tr("Export Status"),
                             tr("Export completed successfully."),
                             QMessageBox::Ok);

    QDialog::accept();
  } else {
    if (!cancelled_) {
      QMessageBox::critical(this,
                            tr("Export Status"),
                            tr("Export failed: %1").arg(exporter_->GetExportError()),
                            QMessageBox::Ok);
    }

    SetUIElementsEnabled(true);
  }

  exporter_->deleteLater();
  exporter_ = nullptr;
  cancelled_ = false;
}

void ExportDialog::CancelExport()
{
  if (exporter_) {
    cancelled_ = true;
    exporter_->Cancel();
  }
}
