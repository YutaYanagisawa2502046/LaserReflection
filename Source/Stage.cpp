#include "Stage.h"
#include "Utility.h"
#include <fstream>  // 💡 追加：ファイル入力用
#include <sstream>  // 💡 追加：文字列解析用

float GetDistanceLineToPoint(VECTOR lineStart, VECTOR lineEnd, VECTOR point) {
    float A = point.x - lineStart.x;
    float B = point.y - lineStart.y;
    float C = lineEnd.x - lineStart.x;
    float D = lineEnd.y - lineStart.y;
    float dot = A * C + B * D;
    float lenSq = C * C + D * D;
    float param = (lenSq != 0) ? (dot / lenSq) : -1;
    float nearestX, nearestY;
    if (param < 0) {
        nearestX = lineStart.x;
        nearestY = lineStart.y;
    }
    else if (param > 1) {
        nearestX = lineEnd.x;
        nearestY = lineEnd.y;
    }
    else {
        nearestX = lineStart.x + param * C;
        nearestY = lineStart.y + param * D;
    }
    return sqrtf((point.x - nearestX) * (point.x - nearestX) + (point.y - nearestY) * (point.y - nearestY));
}

Stage::Stage(const StageData& data)
    : m_laser(nullptr)
    , m_target(nullptr)
    , m_maxMirrors(data.maxMirrors)
    , m_clearTimer(0)
{
    m_laser = new Laser(data.laserPos, data.laserDir, data.initialColor);
    m_target = new LaserTarget(data.targetPos, data.targetRadius, data.requiredColor);
    m_obstacles = data.obstacles;
    m_filters = data.filters;
}

Stage::~Stage() {
    if (m_laser != nullptr)  delete m_laser;
    if (m_target != nullptr) delete m_target;
}

// 💡 的がヒットしているかを GameScene に伝えるためのゲッター
bool Stage::IsTargetHit() const {
    if (m_target == nullptr) return false;
    return m_target->IsHit();
}

// 💡 外部（GameScene）から新しい鏡を配置するための関数
void Stage::AddMirror(const VECTOR& start, const VECTOR& end) {
    if (VSquareSize(VSub(end, start)) <= 20.f *20.f)
        return;
    
    if ((int)m_mirrors.size() < m_maxMirrors) {
        m_mirrors.push_back(Mirror(start, end));

        // 鏡が増えたのでレーザーを一度リセットする
        if (m_laser != nullptr) {
            m_laser->Reset();
        }
    }
}

bool Stage::Update(const VECTOR& mousePos, int mouseInput, int prevMouseInput) {
    int targetIndex = -1;
    float minDistance = 999999.0f;

    // 全ての鏡の選択状態を一旦クリアしておく
    for (auto& mirror : m_mirrors) {
        mirror.SetSelect(false);
        mirror.Update();
    }

    m_laser->Update();

    // 1. マウスに一番近い鏡を探索
    for (int i = 0; i < (int)m_mirrors.size(); ++i) {
        float dist = GetDistanceLineToPoint(m_mirrors[i].GetStart(), m_mirrors[i].GetEnd(), mousePos);
        if (dist < 12.0f && dist < minDistance) {
            minDistance = dist;
            targetIndex = i;
        }
    }

    bool isLaserResetRequired = false;

    // 2. マウスの近くに鏡が見つかった場合の処理
    if (targetIndex != -1) {
        m_mirrors[targetIndex].SetSelect(true);

        // [A] 右クリックによる鏡の削除
        if (mouseInput & MOUSE_INPUT_RIGHT && !(prevMouseInput & MOUSE_INPUT_RIGHT)) {
            m_mirrors.erase(m_mirrors.begin() + targetIndex);
            isLaserResetRequired = true;
        }
        // [B] ホイール回転による鏡の角度更新
        else {
            // 先ほどの順序バグ対策：Updateする前に前フレームのChangeフラグを回収
            if (m_mirrors[targetIndex].GetIsChange()) {
                isLaserResetRequired = true;
            }
            m_mirrors[targetIndex].Update();
        }
    }

    // [C] 鏡の回転、配置、削除によるレーザーリセット
    if (m_laser != nullptr && isLaserResetRequired) {
        m_laser->Reset();
        m_clearTimer = 0; // 回転中などはタイマーリセット
    }

    // 的がDraw()内でHitを検知した結果を反映するため、クリア判定はDrawの直後（GameScene側）で行います
    return false;
}

void Stage::Draw(bool isDragging, const VECTOR& dragStartPos, const VECTOR& dragCurrentPos) {
    // 1. オブジェクトの描画
    for (auto& mirror : m_mirrors)   mirror.Draw();
    for (auto& obstacle : m_obstacles) obstacle.Draw();
    for (auto& filter : m_filters)     filter.Draw();

    // ドラッグ中のプレビュー線
    if (isDragging) {
        DrawLine((int)dragStartPos.x, (int)dragStartPos.y, (int)dragCurrentPos.x, (int)dragCurrentPos.y, GetColor(255, 255, 0), 2);
    }

    // 2. レーザーと的の計算・描画直前にリセット（描画とリセットのねじれ解消）
    if (m_target != nullptr) {
        m_target->ResetHitState();
    }

    if (m_laser != nullptr && m_target != nullptr) {
        m_laser->Draw(m_mirrors, m_obstacles, m_filters, *m_target);
    }
    if (m_target != nullptr) {
        m_target->Draw();
    }
}