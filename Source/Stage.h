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
	VECTOR laserPos;
	VECTOR laserDir;
	LaserColor initialColor;
	VECTOR targetPos;
	float targetRadius;
	int maxMirrors;
	LaserColor requiredColor; // ★追加
	std::vector<Obstacle> obstacles;
	std::vector<Filter> filters; // ★追加

	StageData(const VECTOR& laserPos, const VECTOR& laserDir, const LaserColor& initialColor, const VECTOR& targetPos, float targetRadius, int maxMirrors, const LaserColor& requiredColor, const std::vector<Obstacle>& obstacles, const std::vector<Filter>& filters)
		: laserPos(laserPos), laserDir(laserDir), initialColor(initialColor), targetPos(targetPos), targetRadius(targetRadius), maxMirrors(maxMirrors), requiredColor(requiredColor), obstacles(obstacles), filters(filters)
	{
	}StageData() = default;

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
	float life;			// 残り寿命（フレーム数）
	float maxLife;		// 最大寿命
	unsigned int color; // 色
};

class Stage {
private:
	Laser* m_laser;
	LaserTarget* m_target;
	std::vector<Mirror> m_mirrors;
	std::vector<Obstacle> m_obstacles;
	std::vector<Filter> m_filters;

	int m_maxMirrors;
	int m_clearTimer;
	bool m_isDragging;
	VECTOR m_dragStartPos;

	// 💡 【追加】Stage側でパーティクルを一元管理する
	std::vector<DotParticle> m_particles;
	void SpawnEmitParticles(const VECTOR& emitPos, unsigned int color);
public:
	Stage(const StageData& data);
	~Stage();

	bool Update(const VECTOR& mousePos, int mouseInput, int prevMouseInput); // 返り値：ステージクリアしたら true
	void Draw(bool isDragging, const VECTOR& dragStartPos, const VECTOR& dragCurrentPos);

	bool IsTargetHit() const;
	void AddMirror(const VECTOR& start, const VECTOR& end);

	// GameScene側でUI（残り鏡の枚数など）を表示するためのゲッター
	int GetRemainingMirrors() const { return m_maxMirrors - (int)m_mirrors.size(); }
	int GetMaxMirrors() const { return m_maxMirrors; }
};