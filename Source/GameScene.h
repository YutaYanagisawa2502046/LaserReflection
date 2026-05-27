#pragma once
#include "Scene.h"
#include "DxLib.h"
#include <vector>
#include "Stage.h" // 💡 新しいStageクラスをインクルード

class GameScene : public Scene {
private:
    std::vector<StageData> m_stages; // メモ帳などから読み込んだステージデータベース
    Stage* m_currentStage;           // 💡 現在動いているステージの実体
    int m_currentStageIndex;         // 現在のステージ番号

    // --- GameScene.h 内の GameState 定義部分 ---
    enum class GameState {
        Playing,
        Clear,
        FadeIn  // 💡 【追加】次のステージが始まった直後のフェードイン状態
    };

    GameState m_state;
    int m_clearTimer;               // 💡 確実に届いているか確認する GameScene 側のタイマー
    float m_stateTransitionTimer;   // 次のステージへの遷移タイマー

    // ドラッグ（鏡の生成）に関する入力状態
    bool m_isDragging;
    VECTOR m_dragStartPos;
    VECTOR m_dragCurrentPos;

    // 💡 UI用のフォントハンドルを追加
    int m_fontUiMain;  // ステージ番号や大きなメッセージ用
    int m_fontUiSub;   // 手持ちの鏡や操作説明の本文用

    void LoadStage(int index);      // 指定したインデックスのステージを読み込む

public:
    GameScene();
    virtual ~GameScene();

    virtual void Initialize() override;
    virtual SceneName Update() override;
    virtual void Draw() override;
};