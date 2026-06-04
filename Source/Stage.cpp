#include "Stage.h"
#include "Utility.h"
#include <fstream>
#include <sstream>

float GetDistanceLineToPoint(VECTOR lineStart, VECTOR lineEnd, VECTOR point) {
	float A = point.x - lineStart.x;	// 点と線分の始点を結ぶベクトルのx成分
	float B = point.y - lineStart.y;	// 点と線分の始点を結ぶベクトルのy成分
	float C = lineEnd.x - lineStart.x;	// 線分のx成分
	float D = lineEnd.y - lineStart.y;	// 線分のy成分
	float dot = A * C + B * D;// 点と線分の始点を結ぶベクトルと、線分のベクトルの内積
	float lenSq = C * C + D * D; // 線分の長さの二乗
	float param = (lenSq != 0) ? (dot / lenSq) : -1;	// 線分が点の場合は-1を返す
	float nearestX, nearestY;	// 線分上の最近接点の座標
	// 線分上の最近接点を計算
	if (param < 0) {
		// 最近接点は線分の始点
		nearestX = lineStart.x;	
		nearestY = lineStart.y;
	}
	else if (param > 1) {
		// 最近接点は線分の終点
		nearestX = lineEnd.x;
		nearestY = lineEnd.y;
	}
	else {
		// 最近接点は線分上の点
		nearestX = lineStart.x + param * C;
		nearestY = lineStart.y + param * D;
	}

	// 点と最近接点の距離を計算して返す
	return sqrtf((point.x - nearestX) * (point.x - nearestX) + (point.y - nearestY) * (point.y - nearestY));
}

// 💡 ここで、Stageクラスのメンバ関数 SpawnEmitParticles を実装します
void Stage::SpawnEmitParticles(const VECTOR& emitPos, unsigned int color, const VECTOR& normal) {
	int spawnCount = 4; // 毎フレーム散らす数

	// 壁の法線ベクトルを元に、パーティクルの発射角度を決定します。
	for (int i = 0; i < spawnCount; ++i) {
		// Particleの基本情報をセットアップ
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

		// 4. 最終的な速度ベクトルを、計算した角度とスピードから決定します
		p.velocity = VGet(std::cosf(finalAngle) * speed, std::sinf(finalAngle) * speed, 0.0f);
		p.maxLife = static_cast<float>(10 + GetRand(15)) / 60.f; // 寿命は10〜25フレーム（少し短くしてキレを良くする）
		p.life = p.maxLife;
		p.color = color;

		// 5. 最後に、Stageのパーティクル管理配列に追加します
		m_particles.push_back(p);
	}
}

Stage::Stage(const StageData& data)
	: m_laser(nullptr)
	, m_target(nullptr)
	, m_maxMirrors(data.maxMirrors)
	, m_clearTimer(0)
	, m_hitObstacleIndex(-1) // 💡 【追加】初期化
	, m_isHitObstacle(false) // 💡 【追加】
	, m_hitObstaclePos(VGet(0, 0, 0)) // 💡 【追加】
{
	m_laser = new Laser(data.laserPos, data.laserDir, data.initialColor);					// 💡 レーザークラスのインスタンスを生成してポインタに格納
	m_target = new LaserTarget(data.targetPos, data.targetRadius, data.requiredColor);		// 💡 的クラスのインスタンスを生成してポインタに格納
	m_obstacles = data.obstacles;	// 💡 壁の配列をコピー
	m_filters = data.filters;		// 💡 フィルターの配列をコピー
}

Stage::~Stage() {
	// 💡 動的に生成したLaserとLaserTargetのインスタンスを解放します
	if (m_laser != nullptr)  delete m_laser;
	if (m_target != nullptr) delete m_target;
}

bool Stage::IsTargetHit() const {
	// 💡 的に当たっているかを返す関数。レーザークラスのDraw()内で、的に当たったときにLaserTargetのSetHit()が呼ばれているので、そのフラグをここで返すだけでOKです。
	if (m_target == nullptr) return false;
	return m_target->IsHit();
}

