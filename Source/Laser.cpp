#include "Laser.h"
#include "Mirror.h"
#include <cmath>
#include "LaserTarget.h"
#include "Obstacle.h"

Laser::Laser(VECTOR position, VECTOR direction, int maxReflections, float growSpeed)
    : m_position(position), m_direction(direction), m_maxReflections(maxReflections)
    , m_growSpeed(growSpeed), m_currentLength(0.0f), m_isLooping(false) {

    float len = std::sqrt(m_direction.x * m_direction.x + m_direction.y * m_direction.y);
    if (len > 0.0f) {
        float tmp = 1.f / len;
        m_direction.x *= tmp;
        m_direction.y *= tmp;

    }
}

void Laser::Reset() {
    m_currentLength = 0.0f;
    m_isLooping = false;     // ループフラグもリセット
}

void Laser::Update() {
    // もし無限ループを検知していても、現在の長さの更新は
    // ループ軌道上でシュウウウッと最後まで伸びきるために続けさせます
    //if (m_currentLength < FLT_MAX) {
        m_currentLength += m_growSpeed;
    //}
}

// 同じ軌道があるか過去の履歴をループで探す関数
bool Laser::IsDuplicateOrbit(VECTOR pos, VECTOR dir) {
    // 誤差の許容範囲（1ピクセル未満のズレや、わずかな角度のズレを許容する）
    const float EPSILON = 0.01f;

    for (const auto& record : m_history) {
        // 座標の差を計算
        float distDiff = std::sqrt(std::pow(record.position.x - pos.x, 2) + std::pow(record.position.y - pos.y, 2));
        // 方向の差を計算
        float dirDiff = std::sqrt(std::pow(record.direction.x - dir.x, 2) + std::pow(record.direction.y - dir.y, 2));

        // 座標も方向もほぼ一緒なら「同じ軌道」とみなす
        if (distDiff < EPSILON && dirDiff < EPSILON) {
            return true;
        }
    }
    return false;
}

// 交差判定（変更なしのため省略）
bool Laser::CheckLineIntersection(VECTOR p1, VECTOR p2, VECTOR p3, VECTOR p4, VECTOR* outIntersection) {
    float d = (p2.x - p1.x) * (p4.y - p3.y) - (p2.y - p1.y) * (p4.x - p3.x);
    if (std::abs(d) < 0.0001f) return false; // 平行な場合は交差しない

    float u = ((p3.x - p1.x) * (p4.y - p3.y) - (p3.y - p1.y) * (p4.x - p3.x)) / d;
    float v = ((p3.x - p1.x) * (p2.y - p1.y) - (p3.y - p1.y) * (p2.x - p1.x)) / d;

    // u, v ともに 0～1 の間であれば線分同士が交差している
    if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
        if (outIntersection != nullptr) {
            outIntersection->x = p1.x + u * (p2.x - p1.x);
            outIntersection->y = p1.y + u * (p2.y - p1.y);
            outIntersection->z = 0.0f;
        }
        return true;
    }
    return false;
}

// 線分と点の最短距離を求める（数学の定石ロジック）
float Laser::GetDistanceLineToPoint(VECTOR p1, VECTOR p2, VECTOR pt) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float lensq = dx * dx + dy * dy;

    if (lensq == 0.0f) return std::sqrt(std::pow(pt.x - p1.x, 2) + std::pow(pt.y - p1.y, 2));

    // 線分上のどこの位置が点に一番近いか（比率 t）を計算
    float t = ((pt.x - p1.x) * dx + (pt.y - p1.y) * dy) / lensq;

    // 0～1の範囲にクランプ（線分の外側に行かないようにする）
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // 最短距離にある線分上の座標
    float closestX = p1.x + t * dx;
    float closestY = p1.y + t * dy;

    // その点とターゲット(pt)との距離を返す
    return std::sqrt(std::pow(pt.x - closestX, 2) + std::pow(pt.y - closestY, 2));
}

