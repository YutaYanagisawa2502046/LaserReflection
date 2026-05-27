#include "Laser.h"
#include "Mirror.h"
#include <cmath>
#include "LaserTarget.h"
#include "Obstacle.h"
#include "Filter.h"

// 💡 1. まず一番上に GetDistanceSq を定義します（これで下の関数から見つかるようになります）
float GetDistanceSq(VECTOR p1, VECTOR p2) {
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

Laser::Laser(VECTOR position, VECTOR direction, LaserColor initalColor, int maxReflections, float growSpeed)
	: m_position(position), m_direction(direction), m_maxReflections(maxReflections)
	, m_growSpeed(growSpeed), m_currentLength(0.0f), m_isLooping(false), m_initalColor(initalColor){

	float len = std::sqrtf(m_direction.x * m_direction.x + m_direction.y * m_direction.y);
	if (len > 0.0f) {
		float tmp = 1.f / len;
		m_direction.x *= tmp;
		m_direction.y *= tmp;
	}
}

void Laser::Reset() {
	m_currentLength = 0.0f;
	m_isLooping = false;     // ループフラグもリセット
	m_history.clear();
}

void Laser::Update() {
	// 毎フレーム、growSpeed 分だけレーザーの限界可視長さを伸ばしていく
	const float MAX_REACH = 3000.0f;
	if (m_currentLength < MAX_REACH) {
		m_currentLength += m_growSpeed;
		if (m_currentLength > MAX_REACH) {
			m_currentLength = MAX_REACH;
		}
	}
}

// 同じ軌道があるか過去の履歴をループで探す関数
bool Laser::IsDuplicateOrbit(VECTOR pos, VECTOR dir) {
	// 誤差の許容範囲（1ピクセル未満のズレや、わずかな角度のズレを許容する）
	const float EPSILON = 0.01f;

	for (const auto& record : m_history) {
		// 座標の差を計算
		float distDiff = std::sqrtf(std::powf(record.position.x - pos.x, 2) + std::powf(record.position.y - pos.y, 2));
		// 方向の差を計算
		float dirDiff = std::sqrtf(std::powf(record.direction.x - dir.x, 2) + std::powf(record.direction.y - dir.y, 2));

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
	if (std::fabsf(d) < 0.0001f) return false; // 平行な場合は交差しない

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

	if (lensq == 0.0f) return std::sqrtf(std::powf(pt.x - p1.x, 2) + std::powf(pt.y - p1.y, 2));

	// 線分上のどこの位置が点に一番近いか（比率 t）を計算
	float t = ((pt.x - p1.x) * dx + (pt.y - p1.y) * dy) / lensq;

	// 0～1の範囲にクランプ（線分の外側に行かないようにする）
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	// 最短距離にある線分上の座標
	float closestX = p1.x + t * dx;
	float closestY = p1.y + t * dy;

	// その点とターゲット(pt)との距離を返す
	return std::sqrtf(std::powf(pt.x - closestX, 2) + std::powf(pt.y - closestY, 2));
}

void Laser::Draw(const std::vector<Mirror>& mirrors,
	const std::vector<Obstacle>& obstacles,
	const std::vector<Filter>& filters,
	LaserTarget& target)
{
	VECTOR currentStart = m_position;
	VECTOR currentDir = m_direction;
	float remainingLength = m_currentLength;

	LaserColor currentLaserColor = m_initalColor;
	bool hasFilter = !filters.empty();

	RayHistory History = RayHistory();
	for (int i = 0; i < m_maxReflections; ++i) {
		if (remainingLength <= 0.0f) break;

		VECTOR currentEnd;
		currentEnd.x = currentStart.x + currentDir.x * remainingLength;
		currentEnd.y = currentStart.y + currentDir.y * remainingLength;
		currentEnd.z = 0.0f;

		const Mirror* closestMirror = nullptr;
		const Obstacle* closestObstacle = nullptr;
		const Filter* closestFilter = nullptr;

		VECTOR closestPoint = currentEnd;

		// 💡 2乗ではなく、通常の「距離」で比較するように変更します
		float minDistance = remainingLength;

		// ----------------------------------------------------
		// 1. 鏡との衝突判定
		// ----------------------------------------------------
		for (const auto& mirror : mirrors) {
			VECTOR intersect;
			if (CheckLineIntersection(currentStart, currentEnd, mirror.GetStart(), mirror.GetEnd(), &intersect)) {
				// 💡 sqrt をここでかけて、正しい距離（ピクセル単位）を出します
				float dist = std::sqrtf(GetDistanceSq(currentStart, intersect));
				if (dist > 1.0f && dist < minDistance) {
					minDistance = dist;
					closestPoint = intersect;
					closestMirror = &mirror;
					closestObstacle = nullptr;
					closestFilter = nullptr;
				}
			}
		}

		// ----------------------------------------------------
		// 2. 壁（障害物）との衝突判定
		// ----------------------------------------------------
		for (const auto& obstacle : obstacles) {
			VECTOR intersect;
			if (CheckLineIntersection(currentStart, currentEnd, obstacle.GetStart(), obstacle.GetEnd(), &intersect)) {
				float dist = std::sqrtf(GetDistanceSq(currentStart, intersect));
				if (dist > 1.0f && dist < minDistance) {
					minDistance = dist;
					closestPoint = intersect;
					closestMirror = nullptr;
					closestObstacle = &obstacle;
					closestFilter = nullptr;
				}
			}
		}

		// ----------------------------------------------------
		// 3. カラーフィルターとの衝突判定
		// ----------------------------------------------------
		for (const auto& filter : filters) {
			VECTOR intersect;
			if (CheckLineIntersection(currentStart, currentEnd, filter.GetStart(), filter.GetEnd(), &intersect)) {
				float dist = std::sqrtf(GetDistanceSq(currentStart, intersect));
				if (dist > 1.0f && dist < minDistance) {
					minDistance = dist;
					closestPoint = intersect;
					closestMirror = nullptr;
					closestObstacle = nullptr;
					closestFilter = &filter;
				}
			}
		}

		// ----------------------------------------------------
		// 4. 的（LaserTarget）への当たり判定
		// ----------------------------------------------------
		float distToTarget = GetDistanceLineToPoint(currentStart, closestPoint, target.GetPosition());
		if (distToTarget <= target.GetRadius()) {
			target.SetHit(currentLaserColor, hasFilter);
		}

		// ----------------------------------------------------
		// 5. レーザーの描画処理
		// ----------------------------------------------------
		unsigned int drawColor = GetColor(255, 50, 50);
		if (currentLaserColor == LaserColor::Green)
			drawColor = GetColor(50, 255, 50);
		else if (currentLaserColor == LaserColor::Blue)
			drawColor = GetColor(50, 50, 255);


		// 正しい交点（closestPoint）まで線を引く
		DrawLine((int)currentStart.x, (int)currentStart.y, (int)closestPoint.x, (int)closestPoint.y, drawColor, 2);

		// 💡 正しい距離（minDistance）をマイナスするので、残りの長さが正常に維持されます
		remainingLength -= minDistance;

		// ----------------------------------------------------
		// 6. 衝突後の挙動分岐
		// ----------------------------------------------------
		if (closestMirror != nullptr) {
			VECTOR N = closestMirror->GetNormal();
			float dotProduct = currentDir.x * N.x + currentDir.y * N.y;

			currentStart = closestPoint;
			currentDir.x = currentDir.x - 2.0f * dotProduct * N.x;
			currentDir.y = currentDir.y - 2.0f * dotProduct * N.y;

			float len = std::sqrtf(currentDir.x * currentDir.x + currentDir.y * currentDir.y);
			if (len > 0.0f) { float tmp = 1.f / len; currentDir.x *= tmp; currentDir.y *= tmp; }
			History.position = currentStart;
			History.direction = currentDir;
			History.Color = currentLaserColor;
			m_history.emplace_back(History);
		}
		else if (closestFilter != nullptr) {
			currentStart = closestPoint;
			currentLaserColor = closestFilter->GetLaserColor();
			History.Color = currentLaserColor;
			History.position = currentEnd;
			History.direction = currentDir;
			m_history.emplace_back(History);
		}
		else if (closestObstacle != nullptr) {
			History.position = closestPoint;
			History.direction = currentDir;
			History.Color = currentLaserColor;
			m_history.emplace_back(History);
			break;
		}
		else {
			break;
		}
	}
}
