#include "Mirror.h"
#include <cmath>

Mirror::Mirror(VECTOR start, VECTOR end) : m_start(start), m_end(end) {
    CalculateNormal();
}

// 反射板の向き（法線）を計算する
void Mirror::CalculateNormal() {
    // 1. 始点から終点へのベクトルを計算
    float dx = m_end.x - m_start.x;
    float dy = m_end.y - m_start.y;

    // 2. ベクトルを90度回転させて垂直なベクトル（法線）を作る (-dy, dx)
    // ※今回は「線の右側」を反射面として扱います
    float nx = -dy;
    float ny = dx;

    // 3. ベクトルの長さを1にする（正規化）
    float length = std::sqrt(nx * nx + ny * ny);
    if (length > 0.0f) {
        float tmp = 1.f / length;
        m_normal.x = nx * tmp;
        m_normal.y = ny * tmp;
        m_normal.z = 0.0f;
    }
}

void Mirror::Draw() {
    // 反射板を緑色の太線で描画
    DrawLine((int)m_start.x, (int)m_start.y, (int)m_end.x, (int)m_end.y, GetColor(0, 255, 0), 5);

    // 【デバッグ用】法線（向き）がどちらを向いているか、中央から短い線で視覚化
    int midX = (int)((m_start.x + m_end.x) / 2);
    int midY = (int)((m_start.y + m_end.y) / 2);
    DrawLine(midX, midY, midX + (int)(m_normal.x * 15), midY + (int)(m_normal.y * 15), GetColor(255, 255, 0), 1);
}