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

// 💡 引数に「壁の法線ベクトル (normal)」を追加！
void Stage::SpawnEmitParticles(const VECTOR& emitPos, unsigned int color, const VECTOR& normal) {
	int spawnCount = 4; // 毎フレーム散らす数

	for (int i = 0; i < spawnCount; ++i) {
		DotParticle p;
		p.pos = emitPos;

		// 1. 壁の向いている基本的な角度（ラジアン）を法線ベクトルから計算
		float baseAngle = std::atan2f(normal.y, normal.x);

		// 2. 💡【ここが肝！】基本の角度から、左右に「最大45度（±DX_PI_F / 4.f）」の範囲だけで散らす
		// これによって全方向ではなく、壁の正面側へ綺麗に噴き出します！
		float randomSpread = (static_cast<float>(GetRand(100)) / 100.0f - 0.5f) * (DX_PI_F / 2.0f);
		float finalAngle = baseAngle + randomSpread;

		// 3. スピードも少しランダムに
		float speed = 1.5f + static_cast<float>(GetRand(20)) / 10.0f; // 1.5 〜 3.5

		p.velocity = VGet(std::cosf(finalAngle) * speed, std::sinf(finalAngle) * speed, 0.0f);
		p.maxLife = static_cast<float>(10 + GetRand(15)) / 60.f; // 寿命は10〜25フレーム（少し短くしてキレを良くする）
		p.life = p.maxLife;
		p.color = color;

		m_particles.push_back(p);
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

		// Stage.cpp の Update() 関数の最後の方（衝突シミュレーション直後）

			// Laserクラスから最新の軌跡履歴を貰う
		const auto& history = m_laser->GetHistory();

		if (!history.empty()) {
			// 配列の最後（末尾）の要素が、レーザーの先端（行き止まり）
			const auto& lastHistory = history.back();

			// 💡 もしその先端の座標の周囲に壁（Obstacle）が存在するなら、
			// そこが「壁に当たって止まっている場所」と判断できます！
			for (const auto& obstacle : m_obstacles) {
				// 壁の線分と先端の座標の距離がほぼ0（接触している）かチェック
				// もしくは、Laserクラス側で「最後は壁で終わったフラグ」を持たせておくとより確実です
				float dist = GetDistanceLineToPoint(obstacle.GetStart(), obstacle.GetEnd(), lastHistory.position);

				if (dist < 5.0f) { // 壁に当たっている

					VECTOR normal = obstacle.GetNormal();
					// 長さを1に正規化（揃える）
					float len = VSize(normal);
					if (len > 0.0f) normal = VScale(normal, 1.0f / len);

					// 💡 レーザーの飛んできた向き（lastHistory.direction）と逆を向くように、法線の向きを補正
					if (VDot(lastHistory.direction, normal) > 0.0f) {
						normal = VScale(normal, -1.0f);
					}

					// 💡 壁の向きを渡してパーティクル発生！
					unsigned int PtColor = GetColor(255, 100, 100);
					if(lastHistory.Color == LaserColor::Green){
						PtColor = GetColor(100, 255, 100);
					}
					else if (lastHistory.Color == LaserColor::Blue)
					{
						PtColor = GetColor(100, 100, 255);
					}
					SpawnEmitParticles(lastHistory.position, PtColor, normal);
					break;
				}
			}
		}
	}

	// 💡 【追加】パーティクルの移動と寿命更新処理
	// ----------------================================================-
	// ✨ パーティクルの移動・重力・寿命更新処理
	// ----------------================================================-
	for (auto it = m_particles.begin(); it != m_particles.end(); ) {
		DrawPixel(
			static_cast<int>(it->pos.x),
			static_cast<int>(it->pos.y),
			it->color
		);

		// 💡 【ここを追加！】火花の縦方向の速度（velocity.y）に、毎フレーム重力を加算する
		// 数値を大きくすると「ドサッ」と重く落ち、小さくすると「フワッ」とゆっくり落ちます。
		// 0.08f 〜 0.15f あたりでお好みの「軽さ」に調整してみてください！
		it->velocity.y += 0.12f;

		// 速度分だけ位置を移動させる
		it->pos = VAdd(it->pos, it->velocity);

		// 💡 減速（空気抵抗）の計算
		// 重力を加算した後に少しだけ減速させることで、
		// 勢いよく飛び出た火花が、だんだん綺麗な放物線を描いて落ちるようになります。
		it->velocity = VScale(it->velocity, 0.96f);

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