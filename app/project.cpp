// 프로젝트 파일(<모델이름>.trench.json) — 앱 쪽 붙임(사용자 결정 2026-10-11
// 「C: 앱 폴더 projects\ 자동 저장 + 저장/열기」). 읽기·쓰기 모양은
// core(asec/project.hpp)가 맡고, 여기서는 지금 상태 ↔ 문서 변환과
// 파일·폴더·대화상자만.
#include "mainwindow.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QStandardPaths>
#include <QTimer>
#include <cmath>

#include "asec/project.hpp"

using namespace asec;

namespace {
QString qs8(const std::string &s) {
  return QString::fromUtf8(s.c_str(), int(s.size()));
}
bool readTextFile(const QString &path, std::string *out) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return false;
  const QByteArray b = f.readAll();
  out->assign(b.constData(), size_t(b.size()));
  return true;
}
} // namespace

QString MainWindow::projectRootOverride;

QString MainWindow::resolveModelPath(const QString &path) {
  if (path.isEmpty())
    return QString();
  const QFileInfo fi(path);
  if (!fi.isDir())
    return path;
  const QDir dir(path);
  const QStringList filters = {QStringLiteral("*.3mx"), QStringLiteral("*.3sm"),
                               QStringLiteral("*.obj"),
                               QStringLiteral("*.trench.json")};
  for (const QString &flt : filters) {
    const QStringList direct = dir.entryList({flt}, QDir::Files, QDir::Name);
    if (!direct.isEmpty())
      return dir.absoluteFilePath(direct.first());
  }
  const QDir sceneDir(dir.filePath(QStringLiteral("Scene")));
  if (sceneDir.exists()) {
    const QStringList sc =
        sceneDir.entryList({QStringLiteral("*.3mx"), QStringLiteral("*.3sm"),
                            QStringLiteral("*.obj")},
                           QDir::Files, QDir::Name);
    if (!sc.isEmpty())
      return sceneDir.absoluteFilePath(sc.first());
  }
  for (const QString &sub :
       dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    const QDir sd(dir.filePath(sub));
    const QStringList sf =
        sd.entryList({QStringLiteral("*.3mx"), QStringLiteral("*.3sm"),
                      QStringLiteral("*.obj")},
                     QDir::Files, QDir::Name);
    if (!sf.isEmpty())
      return sd.absoluteFilePath(sf.first());
    const QDir ssd(sd.filePath(QStringLiteral("Scene")));
    if (ssd.exists()) {
      const QStringList ssf =
          ssd.entryList({QStringLiteral("*.3mx")}, QDir::Files, QDir::Name);
      if (!ssf.isEmpty())
        return ssd.absoluteFilePath(ssf.first());
    }
  }
  return path;
}

QString MainWindow::modelDisplayName(const QString &path) {
  if (path.isEmpty())
    return QString();
  const QFileInfo fi(resolveModelPath(path));
  const QString base = fi.completeBaseName();
  static const QRegularExpression kGeneric(
      QStringLiteral("^(production(_\\d+)?|scene|model|root|output|mesh)$"),
      QRegularExpression::CaseInsensitiveOption);
  if (kGeneric.match(base).hasMatch()) {
    QDir d = fi.absoluteDir();
    QString dirName = d.dirName();
    if (dirName.compare(QStringLiteral("Scene"), Qt::CaseInsensitive) == 0 &&
        d.cdUp()) {
      dirName = d.dirName();
      if (kGeneric.match(dirName).hasMatch() && d.cdUp()) {
        if (d.dirName().compare(QStringLiteral("Productions"),
                                Qt::CaseInsensitive) == 0 &&
            d.cdUp())
          dirName = d.dirName();
        else
          dirName = d.dirName();
      }
    }
    if (!dirName.isEmpty() && dirName != QStringLiteral(".") &&
        dirName != QStringLiteral("/"))
      return dirName;
  }
  return base;
}

bool MainWindow::isValidCameraForScene(double cx, double cy, double mpp) const {
  if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(mpp) ||
      mpp <= 1e-6 || mpp > 1e5)
    return false;
  if (!sceneBounds_.valid())
    return true;
  const double sx = std::max(1.0, sceneBounds_.mx.x - sceneBounds_.mn.x);
  const double sy = std::max(1.0, sceneBounds_.mx.y - sceneBounds_.mn.y);
  const double margin = std::max(sx, sy) * 3.0;
  return cx >= sceneBounds_.mn.x - margin && cx <= sceneBounds_.mx.x + margin &&
         cy >= sceneBounds_.mn.y - margin && cy <= sceneBounds_.mx.y + margin &&
         mpp <= std::max(sx, sy) * 2.0;
}

