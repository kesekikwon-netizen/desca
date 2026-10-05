#include "planview.hpp"
#include <QElapsedTimer>
#include <QTimer>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <cmath>
#include "theme.hpp"

using namespace asec;

// ---------------- HeightIndex ----------------
void HeightIndex::build(const std::vector<std::shared_ptr<DisplayMesh>>& ms, double cell) {
    tris_.clear(); cells_.clear();
    double xmin = 1e300, ymin = 1e300, xmax = -1e300, ymax = -1e300;
    for (auto& m : ms)
        for (size_t t = 0; t + 2 < m->idx.size(); t += 3) {
            Tri T;
            for (int k = 0; k < 3; ++k) { const float* p = &m->pos[3 * m->idx[t + k]]; T.x[k] = p[0]; T.y[k] = p[1]; T.z[k] = p[2];
                xmin = std::min(xmin, double(p[0])); xmax = std::max(xmax, double(p[0])); ymin = std::min(ymin, double(p[1])); ymax = std::max(ymax, double(p[1])); }
            tris_.push_back(T);
        }
    if (tris_.empty()) return;
    cell_ = std::max(cell, 1e-3);
    x0_ = xmin; y0_ = ymin;
    nx_ = std::max(1, std::min(2048, int((xmax - xmin) / cell_) + 1));
    ny_ = std::max(1, std::min(2048, int((ymax - ymin) / cell_) + 1));
    cell_ = std::max((xmax - xmin) / nx_, (ymax - ymin) / ny_) * 1.0001 + 1e-9;
    cells_.assign(size_t(nx_) * ny_, {});
    for (uint32_t i = 0; i < tris_.size(); ++i) {
        auto& T = tris_[i];
        int cx0 = int((std::min({T.x[0], T.x[1], T.x[2]}) - x0_) / cell_), cx1 = int((std::max({T.x[0], T.x[1], T.x[2]}) - x0_) / cell_);
        int cy0 = int((std::min({T.y[0], T.y[1], T.y[2]}) - y0_) / cell_), cy1 = int((std::max({T.y[0], T.y[1], T.y[2]}) - y0_) / cell_);
        for (int cy = std::max(0, cy0); cy <= std::min(ny_ - 1, cy1); ++cy)
            for (int cx = std::max(0, cx0); cx <= std::min(nx_ - 1, cx1); ++cx) cells_[size_t(cy) * nx_ + cx].push_back(i);
    }
}

bool HeightIndex::z(double x, double y, double& out) const {
    if (cells_.empty()) return false;
    int cx = int((x - x0_) / cell_), cy = int((y - y0_) / cell_);
    if (cx < 0 || cy < 0 || cx >= nx_ || cy >= ny_) return false;
    bool found = false; double best = -1e300;
    for (uint32_t i : cells_[size_t(cy) * nx_ + cx]) {
        auto& T = tris_[i];
        double d = (T.y[1] - T.y[2]) * (T.x[0] - T.x[2]) + (T.x[2] - T.x[1]) * (T.y[0] - T.y[2]);
        if (std::fabs(d) < 1e-14) continue;
        double a = ((T.y[1] - T.y[2]) * (x - T.x[2]) + (T.x[2] - T.x[1]) * (y - T.y[2])) / d;
        double b = ((T.y[2] - T.y[0]) * (x - T.x[2]) + (T.x[0] - T.x[2]) * (y - T.y[2])) / d;
        double c = 1 - a - b;
        if (a < -1e-6 || b < -1e-6 || c < -1e-6) continue;
        double z = a * T.z[0] + b * T.z[1] + c * T.z[2];
        if (z > best) { best = z; found = true; }
    }
    if (found) out = best;
    return found;
}

// ---------------- PlanView ----------------
PlanView::PlanView(QWidget* parent) : QOpenGLWidget(parent) {
    QSurfaceFormat f = format();
    f.setSamples(4);
    f.setDepthBufferSize(24);
    setFormat(f);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
}

