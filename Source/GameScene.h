#pragma once
#include "Scene.h"
#include "DxLib.h"
#include <vector>

class Mirror;
class Laser;
class LaserTarget;
class Obstacle;

struct StageData {
    VECTOR laserPos;       // レーザーの初期位置
    VECTOR laserDir;       // レーザーの初期方向
    VECTOR targetPos;      // 的の位置
    float targetRadius;    // 的のサイズ
    int maxMirrors;        // 使える鏡の枚数

    // このステージにある壁のリスト
    std::vector<Obstacle> obstacles;
};

class GameScene : public Scene {
private:
    Laser* m_laser;                  // レーザーのインスタンス
    LaserTarget* m_target;
    std::vector<Mirror> m_mirrors;  // 複数の反射板を管理する動的配列
    std::vector<Obstacle> m_obstacles;

    // ---- 鏡の設置制限用の追加変数 ----
    int m_maxMirrors;   // このステージで設置できる鏡の最大数

    // ---- 鏡設置用の追加変数 ----
    bool m_isDragging;       // 現在マウスでドラッグ中かどうかのフラグ
    VECTOR m_dragStartPos;   // ドラッグを開始した座標（クリックした位置）
    VECTOR m_dragCurrentPos; // 現在のマウス座標（ドラッグ中のプレビュー用）

    // ★【新規】ゲームの進行状態を表す列挙型
    enum class GameState {
        Playing,    // パズルを解いている最中
        Clear       // ステージクリアの演出中
    };

    GameState m_state;          // 現在の進行状態
    int m_clearTimer;           // クリア判定用のカウントタイマー
    int m_stateTransitionTimer; // ステージ切り替え演出用のタイマー

    // これをゲームシーン側で配列（ベクター）として持っておく
    std::vector<StageData> m_stages;
    int m_currentStageIndex = 0; // 今ステージいくつ？

    // --- GameScene.h の内部に以下を追加 ---
private:
    // 【追加】線分 (p1-p2) と点 (pt) の最短距離を計算する関数（鏡のクリック判定用）
    float GetDistanceLineToPoint(VECTOR p1, VECTOR p2, VECTOR pt);

    // ★【新規】指定した番号のステージを読み込む関数
    void LoadStage(int stageIndex);

public:
    GameScene();
    virtual ~GameScene();

    virtual void Initialize() override;
    virtual SceneName Update() override;
    virtual void Draw() override;
};