bool MainWindow::isValidLineForScene(const SectionLine &l) const {
  if (!std::isfinite(l.a.x) || !std::isfinite(l.a.y) || !std::isfinite(l.b.x) ||
      !std::isfinite(l.b.y))
    return false;
  if (SectionFrame(l).L < 0.01)
    return false;
  if (!sceneBounds_.valid())
    return true;
  const double sx = std::max(1.0, sceneBounds_.mx.x - sceneBounds_.mn.x);
  const double sy = std::max(1.0, sceneBounds_.mx.y - sceneBounds_.mn.y);
  const double margin = std::max(sx, sy) * 2.5;
  const double x0 = sceneBounds_.mn.x - margin, x1 = sceneBounds_.mx.x + margin;
  const double y0 = sceneBounds_.mn.y - margin, y1 = sceneBounds_.mx.y + margin;
  return (l.a.x >= x0 && l.a.x <= x1 && l.a.y >= y0 && l.a.y <= y1) ||
         (l.b.x >= x0 && l.b.x <= x1 && l.b.y >= y0 && l.b.y <= y1);
}

// 자동 저장 폴더: exe 옆 projects\(포터블 — 폴더째 옮겨도 따라감). 거기에 쓸 수
// 없으면 %LOCALAPPDATA%\Trench\projects\.
// --settings DIR 로 설정을 격리하면 DIR\projects(시험용). 모델 폴더(H:\3sm
// 등)에는 절대 쓰지 않는다
QString MainWindow::projectDir() const {
  if (!projectRootOverride.isEmpty()) {
    QDir().mkpath(projectRootOverride + "/projects");
    return projectRootOverride + "/projects";
  }
  const QString beside = QCoreApplication::applicationDirPath() + "/projects";
  if (QDir().mkpath(beside) && QFileInfo(beside).isWritable()) {
    QFile probe(beside +
                "/.write-test"); // isWritable 은 폴더 ACL 만 봐서 Program Files
                                 // 에서 틀릴 수 있다 — 실제로 써 본다
    if (probe.open(QIODevice::WriteOnly)) {
      probe.close();
      probe.remove();
      return beside;
    }
  }
  const QString fallback =
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
      "/Trench/projects";
  QDir().mkpath(fallback);
  return fallback;
}

QString MainWindow::autoProjectPath() const {
  if (path_.isEmpty())
    return QString();
  const QString disp = modelDisplayName(path_);
  const QString candidate =
      projectDir() + "/" + qs8(projectFileName(disp.toStdString()));
  if (QFileInfo::exists(candidate)) {
    ProjectDoc d;
    QString e;
    if (const_cast<MainWindow *>(this)->readProjectFile(candidate, d, &e)) {
      if (QFileInfo(qs8(d.modelPath))
              .absoluteFilePath()
              .compare(QFileInfo(path_).absoluteFilePath(),
                       Qt::CaseInsensitive) != 0) {
        const QByteArray h =
            QCryptographicHash::hash(
                QFileInfo(path_).absoluteFilePath().toLower().toUtf8(),
                QCryptographicHash::Sha1)
                .toHex()
                .left(6);
        const QString disamb =
            disp + QStringLiteral("_") + QString::fromLatin1(h);
        return projectDir() + "/" + qs8(projectFileName(disamb.toStdString()));
      }
    }
  }
  return candidate;
}

// 헤더 「저장됨 · projects\이름.trench.json」 — 앱 폴더 안이면 상대 경로,
// 아니면 파일 이름만(긴 경로가 창 최소 너비를 키우지 않게 — CI --ui-audit
// min-width). 전체 경로는 applyProjectHeader 가 풍선 도움말에 넣는다
QString MainWindow::projectSavedLabel() const {
  if (projectPath_.isEmpty())
    return QStringLiteral("저장됨");
  const QString appDir =
      QDir(QCoreApplication::applicationDirPath()).absolutePath();
  QString shown = projectPath_;
  if (shown.startsWith(appDir + "/", Qt::CaseInsensitive))
    shown = shown.mid(appDir.size() + 1);
  else
    shown = QFileInfo(projectPath_).fileName();
  return QStringLiteral("저장됨 · %1").arg(QDir::toNativeSeparators(shown));
}