PlanView::~PlanView() {
    makeCurrent();
    freeGpu();
    prog_.reset();
    doneCurrent();
}

void PlanView::freeGpu() {
    for (auto& g : gpu_) {
        if (g.vbo) glDeleteBuffers(1, &g.vbo);
        if (g.ibo) glDeleteBuffers(1, &g.ibo);
        if (g.tex) glDeleteTextures(1, &g.tex);
    }
    gpu_.clear();
}

void PlanView::clearScene() {
    makeCurrent(); freeGpu(); doneCurrent();
    meshes_.clear(); hasLine_ = false; update();
}

void PlanView::setScene(std::vector<std::shared_ptr<DisplayMesh>> meshes, const Vec3& center, const Box3& bounds, const SrsInfo& srs) {
    makeCurrent(); freeGpu(); doneCurrent();
    meshes_ = std::move(meshes);
    center_ = center; bounds_ = bounds; srs_ = srs;
    hidx_.build(meshes_, std::max(0.05, std::max(bounds.mx.x - bounds.mn.x, bounds.mx.y - bounds.mn.y) / 512.0));
    needUpload_ = true;
    hasLine_ = false;
    fitAll();
}

void PlanView::initializeGL() {
    initializeOpenGLFunctions();
    prog_ = std::make_unique<QOpenGLShaderProgram>();
    prog_->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "attribute vec3 aPos; attribute vec2 aUv; attribute vec3 aNrm;\n"
        "uniform mat4 uMvp; varying vec2 vUv; varying float vShade;\n"
        "void main(){ gl_Position = uMvp * vec4(aPos, 1.0); vUv = aUv;\n"
        "  vShade = 0.55 + 0.45 * max(dot(normalize(aNrm + vec3(0.0,0.0,1e-6)), normalize(vec3(0.3, 0.4, 0.85))), 0.0); }\n");
    prog_->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#ifdef GL_ES\nprecision mediump float;\n#endif\n"
        "uniform sampler2D uTex; uniform float uHasTex; varying vec2 vUv; varying float vShade;\n"
        "void main(){ vec4 c = texture2D(uTex, vUv); vec3 g = vec3(0.80, 0.78, 0.74) * vShade;\n"
        "  gl_FragColor = vec4(mix(g, c.rgb, uHasTex), 1.0); }\n");
    prog_->bindAttributeLocation("aPos", 0);
    prog_->bindAttributeLocation("aUv", 1);
    prog_->bindAttributeLocation("aNrm", 2);
    prog_->link();
}

