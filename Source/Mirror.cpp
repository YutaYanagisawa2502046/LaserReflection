#include "Mirror.h"
#include <cmath>

Mirror::Mirror(VECTOR start, VECTOR end) : m_start(start), m_end(end) {
    CalculateNormal();

    m_isChange = false;

    VECTOR LineDir = { 0,0,0 };
    float dx = m_end.x - m_start.x;
    float dy = m_end.y - m_start.y;
    float length = std::sqrt(dx * dx + dy * dy);
    if (length > 0.0f) {
        float tmp = 1.f / length;
        LineDir.x = dx * tmp;
        LineDir.y = dy * tmp;
        LineDir.z = 0.0f;
    }

    m_angle = atan2f(LineDir.y, LineDir.x);
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

void Mirror::Update() {
    
    float oldAngle = m_angle;

    if (m_isSelected) {
        float wheelRot = GetMouseWheelRotVolF();

        if (wheelRot != 0) {
            float angleStep = 5.0f;
            if (CheckHitKey(KEY_INPUT_LSHIFT) == 1 || CheckHitKey(KEY_INPUT_RSHIFT) == 1) {
                angleStep *= 0.1f;
            }

            float deltaAngle = angleStep * wheelRot;

            // 💡 【修正点】現在の start と end から、現在の正確な角度（度数法）を逆算する
            float dx = m_end.x - m_start.x;
            float dy = m_end.y - m_start.y;
            float currentAngle = std::atan2f(dy, dx) * (180.0f / DX_PI_F);

            // 💡 逆算した現在の角度に、ホイールの移動量を足す！
            m_angle = currentAngle + deltaAngle;

            // 360度のループ処理
            if (m_angle >= 360.f) m_angle -= 360.f;
            if (m_angle < 0.f)    m_angle += 360.f;

            // 💡 角度が変わったので再配置（changeのif文は外して、ホイールが動いたら確実に実行）
            VECTOR center = VGet((m_start.x + m_end.x) / 2.0f, (m_start.y + m_end.y) / 2.0f, 0.0f);
            float halfLength = std::sqrtf(dx * dx + dy * dy) * 0.5f;

            float rad = m_angle * (DX_PI_F / 180.0f);
            float cosA = std::cosf(rad);
            float sinA = std::sinf(rad);

            m_start.x = center.x - halfLength * cosA;
            m_start.y = center.y - halfLength * sinA;
            m_end.x = center.x + halfLength * cosA;
            m_end.y = center.y + halfLength * sinA;

            CalculateNormal();
        }
    }
    // 💡 oldAngle（前フレームの角度）と現在の角度を比較して変化フラグを確定
    m_isChange = (oldAngle != m_angle);
}