void MainWindow::applyProjectHeader() {
  if (!docTabCorner_)
    return;
  if (!src_) {
    docTabCorner_->setText(QString());
    docTabCorner_->setToolTip(QString());
    return;
  }
  docTabCorner_->setText(QStringLiteral("%1 · %2").arg(modelDisplayName(path_),
                                                       projectSavedLabel()));
  docTabCorner_->setToolTip(
      projectPath_.isEmpty()
          ? QString()
          : QStringLiteral("프로젝트 파일: %1")
                .arg(QDir::toNativeSeparators(projectPath_)));
}

ProjectDoc MainWindow::captureProject() const {
  ProjectDoc p;
  p.modelPath = QFileInfo(path_).absoluteFilePath().toStdString();
  p.modelName = modelDisplayName(path_).toStdString();
  p.srsLabel = srsReport_.desc.shortAscii();
  p.sections = sections_;
  p.current = current_;
  p.front = front_->value();
  p.back = back_->value();
  p.backUserSet = backUserSet_;
  const Vec3 o = srs().origin;
  if (plan_->hasLine()) {
    const SectionLine &l = plan_->line();
    p.hasLine = true;
    p.ax = l.a.x + o.x;
    p.ay = l.a.y + o.y;
    p.bx = l.b.x + o.x;
    p.by = l.b.y + o.y;
  }
  p.opacity = opacity_ ? opacity_->value() : 100;
  p.lineWidthPx = QSettings().value("view/sectionLineWidth", 2.0).toDouble();
  p.smooth = action("smooth")->isChecked();
  p.depthFade = action("fade")->isChecked();
  if (plan_->metersPerPixel() > 0) {
    p.hasCamera = true;
    p.camX = plan_->cameraX() + o.x;
    p.camY = plan_->cameraY() + o.y;
    p.metersPerPixel = plan_->metersPerPixel();
  }
  { // 단면도 조판: 지금 연 조판 값(없으면 기본값 = 설정)
    const SheetParams sp = lastSheet_ ? *lastSheet_ : defaultSheetParams();
    ProjectSheet &s = p.sectionSheet;
    s.present = true;
    s.paper = sp.spec.paper == Paper::A3 ? 1 : 0;
    s.landscape = sp.spec.landscape;
    s.denom = sp.spec.denom;
    s.format = sp.format;
    s.dpi = sp.dpi;
    s.split = sp.split;
    s.withImage = sp.withImage;
    s.withLine = sp.withLine;
    s.withLevels = sp.withLevels;
    s.withTicks = sp.withTicks;
    s.withTitle = sp.withTitle;
    s.withPlan = sp.withPlan;
    s.levelMinorPt = sp.levelMinorPt;
    s.levelMajorPt = sp.levelMajorPt;
    s.hatchKind = sp.hatchKind;
    s.hatchMm = sp.hatchMm;
  }
  { // 평면도 조판: 저장된 적 있을 때만
    QSettings st;
    if (lastPlanSheet_ || st.contains("plansheet/denom")) {
      ProjectSheet &s = p.planSheet;
      s.present = true;
      if (lastPlanSheet_) {
        s.paper = lastPlanSheet_->spec.paper == Paper::A3 ? 1 : 0;
        s.landscape = lastPlanSheet_->spec.landscape;
        s.denom = lastPlanSheet_->spec.denom;
        s.format = lastPlanSheet_->format;
        s.dpi = lastPlanSheet_->dpi;
        s.split = lastPlanSheet_->split;
      } else {
        s.paper = st.value("plansheet/paper", 0).toInt();
        s.landscape = st.value("plansheet/landscape", true).toBool();
        s.denom = st.value("plansheet/denom", 100.0).toDouble();
        s.format = st.value("plansheet/format", 0).toInt();
        s.dpi = st.value("plansheet/dpi", 600.0).toDouble();
      }
    }
  }
  p.savedAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
  return p;
}

bool MainWindow::writeProject(const QString &path, QString *msg) {
  if (!src_ || path.isEmpty()) {
    if (msg)
      *msg = QStringLiteral("열린 모델이 없습니다");
    return false;
  }
  const std::string text = projectToJson(captureProject());
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (msg)
      *msg = QStringLiteral("쓸 수 없습니다: %1")
                 .arg(QDir::toNativeSeparators(path));
    return false;
  }
  f.write(text.data(), qint64(text.size()));
  f.close();
  projectPath_ = QFileInfo(path).absoluteFilePath();
  applyProjectHeader();
  if (msg)
    *msg = QStringLiteral("프로젝트 저장: %1")
               .arg(QDir::toNativeSeparators(projectPath_));
  return true;
}