void PlanView::upload() {
    // 프레임마다 약 12 ms 만큼만 GPU 로 올림 → 큰 모델을 열어도 화면이 멈추지 않고 차례로 채워짐
    QElapsedTimer t; t.start();
    while (gpu_.size() < meshes_.size() && (gpu_.empty() || t.elapsed() < 12)) {
        auto& m = meshes_[gpu_.size()];
        Gpu g;
        size_t n = m->pos.size() / 3;
        std::vector<float> inter(n * 8);
        for (size_t i = 0; i < n; ++i) {
            inter[i * 8 + 0] = m->pos[3 * i]; inter[i * 8 + 1] = m->pos[3 * i + 1]; inter[i * 8 + 2] = m->pos[3 * i + 2];
            inter[i * 8 + 3] = m->uv.size() >= 2 * n ? m->uv[2 * i] : 0.f; inter[i * 8 + 4] = m->uv.size() >= 2 * n ? m->uv[2 * i + 1] : 0.f;
            inter[i * 8 + 5] = m->nrm.size() >= 3 * n ? m->nrm[3 * i] : 0.f; inter[i * 8 + 6] = m->nrm.size() >= 3 * n ? m->nrm[3 * i + 1] : 0.f;
            inter[i * 8 + 7] = m->nrm.size() >= 3 * n ? m->nrm[3 * i + 2] : 1.f;
        }
        glGenBuffers(1, &g.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(inter.size() * 4), inter.data(), GL_STATIC_DRAW);
        glGenBuffers(1, &g.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(m->idx.size() * 4), m->idx.data(), GL_STATIC_DRAW);
        g.count = GLsizei(m->idx.size());
        if (!m->tex.isNull()) {
            QImage im = m->tex.convertToFormat(QImage::Format_RGBA8888).mirrored();  // GL 행0 = 아래 = v 0
            glGenTextures(1, &g.tex);
            glBindTexture(GL_TEXTURE_2D, g.tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.width(), im.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, im.constBits());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            g.hasTex = true;
            m->tex = QImage();  // GPU 로 올렸으니 메모리 해제
        }
        gpu_.push_back(g);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    needUpload_ = gpu_.size() < meshes_.size();
}

void PlanView::resizeGL(int, int) { updateMatrices(); }

double PlanView::refZ() const { return bounds_.valid() ? bounds_.mx.z - center_.z : 0.0; }

void PlanView::updateMatrices() {
    double diag = std::max(1.0, bounds_.diag());
    double w = width() * mpp_, h = height() * mpp_;
    proj_.setToIdentity();
    proj_.ortho(float(-w / 2), float(w / 2), float(-h / 2), float(h / 2), float(-diag * 4), float(diag * 4));
    view_.setToIdentity();
    view_.rotate(float(-(90.0 - pitch_)), 1, 0, 0);
    view_.rotate(float(-yaw_), 0, 0, 1);
    view_.translate(-target_);
}

void PlanView::fitAll() {
    if (!bounds_.valid()) return;
    double w = bounds_.mx.x - bounds_.mn.x, h = bounds_.mx.y - bounds_.mn.y;
    target_ = QVector3D(float(bounds_.center().x - center_.x), float(bounds_.center().y - center_.y), float(bounds_.center().z - center_.z));
    double W = std::max(50, width() - 40), H = std::max(50, height() - 40);
    mpp_ = std::max(1e-4, std::max(w / W, h / H) * 1.04);
    updateMatrices();
    update();
}

void PlanView::topView() { yaw_ = 0; pitch_ = 90; updateMatrices(); update(); }

void PlanView::paintGL() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!meshes_.empty()) {
        if (needUpload_) upload();
        updateMatrices();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        prog_->bind();
        prog_->setUniformValue("uMvp", proj_ * view_);
        prog_->setUniformValue("uTex", 0);
        glActiveTexture(GL_TEXTURE0);
        for (auto& g : gpu_) {
            glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ibo);
            glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, reinterpret_cast<void*>(0));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, reinterpret_cast<void*>(12));
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, reinterpret_cast<void*>(20));
            glBindTexture(GL_TEXTURE_2D, g.tex);
            prog_->setUniformValue("uHasTex", g.hasTex ? 1.0f : 0.0f);
            glDrawElements(GL_TRIANGLES, g.count, GL_UNSIGNED_INT, nullptr);
        }
        glDisableVertexAttribArray(0); glDisableVertexAttribArray(1); glDisableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        prog_->release();
        glDisable(GL_DEPTH_TEST);
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    if (meshes_.empty()) paintEmpty(p);
    else paintOverlay(p);
    if (needUpload_) QTimer::singleShot(0, this, [this] { update(); });  // 남은 메시 다음 프레임에
}

QPointF PlanView::localToScreen(double x, double y, double z) const {
    QVector4D c = (proj_ * view_) * QVector4D(float(x - center_.x), float(y - center_.y), float(z - center_.z), 1.0f);
    if (std::fabs(c.w()) < 1e-12f) return {};
    return QPointF((c.x() / c.w() * 0.5 + 0.5) * width(), (1.0 - (c.y() / c.w() * 0.5 + 0.5)) * height());
}

