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
	}

	// 💡 【超重要】クリア判定の前に、Updateの段階で衝突フラグを完全に確定させる！
	if (m_target != nullptr) {
		m_target->ResetHitState(); // まず綺麗にする
	}
	if (m_laser != nullptr && m_target != nullptr) {
		// レーザーに擬似計算を走らせて的へのヒット状況を「実更新」する
		m_laser->Draw(m_mirrors, m_obstacles, m_filters, *m_target);
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