bool MainWindow::readProjectFile(const QString &path, ProjectDoc &out,
                                 QString *msg) {
  std::string text, err;
  if (!readTextFile(path, &text)) {
    if (msg)
      *msg = QStringLiteral("읽을 수 없습니다: %1")
                 .arg(QDir::toNativeSeparators(path));
    return false;
  }
  if (!projectFromJson(text, out, &err)) {
    if (msg)
      *msg = qs8(err);
    return false;
  }
  return true;
}

// 문서 → 지금 상태. 모델이 열린 뒤(restoreModelState 안)에서만 부른다. 단면
// 계산은 requestSection 으로 작업 스레드에
void MainWindow::applyProject(const ProjectDoc &p) {
  const Vec3 o = srs().origin;
  sections_.clear();
  for (const SavedSection &s : p.sections) {
    SectionLine sl;
    sl.a = {s.ax - o.x, s.ay - o.y};
    sl.b = {s.bx - o.x, s.by - o.y};
    if (isValidLineForScene(sl))
      sections_.push_back(s);
  }
  current_ = (p.current >= 0 && p.current < int(sections_.size()))
                 ? p.current
                 : (sections_.empty() ? -1 : 0);
  if (pendingSection_ >= 0 && pendingSection_ < int(sections_.size()))
    current_ = pendingSection_; // 홈 썸네일로 연 단면
  pendingSection_ = -1;
  {
    QSignalBlocker b1(front_), b2(back_);
    front_->setValue(p.front);
    back_->setValue(p.back);
    backUserSet_ = p.backUserSet;
    plan_->setBand(front_->value(), back_->value());
  }
  if (opacity_) {
    QSignalBlocker b(opacity_);
    opacity_->setValue(std::clamp(p.opacity, 10, 100));
  }
  { // 평활 · 깊이 음영은 켜고 끌 때 단면을 다시 계산하므로 신호를 막고
    // 값만(계산은 아래 requestSection 한 번)
    QSignalBlocker b1(action("smooth")), b2(action("fade"));
    action("smooth")->setChecked(p.smooth);
    action("fade")->setChecked(p.depthFade);
    QSettings st;
    st.setValue("section/smooth", p.smooth);
    st.setValue("section/depthFade", p.depthFade);
  }
  setSectionLineWidth(p.lineWidthPx > 0 ? p.lineWidthPx : 2.0);
  if (p.sectionSheet.present) { // 조판 기본값(설정)에 넣어 두면
                                // defaultSheetParams 가 그대로 읽는다
    QSettings s;
    const ProjectSheet &h = p.sectionSheet;
    s.setValue("sheet/paper", h.paper);
    s.setValue("sheet/landscape", h.landscape);
    s.setValue("sheet/denom", h.denom);
    s.setValue("sheet/format", h.format);
    s.setValue("sheet/dpi", h.dpi);
    s.setValue("sheet/withImage", h.withImage);
    s.setValue("sheet/withLine", h.withLine);
    s.setValue("sheet/withLevels", h.withLevels);
    s.setValue("sheet/withTicks", h.withTicks);
    s.setValue("sheet/withTitle", h.withTitle);
    s.setValue("sheet/withPlan", h.withPlan);
    s.setValue("sheet/levelMinorPt", h.levelMinorPt);
    s.setValue("sheet/levelMajorPt", h.levelMajorPt);
    s.setValue("sheet/hatchKind", h.hatchKind);
    s.setValue("sheet/hatchMm", h.hatchMm);
    lastSheet_.reset();
  }
  if (p.planSheet.present) {
    QSettings s;
    const ProjectSheet &h = p.planSheet;
    s.setValue("plansheet/paper", h.paper);
    s.setValue("plansheet/landscape", h.landscape);
    s.setValue("plansheet/denom", h.denom);
    s.setValue("plansheet/format", h.format);
    s.setValue("plansheet/dpi", h.dpi);
    lastPlanSheet_.reset();
  }
  bool appliedLine = false;
  if (p.hasLine) {
    SectionLine l;
    l.a = {p.ax - o.x, p.ay - o.y};
    l.b = {p.bx - o.x, p.by - o.y};
    l.front = front_->value();
    l.back = back_->value();
    if (isValidLineForScene(l)) {
      applyingState_ = true;
      plan_->setLine(l, true);
      applyingState_ = false;
      fitNextResult_ = true;
      requestSection(true);
      appliedLine = true;
    }
  }
  if (!appliedLine) {
    section_->setStyle(secStyle());
  }
  if (p.hasCamera) {
    const double cx = p.camX - o.x, cy = p.camY - o.y, mpp = p.metersPerPixel;
    if (isValidCameraForScene(cx, cy, mpp)) {
      QTimer::singleShot(50, this, [this, cx, cy, mpp] {
        if (src_)
          plan_->setCamera(cx, cy, mpp);
      });
    }
  }
}

