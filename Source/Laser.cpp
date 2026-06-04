#include "Laser.h"
#include "Mirror.h"
#include <cmath>
#include "LaserTarget.h"
#include "Obstacle.h"
#include "Filter.h"
#include "Master.h"

// 💡 1. まず一番上に GetDistanceSq を定義します（これで下の関数から見つかるようになります）
float GetDistanceSq(VECTOR p1, VECTOR p2) {
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

Laser::Laser(VECTOR position, VECTOR direction, LaserColor initalColor, int maxReflections, float growSpeed)
	: m_position(position), m_direction(direction), m_maxReflections(maxReflections)
	, m_growSpeed(growSpeed), m_currentLength(0.0f), m_initalColor(initalColor)
	, m_isLaserStart(true) {

	// 2. 方向ベクトルを正規化して長さを1に揃える（これで後の計算が楽になります）
	float len = std::sqrtf(m_direction.x * m_direction.x + m_direction.y * m_direction.y);
	if (len > 0.0f) {
		// 逆数を掛けることで正規化（長さを1にする）
		float tmp = 1.f / len;
		m_direction.x *= tmp;
		m_direction.y *= tmp;
	}
}

void Laser::Reset() {
	m_currentLength = 0.0f;
	m_history.clear();
	m_isLaserStart = true;
}

void Laser::Update() {
	// 毎フレーム、growSpeed 分だけレーザーの限界可視長さを伸ばしていく
	const float MAX_REACH = 3000.0f;
	if (m_currentLength < MAX_REACH) {
		// 💡 growSpeed を加算していき、MAX_REACH を超えないようにクランプします
		m_currentLength += m_growSpeed;
		if (m_currentLength > MAX_REACH) {
			m_currentLength = MAX_REACH;
		}
		if (m_isLaserStart)
		{
			// レーザーが最初に発射されたときだけ、SEを再生します
			m_isLaserStart = false;
			Master::m_soundManager->PlaySE(SE::LASER);
		}
	}
}

// 交差判定
bool Laser::CheckLineIntersection(VECTOR p1, VECTOR p2, VECTOR p3, VECTOR p4, VECTOR* outIntersection) {
	float d = (p2.x - p1.x) * (p4.y - p3.y) - (p2.y - p1.y) * (p4.x - p3.x);
	if (std::fabsf(d) < 0.0001f) return false; // 平行な場合は交差しない

	// 交差点の位置を計算するためのパラメータ u と v を求める
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

// 線分と点の最短距離を求める
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

	// 1. レーザーの軌道を計算して描画する関数。鏡や壁、フィルターとの衝突判定もここで行います。
	m_isLooping = false;
	m_history.clear();

	// 💡 まずは、レーザーの現在の始点と進行方向、残りの長さを初期化します
	VECTOR currentStart = m_position;
	VECTOR currentDir = m_direction;
	float remainingLength = m_currentLength;

	// 💡 レーザーの色は、最初は m_initalColor からスタートします。途中でフィルターを通過したら、その色に変わります。
	LaserColor currentLaserColor = m_initalColor;
	bool hasFilter = !filters.empty();

	// 反射の履歴を記録するための構造体を用意します。これで、過去の衝突位置や方向を保存できます。
	RayHistory History = RayHistory();
	for (int i = 0; i < m_maxReflections; ++i) {
		if (remainingLength <= 0.0f) break;

		// 💡 現在のレーザーの終点を計算します（始点 + 方向 * 残りの長さ）。これが、今の段階でレーザーが届く最大の位置になります。
		VECTOR currentEnd;
		currentEnd.x = currentStart.x + currentDir.x * remainingLength;
		currentEnd.y = currentStart.y + currentDir.y * remainingLength;
		currentEnd.z = 0.0f;

		// 💡 今回は、鏡・壁・フィルターの中で「一番近い衝突点」を見つけるために、候補を比較する変数を用意します。
		const Mirror* closestMirror = nullptr;
		const Obstacle* closestObstacle = nullptr;
		const Filter* closestFilter = nullptr;

		// 💡 最初は、レーザーが届く最大の位置（currentEnd）が「最も近い衝突点」としてスタートします。これを基準に、鏡や壁、フィルターとの交点を比較していきます。
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
					closestMirror = &mirror;	// 鏡が一番近い衝突対象になったので、他の候補はリセットします
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
					closestObstacle = &obstacle;	// 壁が一番近い衝突対象になったので、他の候補はリセットします
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
					closestFilter = &filter;	// フィルターが一番近い衝突対象になったので、他の候補はリセットします
				}
			}
		}

		// ----------------------------------------------------
		// 4. 的（LaserTarget）への当たり判定
		// ----------------------------------------------------
		float distToTarget = GetDistanceLineToPoint(currentStart, closestPoint, target.GetPosition());
		if (distToTarget <= target.GetRadius()) {
			target.SetHit(currentLaserColor, hasFilter);	// 的に当たったことを通知します
		}

		// ----------------------------------------------------
		// 5. レーザーの描画処理
		// ----------------------------------------------------
		unsigned int drawColor = GetColor(255, 50, 50);
		if (currentLaserColor == LaserColor::Green)
			drawColor = GetColor(50, 255, 50);
		else if (currentLaserColor == LaserColor::Blue)
			drawColor = GetColor(50, 50, 255);

		// 💡 無限ループを検知している場合は、色を黄色っぽく変えてみます（これで視覚的にわかりやすくなります）
		if (m_isLooping)
			drawColor = GetColor(255, 255, 50);

		// 正しい交点（closestPoint）まで線を引く
		DrawLineAA(currentStart.x, currentStart.y, closestPoint.x, closestPoint.y, drawColor, 2);

		// 💡 正しい距離（minDistance）をマイナスするので、残りの長さが正常に維持されます
		remainingLength -= minDistance;

		// ----------------------------------------------------
		// 6. 衝突後の挙動分岐
		// ----------------------------------------------------
		if (closestMirror != nullptr) {
			// 衝突点を新しい始点にして、鏡の法線を使って反射方向を計算します
			VECTOR N = closestMirror->GetNormal();
			float dotProduct = currentDir.x * N.x + currentDir.y * N.y;

			currentStart = closestPoint;
			currentDir.x = currentDir.x - 2.0f * dotProduct * N.x;
			currentDir.y = currentDir.y - 2.0f * dotProduct * N.y;

			// 反射後の方向も、念のため長さを1に正規化して揃えておきます（これで次の衝突判定も安定します）
			float len = std::sqrtf(currentDir.x * currentDir.x + currentDir.y * currentDir.y);
			if (len > 0.0f) { float tmp = 1.f / len; currentDir.x *= tmp; currentDir.y *= tmp; }


			// 💡 【ここから追加！】無限ループチェック
			bool isLoopDetected = false;
			// 反射後の位置と方向を、過去の履歴と照合してみます。もし過去に同じような位置と方向があったら、ループと判断します。
			for (const auto& past : m_history) {
				// 過去の衝突位置と、今の位置がほぼ同じかチェック (距離の誤差2px以内)
				float distSq = VSquareSize(VSub(past.position, currentStart));

				// 過去の進行方向と、今の進行方向がほぼ同じかチェック (内積がほぼ1)
				float dirDot = VDot(past.direction, currentDir);

				if (distSq < 2.0f * 2.0f && dirDot > 0.99f) {
					m_isLooping = true; // 循環を検知！
					break;
				}
			}

			// 反射の履歴を保存します。これで、次の反射と比較できるようになります。
			History.position = currentStart;
			History.direction = currentDir;
			History.Color = currentLaserColor;
			m_history.emplace_back(History);
		}
		else if (closestFilter != nullptr) {
			// フィルターに当たったら、レーザーの色をそのフィルターの色に変えます
			currentStart = closestPoint;
			currentLaserColor = closestFilter->GetLaserColor();

			// フィルターを通過した位置と方向も履歴に保存しておきます（これでループ検知の精度が上がります）
			History.Color = currentLaserColor;
			History.position = currentEnd;
			History.direction = currentDir;
			m_history.emplace_back(History);
		}
		else if (closestObstacle != nullptr) {
			// 壁に当たったら、そこでレーザーは止まります。これ以上は進まないので、ループも発生しません。
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
