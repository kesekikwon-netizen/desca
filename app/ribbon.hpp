// 리본(디자인 v5 §3 · Strata KaBeginnerRibbon 과 같은 규칙): 탭 없음, 묶음 이름이 묶음 왼쪽 위, 타일 칩(아이콘 위 · 글자 아래),
// 창 폭에 들어가는 가장 큰 크기 — 좁아지면 글자를 먼저 숨기고 타일을 줄인다. 묶음은 접지 않는다.
// Q_OBJECT 없음(moc 없이 빌드): 신호 연결은 람다, 칩 목록은 멤버로 든다.
#pragma once
#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>
#include "icons.hpp"

class QAction;
class QEvent;
class QFrame;
class QHBoxLayout;
class QLabel;
class QResizeEvent;
class QShowEvent;
class QToolButton;

/// 리본 크기 한 단계. chipWidth 0 = max(타일 + 8, 글자 폭 + 8)
struct RibbonLook {
    int tile;       // 타일 한 변(px)
    int glyph;      // 타일 안 아이콘(px) — round(tile × 18 / 32)
    int chipWidth;  // 글자 숨김일 때 칩 고정 폭, 0 이면 글자를 따름
    bool labels;    // 글자를 타일 아래 보이는가
};

class Ribbon : public QWidget {
public:
    explicit Ribbon(const kerf::ChipColors& colors, QWidget* parent = nullptr);

    /// 묶음을 오른쪽에 더한다(id 가 이미 있으면 그 묶음)
    QFrame* addGroup(const QString& id, const QString& caption);
    /// 동작을 칩으로. label = 타일 아래 짧은 글자(2–4자), 툴팁 = 동작의 툴팁(긴 이름 · 키). kind 가 켜짐 색을 정한다
    QToolButton* addAction(const QString& groupId, QAction* a, kerf::ChipKind kind, const QString& iconName, const QString& label);
    /// 칩이 아닌 위젯(뒤 깊이 칩 · 칸)을 묶음에. 크기는 위젯의 sizeHint
    void addWidget(const QString& groupId, QWidget* w);
    /// 오른쪽 끝 묶음(Strata 「지역」 + 찾기 자리)
    void setCorner(QWidget* w);
    /// 홈 탭처럼 쓸 수 없는 묶음: 이름을 흐리게
    void setGroupDim(const QString& id, bool dim);
    QFrame* group(const QString& id) const;
    QList<QToolButton*> chips() const;

    RibbonLook look() const;
    int lookIndex() const { return lookIndex_; }
    /// 큰 것부터: 타일 50 · 48 … 34(글자) → 32(글자) → 32 · 24 · 20(글자 없음, 칩 40 · 30 · 26)
    static QList<RibbonLook> looks();
    /// widths[i] = looks()[i] 에서 필요한 폭. 들어가는 첫 크기, 없으면 마지막
    static int chooseLook(const QList<int>& widths, int available);
    QList<int> lookWidths() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* e) override;

private:
    struct Chip { QToolButton* button = nullptr; kerf::ChipKind kind = kerf::ChipKind::Normal; QString icon; };
    struct Group { QFrame* frame = nullptr; QLabel* caption = nullptr; QHBoxLayout* row = nullptr; };

    int chipWidth(const RibbonLook& L, const QToolButton* b) const;
    int chipHeight(const RibbonLook& L) const;
    int lineHeight() const;
    void drawChip(const Chip& c, const RibbonLook& L);
    void updateLook();
    void applyLook(int index, bool force = false);
    QIcon chipIconCached(const Chip& c, const RibbonLook& L);

    kerf::ChipColors colors_;
    QHBoxLayout* row_ = nullptr;
    QWidget* corner_ = nullptr;
    QHash<QString, Group> groups_;
    QStringList order_;
    QList<Chip> chips_;
    QHash<QString, QIcon> iconCache_;   // "이름|타일|종류"
    int lookIndex_ = 0;
    int appliedLine_ = 0;
    bool updating_ = false;
};
