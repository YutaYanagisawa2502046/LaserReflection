#include <vector>
#include <string>
#include "DxLib.h"
#include "Mirror.h"
#include "Obstacle.h"
#include "Filter.h"
#include "Laser.h"
#include "LaserTarget.h"

// 前に作った StageData 構造体
struct StageData {
	VECTOR laserPos;			// レーザーの発射位置
	VECTOR laserDir;			// レーザーの初期進行方向
	LaserColor initialColor;	// レーザーの初期色
	VECTOR targetPos;			// 的の位置
	float targetRadius;			// 的の半径
	int maxMirrors;				// 鏡の最大設置数
	LaserColor requiredColor;	// 的が要求する色
	std::vector<Obstacle> obstacles;	// 壁の配列
	std::vector<Filter> filters;		// フィルターの配列

	StageData(const VECTOR& laserPos, const VECTOR& laserDir, const LaserColor& initialColor, const VECTOR& targetPos, float targetRadius, int maxMirrors, const LaserColor& requiredColor, const std::vector<Obstacle>& obstacles, const std::vector<Filter>& filters)
		: laserPos(laserPos), laserDir(laserDir), initialColor(initialColor), targetPos(targetPos), targetRadius(targetRadius), maxMirrors(maxMirrors), requiredColor(requiredColor), obstacles(obstacles), filters(filters)
	{
	}StageData() = default;

	// デバッグ用のステージデータを返す関数
	StageData ClearDebugStage()
	{
		return StageData(
			VGet(100, 300, 0), VGet(1, 0, 0),        // レーザー：左から右へ
			LaserColor::Red,
			VGet(700, 300, 0), 25.0f,                // 的：右端、要求色は青
			0, LaserColor::Blue,                      // 鏡は0枚、クリア条件：青
			{},                                      // 壁：なし
			{
				Filter(VGet(400, 100, 0), VGet(400, 500, 0), LaserColor::Blue)
			} // 中央に縦の青フィルター
		);
	};
};

// 💡 ドット1つの情報
struct DotParticle {
	VECTOR pos;			// 位置
	VECTOR velocity;	// 移動速度と方向
	float life;			// 残り寿命秒数
	float maxLife;		// 最大寿命
	unsigned int color; // 色
};

class Stage {
private:
	Laser* m_laser;						// 💡 レーザークラスのポインタ
	LaserTarget* m_target;				// 💡 的クラスのポインタ
	std::vector<Mirror> m_mirrors;		// 💡 鏡はクラスで管理しているので、単純に配列で持つだけでOK
	std::vector<Obstacle> m_obstacles;	// 💡 壁もクラスで管理しているので、単純に配列で持つだけでOK
	std::vector<Filter> m_filters;		// 💡 フィルターもクラスで管理しているので、単純に配列で持つだけでOK

	int m_maxMirrors;		// 💡 ステージごとに鏡の最大設置数を管理する変数
	int m_clearTimer;		// 💡 ステージクリア後の待機時間を管理する変数（フレーム数でカウント）
	bool m_isDragging;		// 💡 ドラッグ中かどうかのフラグ
	VECTOR m_dragStartPos;	// 💡 ドラッグ開始位置を記録する変数

	bool m_isFiring;		// 💡 レーザーが発射されているかどうかのフラグ

	int m_hitObstacleIndex; // 💡 【追加】現在レーザーが当たっている壁のインデックス（-1はどこにも当たっていない）
	bool m_isHitObstacle;   // 💡 【追加】いま壁に当たっているかどうかのフラグ
	VECTOR m_hitObstaclePos;// 💡 【追加】レーザーが壁に激突しているまさにその座標
	float m_hitGlowAlpha;   // 💡 【変更】0.0f（消灯）〜 1.0f（最大発光）を滑らかに動くタイマー

	// 💡 【追加】Stage側でパーティクルを一元管理する
	std::vector<DotParticle> m_particles;	// 💡 パーティクルを管理する配列
	void SpawnEmitParticles(const VECTOR& emitPos, unsigned int color, const VECTOR& normal); // 💡 パーティクルを発生させる関数（位置、色、そして壁の法線ベクトルを引数に取る）
public:
	Stage(const StageData& data);
	~Stage();

	bool Update(const VECTOR& mousePos, int mouseInput, int prevMouseInput); // 返り値：ステージクリアしたら true
	void Draw(bool isDragging, const VECTOR& dragStartPos, const VECTOR& dragCurrentPos);

	bool IsTargetHit() const;	// 的に当たっているかのゲッター
	void AddMirror(const VECTOR& start, const VECTOR& end);	// 鏡を追加する関数

	// GameScene側でUI（残り鏡の枚数など）を表示するためのゲッター
	int GetRemainingMirrors() const { return m_maxMirrors - (int)m_mirrors.size(); }	// 残り設置可能な鏡の枚数を計算して返す関数
	int GetMaxMirrors() const { return m_maxMirrors; }	// ステージの最大鏡設置数を返す関数
};