bool PlanView::screenToLocalXY(const QPointF& sp, Vec2& out) const {
    bool inv = false;
    QMatrix4x4 m = (proj_ * view_).inverted(&inv);
    if (!inv) return false;
    float nx = float(sp.x() / width() * 2 - 1), ny = float(1 - sp.y() / height() * 2);
    QVector4D a = m * QVector4D(nx, ny, -1, 1), b = m * QVector4D(nx, ny, 1, 1);
    QVector3D A = a.toVector3DAffine(), B = b.toVector3DAffine();
    double zr = refZ();
    double dz = B.z() - A.z();
    double t = std::fabs(dz) < 1e-9 ? 0 : (zr - A.z()) / dz;
    out = Vec2(A.x() + t * (B.x() - A.x()) + center_.x, A.y() + t * (B.y() - A.y()) + center_.y);
    return true;
}

void PlanView::paintEmpty(QPainter& p) {
    QRectF r(0, 0, std::min(440, width() - 40), 150);
    r.moveCenter(QPointF(width() / 2.0, height() / 2.0));
    p.setPen(QPen(theme::Line, 1)); p.setBrush(theme::Card);
    p.drawRoundedRect(r, 14, 14);
    QFont f(theme::fontFamily()); f.setPointSizeF(12); f.setBold(true); p.setFont(f); p.setPen(theme::Ink);
    p.drawText(r.adjusted(20, 22, -20, -80), Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("3MX 실사 메시를 여세요"));
    f.setPointSizeF(9.5); f.setBold(false); p.setFont(f); p.setPen(theme::InkSub);
    p.drawText(r.adjusted(20, 58, -20, -10), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
               QStringLiteral("리본의 「열기」(Ctrl+O) 또는 .3mx / .obj 파일을 이 창에 끌어다 놓기\niTwin Capture 결과 폴더의 .3mx 를 고르면 됩니다"));
}

