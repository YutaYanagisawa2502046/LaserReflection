#include "LaserTarget.h"

LaserTarget::LaserTarget(VECTOR position, float radius)
    : m_position(position), m_radius(radius), m_isHit(false) {
}

void LaserTarget::ResetHitState() {
    m_isHit = false; // 判定をクリア（レーザー側で毎回チェックするため）
}

void LaserTarget::SetHit() {
    m_isHit = true;  // レーザーが接触している
}

void LaserTarget::Draw() {
    int x = (int)m_position.x;
    int y = (int)m_position.y;
    int r = (int)m_radius;

    if (m_isHit) {
        // 【レーザーが当たって光っている状態】
        // 鮮やかな黄色で内側を塗りつぶし、さらに外側に淡い光（オーラ）を描画
        DrawCircle(x, y, r + 5, GetColor(255, 200, 0), FALSE); // 外枠の光
        DrawCircle(x, y, r, GetColor(255, 255, 0), TRUE);      // 輝く黄色
        DrawCircle(x, y, r - 5, GetColor(255, 255, 200), TRUE); // 中心をさらに白っぽく
        DrawString(x - 20, y - 6, "HIT!!", GetColor(0, 0, 0));
    }
    else {
        // 【通常状態】
        // 落ち着いた赤か暗めのターゲットマーク
        DrawCircle(x, y, r, GetColor(150, 0, 0), TRUE);        // 暗い赤の土台
        DrawCircle(x, y, r, GetColor(255, 255, 255), FALSE);   // 白い外枠
        DrawCircle(x, y, r - 8, GetColor(255, 255, 255), FALSE); // 内側の標準線
        DrawCircle(x, y, 4, GetColor(255, 255, 255), TRUE);    // 中心点
    }
}