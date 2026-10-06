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
#include <QSettings>
#include "stb_image.h"

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

// ---------------- 스트리밍 노드 데이터(작업 스레드에서 만듦) ----------------
namespace {
struct NodePrep {
    struct Part { std::vector<float> inter; std::vector<uint32_t> idx; QImage tex; };
    std::vector<Part> parts;
};

/// 3MX 노드 메시 → GPU 업로드용(장면 중심 기준 float, 법선, 텍스처 RGBA). 작업 스레드에서 호출
std::shared_ptr<void> prepareNode(const TmxNode& node, const Vec3& c, size_t* bytes) {
    auto out = std::make_shared<NodePrep>();
    size_t b = 0;
    std::unordered_map<const Texture*, QImage> texCache;
    for (auto& mp : node.meshes) {
        const Mesh& m = *mp;
        size_t n = m.vertexCount();
        if (n == 0 || m.idx.empty()) continue;
        NodePrep::Part P;
        std::vector<float> nrm(n * 3, 0.f);
        for (size_t t = 0; t + 2 < m.idx.size(); t += 3) {
            uint32_t a = m.idx[t], bb = m.idx[t + 1], e = m.idx[t + 2];
            if (a >= n || bb >= n || e >= n) continue;
            double ux = m.pos[3 * bb] - m.pos[3 * a], uy = m.pos[3 * bb + 1] - m.pos[3 * a + 1], uz = m.pos[3 * bb + 2] - m.pos[3 * a + 2];
            double vx = m.pos[3 * e] - m.pos[3 * a], vy = m.pos[3 * e + 1] - m.pos[3 * a + 1], vz = m.pos[3 * e + 2] - m.pos[3 * a + 2];
            float nx = float(uy * vz - uz * vy), ny = float(uz * vx - ux * vz), nz = float(ux * vy - uy * vx);
            if (nz < 0) { nx = -nx; ny = -ny; nz = -nz; }
            for (uint32_t k : {a, bb, e}) { nrm[3 * k] += nx; nrm[3 * k + 1] += ny; nrm[3 * k + 2] += nz; }
        }
        P.inter.resize(n * 8);
        bool hasUv = m.uv.size() >= 2 * n;
        for (size_t i = 0; i < n; ++i) {
            float* q = &P.inter[i * 8];
            q[0] = float(m.pos[3 * i] - c.x); q[1] = float(m.pos[3 * i + 1] - c.y); q[2] = float(m.pos[3 * i + 2] - c.z);
            q[3] = hasUv ? m.uv[2 * i] : 0.f; q[4] = hasUv ? m.uv[2 * i + 1] : 0.f;
            q[5] = nrm[3 * i]; q[6] = nrm[3 * i + 1]; q[7] = nrm[3 * i + 2];
        }
        P.idx = m.idx;
        if (m.texture) {
            auto it = texCache.find(m.texture.get());
            if (it == texCache.end()) {
                QImage q;
                const Texture& t = *m.texture;
                if (!t.rgba.empty()) q = QImage(t.rgba.px.data(), t.rgba.w, t.rgba.h, t.rgba.w * 4, QImage::Format_RGBA8888).copy();
                else if (!t.encoded.empty()) {
                    int w, h, cc;
                    unsigned char* px = stbi_load_from_memory(t.encoded.data(), int(t.encoded.size()), &w, &h, &cc, 4);
                    if (px) { q = QImage(px, w, h, w * 4, QImage::Format_RGBA8888).copy(); stbi_image_free(px); }
                }
                if (!q.isNull() && std::max(q.width(), q.height()) > 2048) q = q.scaled(2048, 2048, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                if (!q.isNull()) q = q.mirrored();  // GL 행0 = 아래 = v 0
                it = texCache.emplace(m.texture.get(), q).first;
            }
            P.tex = it->second;
            if (!P.tex.isNull()) b += size_t(P.tex.width()) * P.tex.height() * 4 * 4 / 3;  // 밉맵 포함
        }
        b += P.inter.size() * 4 + P.idx.size() * 4;
        out->parts.push_back(std::move(P));
    }
    if (out->parts.empty()) return nullptr;
    *bytes = b;
    return out;
}

void uploadPart(QOpenGLFunctions* gl, const float* inter, size_t nFloats, const uint32_t* idx, size_t nIdx, const QImage& texGl, GLuint& vbo, GLuint& ibo, GLuint& tex) {
    gl->glGenBuffers(1, &vbo);
    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(nFloats * 4), inter, GL_STATIC_DRAW);
    gl->glGenBuffers(1, &ibo);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(nIdx * 4), idx, GL_STATIC_DRAW);
    if (!texGl.isNull()) {
        gl->glGenTextures(1, &tex);
        gl->glBindTexture(GL_TEXTURE_2D, tex);
        gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texGl.width(), texGl.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGl.constBits());
        gl->glGenerateMipmap(GL_TEXTURE_2D);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}
}  // namespace