void Laser::Draw(const std::vector<Mirror>& mirrors, const std::vector<Obstacle>& obstacles, LaserTarget& target) {
    VECTOR currentStart = m_position;
    VECTOR currentDir = m_direction;
    float remainingLength = m_currentLength;

    for (int i = 0; i < 200; ++i) {
        if (remainingLength <= 0.0f) break;

        VECTOR currentEnd;
        currentEnd.x = currentStart.x + currentDir.x * 2000.0f;
        currentEnd.y = currentStart.y + currentDir.y * 2000.0f;
        currentEnd.z = 0.0f;

        // 一番近い衝突オブジェクトを探すための変数
        const Mirror* closestMirror = nullptr;
        const Obstacle* closestObstacle = nullptr; // ★追加
        VECTOR closestPoint = currentEnd;
        float minDistance = 999999.0f;

        // ① すべての「鏡」との衝突チェック（既存の処理）
        for (const auto& mirror : mirrors) {
            VECTOR intersect;
            if (CheckLineIntersection(currentStart, currentEnd, mirror.GetStart(), mirror.GetEnd(), &intersect)) {
                float dist = std::sqrt(std::pow(intersect.x - currentStart.x, 2) + std::pow(intersect.y - currentStart.y, 2));
                if (dist > 0.5f && dist < minDistance) {
                    minDistance = dist;
                    closestPoint = intersect;
                    closestMirror = &mirror;
                    closestObstacle = nullptr; // 鏡が暫定1位なので壁はリセット
                }
            }
        }

        // ② すべての「吸収壁」との衝突チェック（★新規追加）
        for (const auto& obstacle : obstacles) {
            VECTOR intersect;
            if (CheckLineIntersection(currentStart, currentEnd, obstacle.GetStart(), obstacle.GetEnd(), &intersect)) {
                float dist = std::sqrt(std::pow(intersect.x - currentStart.x, 2) + std::pow(intersect.y - currentStart.y, 2));
                if (dist > 0.5f && dist < minDistance) {
                    minDistance = dist;
                    closestPoint = intersect;
                    closestMirror = nullptr; // 壁が暫定1位になったので鏡をリセット
                    closestObstacle = &obstacle;
                }
            }
        }

        // 描画する線分の長さを確定
        float segmentLength = minDistance;
        bool stopGrowing = false;

        if (segmentLength > remainingLength) {
            segmentLength = remainingLength;
            closestPoint.x = currentStart.x + currentDir.x * segmentLength;
            closestPoint.y = currentStart.y + currentDir.y * segmentLength;
            closestMirror = nullptr;
            closestObstacle = nullptr; // どこにも届かずに途切れた
            stopGrowing = true;
        }

        // ③ 的との当たり判定（既存の処理）
        float distToTarget = GetDistanceLineToPoint(currentStart, closestPoint, target.GetPosition());
        if (distToTarget <= target.GetRadius()) {
            target.SetHit();
        }

        // レーザーの線を描画
        DrawLine((int)currentStart.x, (int)currentStart.y, (int)closestPoint.x, (int)closestPoint.y, GetColor(255, 0, 0), 2);

        remainingLength -= segmentLength;

        // ★【ここが肝】もし一番近いのが「吸収壁」だった、あるいは成長限界なら、反射せずにここで終了！
        if (closestObstacle != nullptr || closestMirror == nullptr || stopGrowing) {
            break;
        }

        // ④ 鏡だった場合の反射ベクトル計算（既存の処理）
        VECTOR N = closestMirror->GetNormal();
        float dotProduct = currentDir.x * N.x + currentDir.y * N.y;

        currentStart = closestPoint;
        currentDir.x = currentDir.x - 2.0f * dotProduct * N.x;
        currentDir.y = currentDir.y - 2.0f * dotProduct * N.y;
    }
}