// .trench.json 을 읽어 적용 대기(pendingProject_)에 올리고 모델 파일 경로를
// 돌려준다. 모델이 그 경로에 없으면 프로젝트 파일 옆에서 같은 이름을, 그래도
// 없으면(ask) 사용자에게 묻는다(다른 PC 로 모델 + 프로젝트를 복사한 경우).
// 명령줄(ask=false)은 못 찾으면 false
bool MainWindow::stageProject(const QString &path, QString *modelPath,
                              QString *msg, bool ask) {
  auto doc = std::make_shared<ProjectDoc>();
  if (!readProjectFile(path, *doc, msg))
    return false;
  QString model = qs8(doc->modelPath);
  if (!QFileInfo::exists(model)) {
    const QString beside = QFileInfo(path).absolutePath() + "/" +
                           QFileInfo(qs8(doc->modelPath)).fileName();
    if (QFileInfo::exists(beside))
      model = beside;
    else if (!ask) {
      if (msg)
        *msg = QStringLiteral("모델 파일이 없습니다: %1")
                   .arg(QDir::toNativeSeparators(model));
      return false;
    } else {
      const QString picked = QFileDialog::getOpenFileName(
          this,
          QStringLiteral("모델 파일을 찾을 수 없습니다 — %1 의 위치를 고르세요")
              .arg(qs8(doc->modelName)),
          QFileInfo(path).absolutePath(),
          QStringLiteral("실사 메시 (*.3mx *.3sm *.obj);;모든 파일 (*.*)"));
      if (picked.isEmpty()) {
        if (msg)
          *msg = QStringLiteral("취소");
        return false;
      }
      model = picked;
    }
  }
  pendingProject_ = doc;
  pendingProjectPath_ = QFileInfo(path).absoluteFilePath();
  if (modelPath)
    *modelPath = model;
  return true;
}

// .trench.json 열기: 모델 파일을 찾아 열고, 열린 뒤 restoreModelState 가
// pendingProject_ 를 적용한다
bool MainWindow::openProjectFile(const QString &path) {
  QString model, m;
  if (!stageProject(path, &model, &m, true)) {
    if (m != QStringLiteral("취소"))
      QMessageBox::warning(
          this, windowTitle(),
          QStringLiteral("프로젝트를 열 수 없습니다:\n%1\n\n%2")
              .arg(QDir::toNativeSeparators(path), m));
    return false;
  }
  openFile(model);
  return true;
}

void MainWindow::saveProjectNow() {
  QString m;
  const bool ok = writeProject(autoProjectPath(), &m);
  report(ok, m);
}

void MainWindow::saveProjectAs() {
  if (!src_)
    return;
  QSettings st;
  const QString start =
      st.value("dir/project", projectDir()).toString() + "/" +
      qs8(projectFileName(modelDisplayName(path_).toStdString()));
  const QString fn = QFileDialog::getSaveFileName(
      this, QStringLiteral("프로젝트를 다른 이름으로 저장"), start,
      QStringLiteral("Trench 프로젝트 (*.trench.json)"));
  if (fn.isEmpty())
    return;
  st.setValue("dir/project", QFileInfo(fn).absolutePath());
  QString m;
  const bool ok = writeProject(fn, &m);
  report(ok, m);
}

void MainWindow::openProjectDialog() {
  QSettings st;
  const QString fn = QFileDialog::getOpenFileName(
      this, QStringLiteral("프로젝트 열기"),
      st.value("dir/project", projectDir()).toString(),
      QStringLiteral("Trench 프로젝트 (*.trench.json);;모든 파일 (*.*)"));
  if (fn.isEmpty())
    return;
  st.setValue("dir/project", QFileInfo(fn).absolutePath());
  openProjectFile(fn);
}