void PlanView::paintOverlay(QPainter& p) {
    // 북쪽 표시 + 축척 막대
    {
        QPointF c(width() - 34, 40);
        double a = -yaw_ * M_PI / 180.0;
        QPointF dir(std::sin(a), -std::cos(a));
        QPointF tip = c + dir * 16, tail = c - dir * 12, side(dir.y(), -dir.x());
        QPainterPath arr; arr.moveTo(tip); arr.lineTo(tail + side * 8); arr.lineTo(c - dir * 5); arr.lineTo(tail - side * 8); arr.closeSubpath();
        p.setPen(QPen(theme::Ink, 1.2)); p.setBrush(theme::Card); p.drawEllipse(c, 22, 22);
        p.setBrush(theme::Ink); p.drawPath(arr);
        QFont f(theme::fontFamily()); f.setPointSizeF(7.5); f.setBold(true); p.setFont(f); p.setPen(theme::Brand);
        p.drawText(QRectF(c.x() - 20, c.y() - 36, 40, 12), Qt::AlignCenter, "N");
        double len = niceStep(mpp_ * 120, 1);
        double px = len / mpp_;
        QRectF sb(16, height() - 28, px, 6);
        p.setPen(QPen(theme::Ink, 1)); p.setBrush(Qt::white); p.drawRect(sb);
        p.setBrush(theme::Ink); p.drawRect(QRectF(sb.x(), sb.y(), px / 2, 6));
        f.setBold(false); f.setPointSizeF(8); p.setFont(f); p.setPen(theme::Ink);
        QString lab = len >= 1 ? QString::number(len, 'g', 4) + " m" : QString::number(len * 100, 'g', 4) + " cm";
        QRectF lr(sb.right() + 6, sb.y() - 6, 80, 18);
        p.drawText(lr, Qt::AlignLeft | Qt::AlignVCenter, lab);
    }
    if (!hasLine_ && drawStage_ < 1) {
        if (drawStage_ == 0) {
            QFont f(theme::fontFamily()); f.setPointSizeF(9.5); p.setFont(f);
            QRectF r(0, 12, 330, 30); r.moveLeft(width() / 2.0 - 165);
            p.setPen(Qt::NoPen); p.setBrush(QColor(17, 17, 17, 220)); p.drawRoundedRect(r, 15, 15);
            p.setPen(Qt::white); p.drawText(r, Qt::AlignCenter, QStringLiteral("단면 시작점 A 를 클릭하세요  ·  Esc 취소"));
        }
        return;
    }
    SectionFrame f(line_);
    double z = refZ() + center_.z;
    auto S = [&](const Vec2& q) { return localToScreen(q.x, q.y, z); };
    QPointF A = S(line_.a), B = S(line_.b);
    // 두께 띠: 빨간 괄호선(Descartes 와 같은 표시)
    if (f.L > 1e-6) {
        QPen bp(theme::SectionRed, 1.4); bp.setCosmetic(true);
        p.setPen(bp); p.setBrush(QColor(255, 0, 0, 22));
        QPointF q0 = S(f.planXY(0, -line_.front)), q1 = S(f.planXY(f.L, -line_.front)), q2 = S(f.planXY(f.L, line_.back)), q3 = S(f.planXY(0, line_.back));
        QPolygonF band; band << q0 << q1 << q2 << q3;
        p.setPen(Qt::NoPen); p.drawPolygon(band);
        p.setPen(bp);
        QPointF ext = (B - A); double el = std::hypot(ext.x(), ext.y()); QPointF eu = el > 0 ? ext / el : QPointF(1, 0);
        double tick = 10;
        if (line_.back > 1e-6) {
            p.drawLine(q3, q2);
            p.drawLine(q3, q3 - (q3 - A) * (tick / std::max(1.0, std::hypot((q3 - A).x(), (q3 - A).y()))));
            p.drawLine(q2, q2 - (q2 - B) * (tick / std::max(1.0, std::hypot((q2 - B).x(), (q2 - B).y()))));
        }
        if (line_.front > 1e-6) {
            p.drawLine(q0, q1);
            p.drawLine(q0, q0 - (q0 - A) * (tick / std::max(1.0, std::hypot((q0 - A).x(), (q0 - A).y()))));
            p.drawLine(q1, q1 - (q1 - B) * (tick / std::max(1.0, std::hypot((q1 - B).x(), (q1 - B).y()))));
        }
        (void)eu;
        // 보는 방향 화살표(가운데, +n 쪽)
        QPointF mid = (A + B) / 2, nTip = S(f.planXY(f.L / 2, std::max(line_.back, 0.0)));
        QPointF nd = nTip - mid; double nl = std::hypot(nd.x(), nd.y());
        QPointF nu = nl > 1e-6 ? nd / nl : QPointF(-eu.y(), eu.x());
        QPointF a0 = mid + nu * 6, a1 = mid + nu * 22, sd(nu.y(), -nu.x());
        QPen ap(theme::SectionRed, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin); p.setPen(ap);
        p.drawLine(a0, a1); p.drawLine(a1, a1 - nu * 6 + sd * 4); p.drawLine(a1, a1 - nu * 6 - sd * 4);
    }
    // 단면선: 흰 테두리 + 빨강
    p.setPen(QPen(QColor(255, 255, 255, 200), 5.0, Qt::SolidLine, Qt::RoundCap)); p.drawLine(A, B);
    p.setPen(QPen(theme::SectionRed, 2.0, Qt::SolidLine, Qt::RoundCap)); p.drawLine(A, B);
    // 손잡이 + 이름
    QFont fnt(theme::fontFamily()); fnt.setPointSizeF(10); fnt.setBold(true); p.setFont(fnt);
    QPointF d = B - A; double dl = std::hypot(d.x(), d.y()); QPointF du = dl > 0 ? d / dl : QPointF(1, 0);
    auto handle = [&](QPointF c, const QString& name, QPointF away) {
        p.setPen(QPen(theme::SectionRed, 2)); p.setBrush(Qt::white); p.drawEllipse(c, 6.5, 6.5);
        QRectF tr(0, 0, 28, 20); tr.moveCenter(c + away * 18);
        p.setPen(Qt::NoPen); p.setBrush(QColor(255, 255, 255, 230)); p.drawRoundedRect(tr, 6, 6);
        p.setPen(theme::SectionRed); p.drawText(tr, Qt::AlignCenter, name);
    };
    handle(A, "A", -du);
    if (hasLine_ || drawStage_ == 1) handle(B, QStringLiteral("A′"), du);
    if (hasLine_) { p.setPen(QPen(theme::SectionRed, 1.5)); p.setBrush(theme::SectionRed); p.drawEllipse((A + B) / 2, 3.5, 3.5); }
    if (drawStage_ == 1) {
        QFont f2(theme::fontFamily()); f2.setPointSizeF(9.5); p.setFont(f2);
        QRectF r(0, 12, 360, 30); r.moveLeft(width() / 2.0 - 180);
        p.setPen(Qt::NoPen); p.setBrush(QColor(17, 17, 17, 220)); p.drawRoundedRect(r, 15, 15);
        p.setPen(Qt::white);
        p.drawText(r, Qt::AlignCenter, QStringLiteral("끝점 A′ 를 클릭하세요  ·  길이 %1 m").arg(f.L, 0, 'f', 2));
    }
}