size_t PlanView::gpuBudgetBytes() {
    int mb = QSettings().value("view/gpuBudgetMB", 768).toInt();
    if (qEnvironmentVariableIsSet("SECTIONVIEWER_GPU_BUDGET_MB")) mb = qEnvironmentVariableIntValue("SECTIONVIEWER_GPU_BUDGET_MB");
    return size_t(std::clamp(mb, 64, 16384)) << 20;
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
    streamer_.reset();  // 작업 스레드 정지(콜백이 this 를 쓰므로 먼저)
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
    for (auto& kv : gpuNodes_) freeGpuNode(kv.second);
    gpuNodes_.clear();
    gpuNodeBytes_ = 0;
}

void PlanView::freeGpuNode(std::vector<Gpu>& v) {
    for (auto& g : v) {
        if (g.vbo) glDeleteBuffers(1, &g.vbo);
        if (g.ibo) glDeleteBuffers(1, &g.ibo);
        if (g.tex) glDeleteTextures(1, &g.tex);
        gpuNodeBytes_ -= std::min(gpuNodeBytes_, g.bytes);
    }
    v.clear();
}

void PlanView::clearScene() {
    streamer_.reset();
    makeCurrent(); freeGpu(); doneCurrent();
    meshes_.clear(); hidx_ = HeightIndex(); hasScene_ = false; hasLine_ = false; lastFrame_ = FrameInfo(); update();
}

void PlanView::setStreamingScene(const std::vector<fs::path>& roots, const Vec3& center, const Box3& bounds, const SrsInfo& srs,
                                 std::vector<std::shared_ptr<DisplayMesh>> heightMeshes) {
    streamer_.reset();
    makeCurrent(); freeGpu(); doneCurrent();
    meshes_.clear();
    center_ = center; bounds_ = bounds; srs_ = srs;
    hidx_ = HeightIndex();
    if (!heightMeshes.empty()) hidx_.build(heightMeshes, std::max(0.05, std::max(bounds.mx.x - bounds.mn.x, bounds.mx.y - bounds.mn.y) / 512.0));
    lodBias_ = std::clamp(QSettings().value("view/lodBias", 1.0).toDouble(), 0.25, 4.0);
    LodStreamer::Config cfg;
    cfg.gpuBudgetBytes = gpuBudgetBytes();
    cfg.cpuBudgetBytes = std::min<size_t>(cfg.gpuBudgetBytes / 2, size_t(512) << 20);
    cfg.threads = int(std::clamp(std::thread::hardware_concurrency() / 2, 2u, 4u));
    Vec3 c = center;
    streamer_ = std::make_unique<LodStreamer>(cfg, [c](const TmxNode& n, size_t* bytes) { return prepareNode(n, c, bytes); });
    streamer_->onReady = [this] {
        if (!repaintQueued_.exchange(true)) QMetaObject::invokeMethod(this, [this] { repaintQueued_ = false; update(); }, Qt::QueuedConnection);
    };
    streamer_->setRoots(roots);
    lastFrame_ = FrameInfo(); lastFrame_.idle = false;
    hasScene_ = true;
    hasLine_ = false;
    fitAll();
}

