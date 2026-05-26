#pragma once
#include "DxLib.h"
#include "LaserTarget.h" // LaserColor の定義用

class Filter {
private:
    VECTOR m_start;
    VECTOR m_end;
    LaserColor m_color; // このフィルターが何色に変えるか

public:
    Filter(VECTOR start, VECTOR end, LaserColor color)
        : m_start(start), m_end(end), m_color(color) {
    }

    void Draw() const {
        // 透過するセロハンのようなイメージで、少し太めの点線や半透明の線で描画
        unsigned int drawColor = 
            (m_color == LaserColor::Red) ? 
            GetColor(255, 50, 50) : 
            (
                (m_color == LaserColor::Green) ? 
                GetColor(50, 255, 50) : 
                GetColor(50, 50, 255)
                );

        // フィルターっぽく見せるために、ちょっと太め（太さ3）で描画
        DrawLine((int)m_start.x, (int)m_start.y, (int)m_end.x, (int)m_end.y, drawColor, 3);
        // 四角い枠などを端っこに描くとよりフィルターらしくなります
        DrawCircle((int)m_start.x, (int)m_start.y, 4, drawColor, TRUE);
        DrawCircle((int)m_end.x, (int)m_end.y, 4, drawColor, TRUE);
    }

    VECTOR GetStart() const { return m_start; }
    VECTOR GetEnd()   const { return m_end; }
    LaserColor GetLaserColor() const { return m_color; }
};