void PlanView::setDrawMode(bool on) {
    drawStage_ = on ? 0 : -1;
    setCursor(on ? Qt::CrossCursor : Qt::ArrowCursor);
    if (onDrawModeChanged) onDrawModeChanged(on);
    update();
}

void PlanView::setLine(const SectionLine& l, bool has) { line_ = l; hasLine_ = has; update(); }
void PlanView::setBand(double front, double back) { line_.front = front; line_.back = back; update(); }

void PlanView::mousePressEvent(QMouseEvent* e) {
    lastMouse_ = e->pos();
    if (meshes_.empty()) { if (e->button() == Qt::LeftButton && onOpenRequest) onOpenRequest(); return; }
    Vec2 w;
    bool ok = screenToLocalXY(e->position(), w);
    if (e->button() == Qt::LeftButton && drawStage_ >= 0 && ok) {
        if (drawStage_ == 0) { line_.a = w; line_.b = w; drawStage_ = 1; hasLine_ = false; }
        else {
            line_.b = w;
            if (SectionFrame(line_).L > 1e-3) {
                hasLine_ = true; drawStage_ = -1; setCursor(Qt::ArrowCursor);
                if (onDrawModeChanged) onDrawModeChanged(false);
                emitLine(true);
            }
        }
        update();
        return;
    }
    if (e->button() == Qt::LeftButton && hasLine_) {
        double z = refZ() + center_.z;
        QPointF A = localToScreen(line_.a.x, line_.a.y, z), B = localToScreen(line_.b.x, line_.b.y, z);
        auto near = [&](QPointF q) { return std::hypot(q.x() - e->position().x(), q.y() - e->position().y()) < 11; };
        dragHandle_ = near(A) ? 0 : near(B) ? 1 : near((A + B) / 2) ? 2 : -1;
        if (dragHandle_ >= 0) { dragStartMouse_ = e->position(); dragStartLine_ = line_; setCursor(Qt::ClosedHandCursor); return; }
    }
    if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) { panning_ = true; setCursor(Qt::ClosedHandCursor); }
    if (e->button() == Qt::RightButton) { orbiting_ = true; setCursor(Qt::SizeAllCursor); }
}