void PlanView::setCamera(double cx, double cy, double mpp) {
    target_ = QVector3D(float(cx - center_.x), float(cy - center_.y), target_.z());
    mpp_ = std::clamp(mpp, 1e-5, 1e5);
    updateMatrices();
    update();
}

void PlanView::setScene(std::vector<std::shared_ptr<DisplayMesh>> meshes, const Vec3& center, const Box3& bounds, const SrsInfo& srs) {
    streamer_.reset();
    makeCurrent(); freeGpu(); doneCurrent();
    meshes_ = std::move(meshes);
    hasScene_ = !meshes_.empty();
    center_ = center; bounds_ = bounds; srs_ = srs;
    hidx_.build(meshes_, std::max(0.05, std::max(bounds.mx.x - bounds.mn.x, bounds.mx.y - bounds.mn.y) / 512.0));
    needUpload_ = true;
    hasLine_ = false;
    fitAll();
}

void PlanView::initializeGL() {
    initializeOpenGLFunctions();
    if (const GLubyte* r = glGetString(GL_RENDERER)) glRenderer_ = QByteArray(reinterpret_cast<const char*>(r));
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

void PlanView::drawGpu(const Gpu& g) {
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

void PlanView::paintStreaming(const QMatrix4x4& mvp) {
    // 화면 판정(장면 중심 기준 상자 8꼭짓점을 클립 공간으로: 한 평면 밖에 모두 있으면 안 보임)
    LodStreamer::View v;
    const Vec3 c = center_;
    v.visible = [&mvp, c](const Box3& b) {
        int out[6] = {0, 0, 0, 0, 0, 0};
        for (int i = 0; i < 8; ++i) {
            QVector4D p = mvp * QVector4D(float((i & 1 ? b.mx.x : b.mn.x) - c.x), float((i & 2 ? b.mx.y : b.mn.y) - c.y), float((i & 4 ? b.mx.z : b.mn.z) - c.z), 1.f);
            float w = p.w();
            out[0] += p.x() < -w; out[1] += p.x() > w; out[2] += p.y() < -w; out[3] += p.y() > w; out[4] += p.z() < -w; out[5] += p.z() > w;
        }
        for (int k = 0; k < 6; ++k) if (out[k] == 8) return false;
        return true;
    };
    const double ppm = lodBias_ / std::max(1e-9, mpp_ / std::max(1.0, devicePixelRatioF()));  // 정사: 화면 지름 = 대각선 / (m/물리px)
    v.screenDiameter = [ppm](const Box3& b) { return b.diag() * ppm; };
    QElapsedTimer t; t.start();
    auto f = streamer_->update(v);
    FrameInfo fi;
    for (auto k : f.evict) {
        auto it = gpuNodes_.find(k);
        if (it != gpuNodes_.end()) { freeGpuNode(it->second); gpuNodes_.erase(it); }
        fi.evicted++;
    }
    // 올리기: 프레임당 약 8 ms(최소 1노드) — 큰 모델에서도 화면이 멈추지 않음
    QElapsedTimer ut; ut.start();
    for (auto k : f.upload) {
        if (fi.uploaded > 0 && ut.elapsed() >= 8) break;
        auto p = std::static_pointer_cast<NodePrep>(streamer_->take(k));
        if (!p) continue;
        std::vector<Gpu> parts;
        for (auto& P : p->parts) {
            Gpu g;
            uploadPart(this, P.inter.data(), P.inter.size(), P.idx.data(), P.idx.size(), P.tex, g.vbo, g.ibo, g.tex);
            g.count = GLsizei(P.idx.size());
            g.hasTex = !P.tex.isNull();
            g.bytes = P.inter.size() * 4 + P.idx.size() * 4 + (g.hasTex ? size_t(P.tex.width()) * P.tex.height() * 16 / 3 : 0);
            gpuNodeBytes_ += g.bytes;
            parts.push_back(g);
        }
        gpuNodes_[k] = std::move(parts);
        fi.uploaded++;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for (auto k : f.draw) {
        auto it = gpuNodes_.find(k);
        if (it == gpuNodes_.end()) continue;
        for (auto& g : it->second) drawGpu(g);
    }
    fi.draw = f.draw.size(); fi.wanted = f.wanted; fi.queued = f.queued; fi.loading = f.loading;
    fi.residentBytes = f.residentBytes; fi.gpuBytes = gpuNodeBytes_; fi.maxDepth = f.maxDepthDrawn;
    fi.idle = f.idle() && fi.uploaded == size_t(f.upload.size());
    fi.ms = t.nsecsElapsed() / 1e6;
    lastFrame_ = fi;
}

void PlanView::paintGL() {
    QElapsedTimer ft; ft.start();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (hasScene_) {
        if (needUpload_) upload();
        updateMatrices();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        prog_->bind();
        prog_->setUniformValue("uMvp", proj_ * view_);
        prog_->setUniformValue("uTex", 0);
        glActiveTexture(GL_TEXTURE0);
        if (streamer_) paintStreaming(proj_ * view_);
        else for (auto& g : gpu_) drawGpu(g);
        glDisableVertexAttribArray(0); glDisableVertexAttribArray(1); glDisableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        prog_->release();
        glDisable(GL_DEPTH_TEST);
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    if (!hasScene_) paintEmpty(p);
    else paintOverlay(p);
    if (streamer_ && !lastFrame_.idle) {  // 스트리밍 진행 표시(오른쪽 아래)
        QFont f(theme::fontFamily()); f.setPointSizeF(8); p.setFont(f);
        QString s = QStringLiteral("세부 불러오는 중 · %1").arg(lastFrame_.wanted + lastFrame_.queued);
        QRectF r(width() - 190, height() - 26, 180, 18);
        p.setPen(Qt::NoPen); p.setBrush(QColor(255, 255, 255, 210)); p.drawRoundedRect(r, 6, 6);
        p.setPen(theme::InkSub); p.drawText(r, Qt::AlignCenter, s);
    }
    p.end();
    if (!streamer_) { lastFrame_ = FrameInfo(); lastFrame_.draw = gpu_.size(); lastFrame_.idle = !needUpload_; }
    lastFrame_.ms = ft.nsecsElapsed() / 1e6;
    if (needUpload_ || (streamer_ && !lastFrame_.idle)) QTimer::singleShot(streamer_ ? 15 : 0, this, [this] { update(); });  // 남은 것 다음 프레임에
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

bool PlanView::screenRayLocal(const QPointF& sp, Vec3& o, Vec3& d) const {
    bool inv = false;
    QMatrix4x4 m = (proj_ * view_).inverted(&inv);
    if (!inv || width() <= 0 || height() <= 0) return false;
    float nx = float(sp.x() / width() * 2 - 1), ny = float(1 - sp.y() / height() * 2);
    QVector3D A = (m * QVector4D(nx, ny, -1, 1)).toVector3DAffine(), B = (m * QVector4D(nx, ny, 1, 1)).toVector3DAffine();
    // 화면 행렬은 장면 중심 기준 float → 로컬은 double 로 중심을 더함
    o = Vec3(double(A.x()) + center_.x, double(A.y()) + center_.y, double(A.z()) + center_.z);
    d = Vec3(double(B.x()) - A.x(), double(B.y()) - A.y(), double(B.z()) - A.z());
    return d.x * d.x + d.y * d.y + d.z * d.z > 0;
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
    if (!hasScene_) { if (e->button() == Qt::LeftButton && onOpenRequest) onOpenRequest(); return; }
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
    bool ok = hasScene_ && screenToLocalXY(e->position(), w);
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
    if (hasLine_ && hasScene_) {
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
    if (!hasScene_) return;
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
    if (!hasScene_) return false;
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
