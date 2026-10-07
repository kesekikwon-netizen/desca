#pragma once
#include <algorithm>

namespace asec {

/// 피치 90° = 바로 아래, 0° = 수평, 음수 = 위를 봄.
/// dy = 현재 y − 이전 y (위젯 y는 아래로 증가). 포인터를 위로 보내면 dy < 0.
/// liftUp: 가운데 버튼. 오른쪽 버튼은 8°~90°, 가운데는 −89°~90°.
inline double orbitPitch(double pitch, double dy, bool liftUp) {
    const double k = 0.3;
    const double next = liftUp ? pitch + k * dy : pitch - k * dy;
    const double lo = liftUp ? -89.0 : 8.0;
    return std::clamp(next, lo, 90.0);
}

}  // namespace asec