void PlanView::mouseMoveEvent(QMouseEvent* e) {
    QPoint d = e->pos() - lastMouse_;
    lastMouse_ = e->pos();
    Vec2 w;
    bool ok = !meshes_.empty() && screenToLocalXY(e->position(), w);
    if (ok && onCursor) {
        double zz = 0;
        bool hz = hidx_.z(w.x - center_.x, w.y - center_.y, zz);
        Vec3 W = srs_.toWorld(Vec3(w.x, w.y, zz + center_.z));
        onCursor(W.x, W.y, W.z, hz, true);
    }
    if (drawStage_ == 1 && ok) { line_.b = w; update(); if (SectionFrame(line_).L > 0.05 && onLineChanged) onLineChanged(line_, false); return; }
    if (dragHandle_ >= 0 && ok) {
        Vec2 w0;
        screenToLocalXY(dragStartMouse_, w0);
        Vec2 dd = w - w0;
        line_ = dragStartLine_;
        if (dragHandle_ == 0) line_.a = dragStartLine_.a + dd;
        else if (dragHandle_ == 1) line_.b = dragStartLine_.b + dd;
        else { line_.a = dragStartLine_.a + dd; line_.b = dragStartLine_.b + dd; }
        update();
        emitLine(false);
        return;
    }
    if (panning_) {
        QMatrix4x4 rot; rot.rotate(float(yaw_), 0, 0, 1); rot.rotate(float(90.0 - pitch_), 1, 0, 0);
        QVector3D mv = rot.map(QVector3D(float(-d.x() * mpp_), float(d.y() * mpp_), 0));
        target_ += mv;
        update();
        return;
    }
    if (orbiting_) {
        yaw_ = std::fmod(yaw_ - d.x() * 0.4 + 360.0, 360.0);
        pitch_ = std::clamp(pitch_ - d.y() * 0.3, 8.0, 90.0);
        update();
        return;
    }
    if (hasLine_ && !meshes_.empty()) {
        double z = refZ() + center_.z;
        QPointF A = localToScreen(line_.a.x, line_.a.y, z), B = localToScreen(line_.b.x, line_.b.y, z);
        auto near = [&](QPointF q) { return std::hypot(q.x() - e->position().x(), q.y() - e->position().y()) < 11; };
        setCursor(drawStage_ >= 0 ? Qt::CrossCursor : (near(A) || near(B) || near((A + B) / 2)) ? Qt::OpenHandCursor : Qt::ArrowCursor);
    }
}

void PlanView::mouseReleaseEvent(QMouseEvent*) {
    if (dragHandle_ >= 0) { dragHandle_ = -1; emitLine(true); }
    panning_ = orbiting_ = false;
    setCursor(drawStage_ >= 0 ? Qt::CrossCursor : Qt::ArrowCursor);
}

void PlanView::mouseDoubleClickEvent(QMouseEvent* e) {
    if (drawStage_ < 0 && e->button() == Qt::MiddleButton) fitAll();
}

void PlanView::wheelEvent(QWheelEvent* e) {
    if (meshes_.empty()) return;
    Vec2 before; bool ok = screenToLocalXY(e->position(), before);
    double f = std::pow(0.85, e->angleDelta().y() / 120.0);
    mpp_ = std::clamp(mpp_ * f, 1e-4, 1e4);
    updateMatrices();
    Vec2 after;
    if (ok && screenToLocalXY(e->position(), after)) {
        target_ += QVector3D(float(before.x - after.x), float(before.y - after.y), 0);
        updateMatrices();
    }
    update();
}

void PlanView::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape && drawStage_ >= 0) { drawStage_ = -1; setCursor(Qt::ArrowCursor); if (onDrawModeChanged) onDrawModeChanged(false); update(); return; }
    QOpenGLWidget::keyPressEvent(e);
}

void PlanView::leaveEvent(QEvent*) { if (onCursor) onCursor(0, 0, 0, false, false); }

bool PlanView::viewRectLocal(Box3& out) const {
    if (meshes_.empty()) return false;
    out = Box3();
    for (QPointF c : {QPointF(0, 0), QPointF(width(), 0), QPointF(0, height()), QPointF(width(), height())}) {
        Vec2 w;
        if (!screenToLocalXY(c, w)) return false;
        out.add(Vec3(w.x, w.y, bounds_.mn.z));
    }
    out.mn.z = bounds_.mn.z; out.mx.z = bounds_.mx.z;
    return true;
}

void PlanView::zoomBy(double f) { mpp_ = std::clamp(mpp_ * f, 1e-5, 1e5); updateMatrices(); update(); }
