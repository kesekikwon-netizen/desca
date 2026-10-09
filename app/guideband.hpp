// 떠 있는 안내(디자인 v5 §5 · Strata 지도 왼쪽 위 안내 띠): 도구 아이콘 + 「도구 › 단계」 + 선 + 문장 + 키 캡.
// 화면(평면 · 단면 · 책상) 위 왼쪽 위 (12, 12)에 떠 있고, 호스트 크기가 바뀌면 제자리를 지킨다. 도구를 든 동안만 보인다.
// FloatButtons = 같은 모양의 떠 있는 단추 묶음(되돌리기 · 다시, 종이 아래 도구 줄). Q_OBJECT 없음.
#pragma once
#include <QFrame>
#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>

class QAction;
class QBoxLayout;
class QEvent;
class QHBoxLayout;
class QLabel;
class QToolButton;

class GuideBand : public QFrame {
public:
    /// host = 화면 캔버스를 품은 위젯(이 위젯의 자식으로 떠 있는다)
    explicit GuideBand(QWidget* host);
    void setTool(const QString& iconName, const QString& title);   // 「단면선 긋기 › A 찾는 중」
    void setHint(const QString& text);                              // 「평면에서 시작점 A를 클릭하세요」
    void setKeys(const QStringList& keys);                          // {"Shift", "Esc"} → 키 캡
    void place();                                                   // (12, 12), 폭은 호스트 안(오른쪽 되돌리기 자리 reserveRight 만큼 비움)
    void setReserveRight(int px) { reserveRight_ = px; place(); }
    QString title() const;
    QString hint() const;
    bool titleShown() const;   // 감사용: 도구 이름(단계)이 보이는가
    QString debugWidths() const;   // 감사용: 문장 라벨 폭/권장 · 칩 폭/권장

protected:
    bool eventFilter(QObject* watched, QEvent* e) override;

private:
    QWidget* host_ = nullptr;
    QLabel* icon_ = nullptr;
    QLabel* title_ = nullptr;
    QLabel* hint_ = nullptr;
    QFrame* rule_ = nullptr;   // 도구 이름 뒤 1×16 선(이름을 숨기면 같이 숨김)
    QWidget* keysBox_ = nullptr;
    QHBoxLayout* keysLay_ = nullptr;
    QList<QLabel*> keyLabels_;
    QString fullHint_;
    QString fullTitle_;      // 「단면선 긋기 › A′ 찾는 중」 — 좁으면 「A′ 찾는 중」, 더 좁으면 숨김
    QString iconName_;       // 지금 아이콘(같으면 SVG 를 다시 그리지 않음)
    QStringList keys_;       // 지금 키 캡(같으면 라벨을 다시 만들지 않음 — 검토 I3)
    int reserveRight_ = 85;   // 되돌리기 · 다시(73) + 사이 12
};

class FloatButtons : public QFrame {
public:
    /// where: Qt::AlignLeft|AlignTop(왼쪽 위) · AlignRight|AlignTop(오른쪽 위) · AlignHCenter|AlignBottom(아래 가운데)
    FloatButtons(QWidget* host, Qt::Orientation o, Qt::Alignment where, int margin = 12);
    /// 아이콘 + (글자) 단추. a 가 있으면 눌림 · 켜짐 · 꺼짐을 동작과 맞춘다
    QToolButton* add(const QString& iconName, const QString& tip, QAction* a = nullptr, const QString& label = {});
    /// 다른 떠 있는 위젯의 오른쪽에 붙어 다닌다(되돌리기 · 다시 = 안내 칩 오른쪽 12 px)
    void followRightOf(QWidget* w, int gap = 12);
    void place();

protected:
    bool eventFilter(QObject* watched, QEvent* e) override;

private:
    QWidget* host_ = nullptr;
    QPointer<QWidget> follow_;   // 따라가는 위젯이 먼저 지워지면 null(검토)
    int gap_ = 12, margin_ = 12;
    Qt::Alignment where_;
    Qt::Orientation orient_;
    QBoxLayout* lay_ = nullptr;
    QList<QToolButton*> buttons_;
};
