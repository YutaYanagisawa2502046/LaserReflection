#include "Stage.h"
#include "Utility.h"
#include <fstream>
#include <sstream>

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

void Stage::SpawnEmitParticles(const VECTOR& emitPos, unsigned int color)
{
	// 毎フレーム3粒ずつ、壁との衝突点からシュワシュワ湧き出させる
	int spawnCount = 3;
	for (int i = 0; i < spawnCount; ++i) {
		DotParticle p;
		p.pos = emitPos;

		// 全方向（360度）にランダムな角度で飛び散る
		float angle = static_cast<float>(GetRand(360)) * DX_PI_F / 180.0f;
		// 1.0 〜 2.5 の間でランダムな初速
		float speed = 1.0f + static_cast<float>(GetRand(15)) / 10.0f;

		p.velocity = VGet(cosf(angle) * speed, sinf(angle) * speed, 0.0f);
		p.maxLife = static_cast<float>(30 + GetRand(50 - 30)) * 0.1f; // 寿命は15〜30フレーム（約0.3〜0.5秒）
		p.life = p.maxLife;
		p.color = color;

		m_particles.emplace_back(p);
	}
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

bool Stage::IsTargetHit() const {
	if (m_target == nullptr) return false;
	return m_target->IsHit();
}

void Stage::AddMirror(const VECTOR& start, const VECTOR& end) {
	if (VSquareSize(VSub(end, start)) <= 20.f * 20.f)
		return;

	if ((int)m_mirrors.size() < m_maxMirrors) {
		m_mirrors.push_back(Mirror(start, end));
		if (m_laser != nullptr) {
			m_laser->Reset();
		}
	}
}

bool Stage::Update(const VECTOR& mousePos, int mouseInput, int prevMouseInput) {
	int targetIndex = -1;
	float minDistance = 999999.0f;

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

	// 2. 鏡の操作
	if (targetIndex != -1) {
		m_mirrors[targetIndex].SetSelect(true);

		if (mouseInput & MOUSE_INPUT_RIGHT && !(prevMouseInput & MOUSE_INPUT_RIGHT)) {
			m_mirrors.erase(m_mirrors.begin() + targetIndex);
			isLaserResetRequired = true;
		}
		else {
			m_mirrors[targetIndex].Update();
			if (m_mirrors[targetIndex].GetIsChange()) {
				isLaserResetRequired = true;
			}
		}
	}

	if (m_laser != nullptr && isLaserResetRequired) {
		m_laser->Reset();
		m_clearTimer = 0;
		m_particles.clear();
	}

	// 💡 【超重要】クリア判定の前に、Updateの段階で衝突フラグを完全に確定させる！
	if (m_target != nullptr) {
		m_target->ResetHitState(); // まず綺麗にする
	}
	if (m_laser != nullptr && m_target != nullptr) {
		// レーザーに擬似計算を走らせて的へのヒット状況を「実更新」する
		m_laser->Draw(m_mirrors, m_obstacles, m_filters, *m_target);

		// 💡 【ここがポイント！】
		// レーザーの衝突シミュレーションが走った直後、もしレーザーが壁（Obstacle）に
		// 遮断されて止まっている場合、その終端座標（ぶつかっている場所）を取得します。
		// ※お使いの Laser クラスに終端座標をとる関数（例: GetEndPoint() や GetHitPosition()）
		// がある場合、以下のようにして毎フレーム自動的に火花を生成できます。

		// 【実装例】
		//VECTOR hitPoint = m_laser->GetEndPoint();
		//unsigned int currentLaserColor = m_laser->GetDxColor(); // 現在のレーザーの描画色
		//if (m_laser->IsHitObstacle()) { // 壁に当たっているフラグ等があれば
		//	SpawnEmitParticles(hitPoint, currentLaserColor);
		//}
	}

	// 💡 【追加】パーティクルの移動と寿命更新処理
	for (auto it = m_particles.begin(); it != m_particles.end(); ) {
		it->pos = VAdd(it->pos, it->velocity); // 移動
		it->velocity = VScale(it->velocity, 0.90f); // 💡 減速（空気抵抗で上品に）
		it->life -= 1.f / 60.f;

		if (it->life <= 0) {
			it = m_particles.erase(it); // 寿命で消滅
		}
		else {
			++it;
		}
	}

	return false;
}

void Stage::Draw(bool isDragging, const VECTOR& dragStartPos, const VECTOR& dragCurrentPos) {
	// 1. 静的オブジェクトの描画
	for (auto& obstacle : m_obstacles) obstacle.Draw();
	for (auto& filter : m_filters)     filter.Draw();
	for (auto& mirror : m_mirrors)     mirror.Draw();

	// ドラッグ中のプレビュー線
	if (isDragging) {
		DrawLine((int)dragStartPos.x, (int)dragStartPos.y, (int)dragCurrentPos.x, (int)dragCurrentPos.y, GetColor(255, 255, 0), 2);
	}

	// 2. 描画処理（ここではResetHitStateを絶対に呼ばない）
	if (m_laser != nullptr && m_target != nullptr) {
		m_laser->Draw(m_mirrors, m_obstacles, m_filters, *m_target);
	}
	if (m_target != nullptr) {
		m_target->Draw(!m_filters.empty());
	}
}