void Stage::AddMirror(const VECTOR& start, const VECTOR& end) {
	// 💡 鏡を追加する関数。引数で始点と終点の座標を受け取ります。
	if (VSquareSize(VSub(end, start)) <= 20.f * 20.f)
		return;	// 鏡の長さが20ピクセル以下なら追加しない（短すぎて操作しづらいので）

	// 💡 鏡の最大設置数を超えていなければ、m_mirrors配列に新しいMirrorインスタンスを追加します。鏡を追加したらレーザーの軌道もリセットする必要があるので、m_laserがnullptrでないことを確認してからReset()を呼びます。
	if ((int)m_mirrors.size() < m_maxMirrors) {
		m_mirrors.push_back(Mirror(start, end));
		if (m_laser != nullptr) {
			m_laser->Reset();
		}
	}
}

bool Stage::Update(const VECTOR& mousePos, int mouseInput, int prevMouseInput) {
	
	// 💡 Update関数の冒頭で、まずは全ての鏡を「非選択状態」にリセットしてから、マウスに一番近い鏡を探して選択状態にする処理を行います。
	int targetIndex = -1;
	float minDistance = 999999.0f;

	// 💡 まずは全ての鏡を非選択状態にリセットします。これによって、毎フレーム「どの鏡が選択されているか」が完全に確定するようになります。
	for (auto& mirror : m_mirrors) {
		mirror.SetSelect(false);
		mirror.Update();
	}

	// 💡 ここで、マウスの中クリック（ホイールクリック）が押された瞬間を検知して、レーザーの軌道をリセットする処理を追加します。これによって、ユーザーはいつでもレーザーの軌道をリセットできるようになります。
	if (mouseInput & MOUSE_INPUT_MIDDLE && !(prevMouseInput & MOUSE_INPUT_MIDDLE))
	{
		m_isFiring = m_isFiring ? false : true; // 中クリックが押された瞬間に、レーザーの発射状態をトグル（切り替え）します
		if (m_laser != nullptr) {
			m_laser->Reset(); // レーザーの軌道をリセットします
			m_clearTimer = 0; // クリアタイマーもリセットします
			m_particles.clear(); // パーティクルも全て消去します
		}
	}

	// 💡 ここで、レーザークラスのUpdate()も呼び出しておきます。これによって、鏡の状態が変わったときにレーザーの軌道も最新の状態に更新されるようになります。
	if (m_isFiring)
		m_laser->Update();

	// 1. マウスに一番近い鏡を探索
	for (int i = 0; i < (int)m_mirrors.size(); ++i) {
		// 💡 鏡の線分とマウス座標の距離を計算して、最も近い鏡を探します。距離が12ピクセル以内なら選択対象になります。
		float dist = GetDistanceLineToPoint(m_mirrors[i].GetStart(), m_mirrors[i].GetEnd(), mousePos);
		if (dist < 12.0f && dist < minDistance) {
			minDistance = dist;
			targetIndex = i;
		}
	}

	// 💡 鏡の操作によってレーザーの軌道が変わる可能性があるので、鏡の操作の前に「レーザーのリセットが必要かどうか」を判断するフラグを用意しておきます。
	bool isLaserResetRequired = false;

	// 2. 鏡の操作
	if (targetIndex != -1) {
		// 💡 最も近い鏡が見つかった場合、その鏡を選択状態にして、右クリックで削除、左クリックで角度変更の操作を行います。
		m_mirrors[targetIndex].SetSelect(true);

		// 💡 右クリックで削除の処理。右クリックが押された瞬間（前フレームは押されていない）に、その鏡をm_mirrors配列から削除します。
		// 鏡を削除したらレーザーの軌道もリセットする必要があるので、isLaserResetRequiredフラグをtrueにします。
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

	// 💡 鏡の操作があった場合や、レーザーの軌道に影響を与えるような変更があった場合は、レーザークラスのReset()を呼び出して軌道をリセットします。
	if (m_laser != nullptr && isLaserResetRequired) {
		m_laser->Reset();
		m_clearTimer = 0;
		m_particles.clear();
		m_isFiring = false; // 鏡を操作したらレーザーの発射を一旦止める（ユーザーが再度中クリックで発射するまで待つ）
	}

	// 💡 【超重要】クリア判定の前に、Updateの段階で衝突フラグを完全に確定させる！
	if (m_target != nullptr) {
		m_target->ResetHitState(); // まず綺麗にする
	}
	bool isCurrentlyHitting = false; // 今この瞬間当たっているかのローカルフラグ
	if (m_laser != nullptr && m_target != nullptr) {
		// レーザーに擬似計算を走らせて的へのヒット状況を「実更新」する
		m_laser->Draw(m_mirrors, m_obstacles, m_filters, *m_target);

		// Stage.cpp の Update() 関数の最後の方（衝突シミュレーション直後）

			// Laserクラスから最新の軌跡履歴を貰う
		const auto& history = m_laser->GetHistory();
		// まずは「壁に当たっているかどうか」のフラグをリセットしておきます。これによって、毎フレーム「どこに当たっているか」が完全に確定するようになります。
		m_hitObstacleIndex = -1;

		// 💡 もしレーザーの軌跡履歴が空でなければ、最後の座標をチェックして、壁に当たっているかどうかを判断します。
		if (!history.empty()) {
			// 配列の最後（末尾）の要素が、レーザーの先端（行き止まり）
			const auto& lastHistory = history.back();

			int index = 0;
			// 💡 もしその先端の座標の周囲に壁（Obstacle）が存在するなら、
			// そこが「壁に当たって止まっている場所」と判断できます！
			for (const auto& obstacle : m_obstacles) {
				// 壁の線分と先端の座標の距離がほぼ0（接触している）かチェック
				// もしくは、Laserクラス側で「最後は壁で終わったフラグ」を持たせておくとより確実です
				float dist = GetDistanceLineToPoint(obstacle.GetStart(), obstacle.GetEnd(), lastHistory.position);

				// 💡 もし距離が5ピクセル未満なら、壁に当たっていると判断します。距離の閾値は、レーザーの太さや描画の誤差を考慮して、数ピクセル程度に設定すると良いでしょう。
				if (isCurrentlyHitting = m_isHitObstacle = (dist < 5.0f)) { // 壁に当たっている

					// 💡 当たっている壁のインデックスと、当たっている座標を保存しておきます。これによって、Draw()関数で「どこに当たっているか」が完全に確定するようになります。
					m_hitObstacleIndex = index;
					m_hitObstaclePos = lastHistory.position;
					// 💡 ここで、当たっている壁の法線ベクトルを取得します。これによって、パーティクルの発生方向を壁の向きに合わせることができます。
					VECTOR normal = obstacle.GetNormal();
					// 長さを1に正規化（揃える）
					float len = VSize(normal);
					// もし長さが0でないなら、法線ベクトルを正規化します。これによって、パーティクルの発生方向が安定します。
					if (len > 0.0f) normal = VScale(normal, 1.0f / len);

					// 💡 レーザーの飛んできた向き（lastHistory.direction）と逆を向くように、法線の向きを補正
					if (VDot(lastHistory.direction, normal) > 0.0f) {
						// レーザーの進行方向と法線が同じ向きを向いている場合は、法線を反転させます。これによって、パーティクルが壁の正面側に噴き出すようになります。
						normal = VScale(normal, -1.0f);
					}

					// 💡 壁の向きを渡してパーティクル発生！
					unsigned int PtColor = GetColor(255, 100, 100);
					// 💡 もしレーザーの色が緑や青なら、パーティクルの色もそれに合わせて変えるとより見た目が良くなります！
					if(lastHistory.Color == LaserColor::Green){
						PtColor = GetColor(100, 255, 100);
					}
					else if (lastHistory.Color == LaserColor::Blue)
					{
						PtColor = GetColor(100, 100, 255);
					}
					// ここで、パーティクル発生関数を呼び出します。引数には、発生位置（lastHistory.position）、色（PtColor）、そして壁の法線ベクトル（normal）を渡します。
					SpawnEmitParticles(lastHistory.position, PtColor, normal);
					break;
				}
				// 壁に当たっていない場合は、次の壁との距離をチェックするためにインデックスを増やします。
				index++;
			}
		}
	}
	
	// ----------------================================================-
	// ✨ 【ここがキモ！】着弾点の「余熱・発光」フェードイン・アウト処理
	// ----------------================================================-
	if (isCurrentlyHitting) {
		// 💡 レーザーが当たっている間は、約60フレーム（1秒）かけて「じわっ」と最大輝度へ加熱
		m_hitGlowAlpha += 1.0f / 60.0f;
		// 💡 もしフェードインの値が1.0fを超えたら、最大値で止める（これによって、ずっと当たっていても発光が強くなりすぎないようにします）
		if (m_hitGlowAlpha > 1.0f) m_hitGlowAlpha = 1.0f;
		// 揺らげる
		if (GetRand(100) <= 35)
		{
			// 💡 発光が強くなるにつれて、揺らぎの振れ幅も大きくなるように調整します。これによって、弱い発光は小さくチラチラと、強い発光は大きくドカンと揺れるようになり、よりダイナミックな表現になります。
			float tmp = 1.0f - m_hitGlowAlpha;
			// 💡 揺らぎの減衰も、発光が強くなるにつれてゆっくりになるように調整します。これによって、弱い発光はすぐに落ち着き、強い発光はなかなか落ち着かないようになり、よりリアルな表現になります。
			m_hitGlowAlpha -= (tmp + 0.15f) * (1.0f / (60.0f * 1.5f));
		}

	}
	else {
		// 💡 レーザーが逸れたら、約90フレーム（1.5秒）かけて「スーッ」と余熱が冷めるように消灯
		m_hitGlowAlpha -= 1.0f / (60.0f * 1.5f);
		// 💡 もしフェードアウトの値が0.0fを下回ったら、最小値で止める（これによって、完全に消えた後はマイナスにならないようにします）
		if (m_hitGlowAlpha < 0.0f) m_hitGlowAlpha = 0.0f;
	}

	// 💡 【追加】パーティクルの移動と寿命更新処理
	// ----------------================================================-
	// ✨ パーティクルの移動・重力・寿命更新処理
	// ----------------================================================-
	for (auto it = m_particles.begin(); it != m_particles.end(); ) {
		// 💡 【ここを追加！】火花の縦方向の速度（velocity.y）に、毎フレーム重力を加算する
		// 数値を大きくすると「ドサッ」と重く落ち、小さくすると「フワッ」とゆっくり落ちます。
		// 0.08f 〜 0.15f あたりでお好みの「軽さ」に調整してみてください！
		it->velocity.y += 0.12f;	// 重力の加算

		// 速度分だけ位置を移動させる
		it->pos = VAdd(it->pos, it->velocity);	// 位置の更新

		// 💡 減速（空気抵抗）の計算
		// 重力を加算した後に少しだけ減速させることで、
		// 勢いよく飛び出た火花が、だんだん綺麗な放物線を描いて落ちるようになります。
		it->velocity = VScale(it->velocity, 0.96f);	// 減速の加算
		// 寿命を減らす（フレームレートが60fpsの場合、1秒でlifeが0になるように調整）

		it->life -= 1.f / 60.f;	// 寿命の減少

		// 💡 もし寿命が0以下になったら、もしくは画面下（y > 600）に落ちたら、そのパーティクルは消滅させます。これによって、画面外に落ちた火花がいつまでも残ることがなくなります。
		if (it->life <= 0 || it->pos.y > Ut::SCREEN_HEIGHT) {
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

	// 1. 静的オブジェクトの描画
	// 💡 【ここを追加！】壁に当たっているなら、着弾点を中心に広がる光の円を描く
	if (m_isHitObstacle) {

		// 💡 まずは、レーザーが当たっている壁の上に、火花のパーティクルを散らします。これによって、レーザーが当たった瞬間の「ドカッ」という感じがよりリアルになります。
		for (const auto& Particle : m_particles)
		{
			// 💡 パーティクルは、位置（Particle.pos）と色（Particle.color）を元に、DrawPixel関数で描画します。これによって、火花が壁の上でキラキラと輝くようになります。
			DrawPixel(
				static_cast<int>(Particle.pos.x),
				static_cast<int>(Particle.pos.y),
				Particle.color
			);
		}

		// 💡 そして、火花のパーティクルを描いた後に、着弾点を中心に広がる光の円を描きます。これによって、レーザーが当たった場所が「ピカッ」と明るく輝くようになります。
		if (m_hitGlowAlpha > 0.0f) {
			unsigned int colorGlow = GetColor(255, 215, 0); // 鮮やかなゴールド・イエロー

			// 💡 1. 中心の明るい核（最大不透明度 140 に、現在のフェード率をかける）
			int alphaCenter = static_cast<int>(140 * m_hitGlowAlpha);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaCenter);
			DrawCircleAA(m_hitObstaclePos.x, m_hitObstaclePos.y,
				5, 360, colorGlow, TRUE);

			// 💡 2. 周囲の大きな光の広がり（最大不透明度 60 に、現在のフェード率をかける）
			int alphaOuter = static_cast<int>(60 * m_hitGlowAlpha);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaOuter);

			// 💡 広がる感じをさらに強化：発光が強くなる（m_hitGlowAlphaが増える）につれて、
			// 円の半径自体も「10px ➔ 15px」へ、ポッと膨らむように変化させると最高に気持ちいいです！
			float currentRadius = 10 + 5 * m_hitGlowAlpha;
			DrawCircleAA(m_hitObstaclePos.x, m_hitObstaclePos.y,
				currentRadius, 360, colorGlow, TRUE);

			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドを元に戻す
		}
	}

	// ドラッグ中のプレビュー線
	if (isDragging) {
		// 💡 ドラッグ中は、マウスの始点と現在位置を結ぶ線を描画して、鏡の設置プレビューを表示します。これによって、鏡をどこに設置するかが視覚的にわかりやすくなります。
		DrawLineAA(dragStartPos.x, dragStartPos.y, dragCurrentPos.x, dragCurrentPos.y, GetColor(255, 255, 0), 2);
	}

	// 2. 描画処理（ここではResetHitStateを絶対に呼ばない）
	if (m_laser != nullptr && m_target != nullptr) {
		// 💡 レーザークラスのDraw()内で、レーザーの軌道を計算して描画する際に、鏡や壁、フィルターとの衝突判定も行います。そして、的へのヒット状況も更新されます。
		m_laser->Draw(m_mirrors, m_obstacles, m_filters, *m_target);

		// 💡 もしレーザーが発射されていない状態なら、レーザーの先端に「Firing...」の文字と赤い線を描画して、レーザーが発射されていることを視覚的にわかりやすくします。
		// これによって、ユーザーはレーザーがどこからどこへ向かっているのかを一目で把握できるようになります。
		if (!m_isFiring)
		{
			// 💡 レーザーの色に合わせて、線と文字の色も変えるとより見た目が良くなります！
			auto color = GetColor(255, 0, 0);
			if (m_laser->GetInitialColor() == LaserColor::Green)
				color = GetColor(0, 255, 0);
			else if (m_laser->GetInitialColor() == LaserColor::Blue)
				color = GetColor(0, 0, 255);

			DrawStringF(m_laser->GetPosition().x, m_laser->GetPosition().y + 10, "Firing...", GetColor(255, 255, 255));
			DrawLineAA(m_laser->GetPosition().x, m_laser->GetPosition().y,
				(m_laser->GetPosition().x + m_laser->GetDirection().x * 20),
				(m_laser->GetPosition().y + m_laser->GetDirection().y * 20),
				color, 3);
			DrawTriangleAA(
				(m_laser->GetPosition().x + m_laser->GetDirection().x * 22),
				(m_laser->GetPosition().y + m_laser->GetDirection().y * 22),
				(m_laser->GetPosition().x + m_laser->GetDirection().x * 17 - m_laser->GetDirection().y * 5),
				(m_laser->GetPosition().y + m_laser->GetDirection().y * 17 + m_laser->GetDirection().x * 5),
				(m_laser->GetPosition().x + m_laser->GetDirection().x * 17 + m_laser->GetDirection().y * 5),
				(m_laser->GetPosition().y + m_laser->GetDirection().y * 17 - m_laser->GetDirection().x * 5),
				color, TRUE);
			DrawCircleAA(m_laser->GetPosition().x, m_laser->GetPosition().y, 5, 360, color, TRUE);
		}
	}
	if (m_target != nullptr) {
		// 💡 的の描画は、レーザークラスのDraw()の後に行います。これによって、レーザーが的に当たったときに、的の見た目を変えることができます。
		m_target->Draw(!m_filters.empty());
	}
}