//void Laser::Draw(const std::vector<Mirror>& mirrors) {
//    // 描画が始まる瞬間に、前フレームの履歴をクリアする
//    m_history.clear();
//
//    VECTOR currentStart = m_position;
//    VECTOR currentDir = m_direction;
//    float remainingLength = m_currentLength;
//
//    // 最初の発射点を履歴に登録
//    m_history.push_back({ currentStart, currentDir });
//
//    // 一時的にループフラグを偽にして、本当にループが繋がった時だけ真にする
//    m_isLooping = false;
//
//    for (int i = 0; i < m_maxReflections; ++i) {
//        if (remainingLength <= 0.0f) break;
//
//        VECTOR currentEnd;
//        currentEnd.x = currentStart.x + currentDir.x * 2000.0f;
//        currentEnd.y = currentStart.y + currentDir.y * 2000.0f;
//        currentEnd.z = 0.0f;
//
//        const Mirror* closestMirror = nullptr;
//        VECTOR closestPoint = currentEnd;
//        float minDistance = FLT_MAX;
//
//        for (const auto& mirror : mirrors) {
//            VECTOR intersect;
//            if (CheckLineIntersection(currentStart, currentEnd, mirror.GetStart(), mirror.GetEnd(), &intersect)) {
//                float dist = std::sqrt(std::pow(intersect.x - currentStart.x, 2) + std::pow(intersect.y - currentStart.y, 2));
//
//                // 【改良ポイント1】密着時の連続誤判定を防ぐため、1.0ピクセル以上進んだ場合のみ衝突とみなす
//                if (dist > 1.0f && dist < minDistance) {
//                    minDistance = dist;
//                    closestPoint = intersect;
//                    closestMirror = &mirror;
//                }
//            }
//        }
//
//        float segmentLength = minDistance;
//        bool stopGrowing = false;
//
//        if (segmentLength > remainingLength) {
//            segmentLength = remainingLength;
//            closestPoint.x = currentStart.x + currentDir.x * segmentLength;
//            closestPoint.y = currentStart.y + currentDir.y * segmentLength;
//            closestMirror = nullptr;
//            stopGrowing = true;
//        }
//
//        // レーザーを描画（通常は赤、無限ループ確定時は水色に）
//        int laserColor = m_isLooping ? GetColor(0, 255, 255) : GetColor(255, 0, 0);
//        DrawLine((int)currentStart.x, (int)currentStart.y, (int)closestPoint.x, (int)closestPoint.y, laserColor, 2);
//
//        remainingLength -= segmentLength;
//
//        if (closestMirror == nullptr || stopGrowing) {
//            break;
//        }
//
//        // 反射ベクトルの計算
//        VECTOR N = closestMirror->GetNormal();
//        float dotProduct = currentDir.x * N.x + currentDir.y * N.y;
//        VECTOR nextDir;
//        nextDir.x = currentDir.x - 2.0f * dotProduct * N.x;
//        nextDir.y = currentDir.y - 2.0f * dotProduct * N.y;
//        nextDir.z = 0.0f;
//
//        VECTOR nextStart = closestPoint;
//
//        // 【改良ポイント2】しっかりと距離（3ピクセル以上）を進んだ反射履歴だけを対象にループチェックを行う
//        // これにより、同じ鏡の同じ位置で連続して微小な反射が起きた際の誤検知を完全にシャットアウトします
//        if (minDistance > 3.0f && IsDuplicateOrbit(nextStart, nextDir)) {
//            m_isLooping = true;
//
//            // ループを閉じる最後の1本を、残りの長さの範囲で描画して終了
//            if (remainingLength > 0.0f) {
//                // ループが綺麗に閉じるように、交点までの線を引いてからブレイク
//                DrawLine((int)currentStart.x, (int)currentStart.y, (int)nextStart.x, (int)nextStart.y, GetColor(0, 255, 255), 2);
//            }
//            break;
//        }
//
//        // 正常に一定以上の距離を進んだ軌道のみ、履歴に登録する
//        if (minDistance > 3.0f) {
//            m_history.push_back({ nextStart, nextDir });
//        }
//
//        currentStart = nextStart;
//        currentDir = nextDir;
//    }
//}