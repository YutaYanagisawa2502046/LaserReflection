#include "GameScene.h"
#include "DxLib.h"
#include "Mirror.h"
#include "Obstacle.h"
#include "Laser.h"
#include "LaserTarget.h"
#include <cmath>

GameScene::GameScene()
    : m_laser(nullptr)
    , m_target(nullptr)
    , m_isDragging(false)
    , m_dragStartPos(VGet(0, 0, 0))
    , m_dragCurrentPos(VGet(0, 0, 0))
    , m_currentStageIndex(0)
    , m_maxMirrors(0)
    , m_state(GameState::Playing) // ★初期状態はプレイ中
    , m_clearTimer(0)
    , m_stateTransitionTimer(0)
{
}

GameScene::~GameScene() {
    if (m_laser != nullptr)  delete m_laser;
    if (m_target != nullptr) delete m_target;
}

// Initialize や LoadStage の基本はそのままですが、少し追記します
void GameScene::Initialize() {
    m_stages.clear();

    // ---- ステージデータベース（前回作ったもの） ----
    StageData stage1;
    stage1.laserPos = VGet(50, 150, 0); stage1.laserDir = VGet(1.0f, 0.0f, 0);
    stage1.targetPos = VGet(550, 400, 0); stage1.targetRadius = 25.0f; stage1.maxMirrors = 2;
    stage1.obstacles.push_back(Obstacle(VGet(250, 0, 0), VGet(250, 300, 0)));
    m_stages.push_back(stage1);

    StageData stage2;
    stage2.laserPos = VGet(50, 450, 0); stage2.laserDir = VGet(1.0f, -0.5f, 0);
    stage2.targetPos = VGet(550, 100, 0); stage2.targetRadius = 20.0f; stage2.maxMirrors = 3;
    stage2.obstacles.push_back(Obstacle(VGet(300, 0, 0), VGet(300, 200, 0)));
    stage2.obstacles.push_back(Obstacle(VGet(300, 300, 0), VGet(300, 600, 0)));
    m_stages.push_back(stage2);

    // ★テスト用にもう1つ簡単なステージ3を追加（全クリア確認用）
    StageData stage3;
    stage3.laserPos = VGet(50, 300, 0); stage3.laserDir = VGet(1.0f, 0.0f, 0);
    stage3.targetPos = VGet(550, 300, 0); stage3.targetRadius = 30.0f; stage3.maxMirrors = 0; // 鏡なしでクリア！
    m_stages.push_back(stage3);

    m_currentStageIndex = 0;
    LoadStage(m_currentStageIndex);
}

void GameScene::LoadStage(int stageIndex) {
    if (stageIndex < 0 || stageIndex >= (int)m_stages.size()) return;

    const StageData& data = m_stages[stageIndex];
    if (m_laser != nullptr)  delete m_laser;
    if (m_target != nullptr) delete m_target;

    m_laser = new Laser(data.laserPos, data.laserDir, 20, 6.0f);
    m_target = new LaserTarget(data.targetPos, data.targetRadius);
    m_maxMirrors = data.maxMirrors;
    m_obstacles = data.obstacles;

    m_mirrors.clear();
    m_isDragging = false;

    // ★ステージ読み込み時にタイマーと状態をプレイ中に戻す
    m_state = GameState::Playing;
    m_clearTimer = 0;
    m_stateTransitionTimer = 0;
}

// 【追加】線分と点の最短距離を求める関数（Laser.cpp のものと同じです）
float GameScene::GetDistanceLineToPoint(VECTOR p1, VECTOR p2, VECTOR pt) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float lensq = dx * dx + dy * dy;
    if (lensq == 0.0f) return std::sqrt(std::pow(pt.x - p1.x, 2) + std::pow(pt.y - p1.y, 2));

    float t = ((pt.x - p1.x) * dx + (pt.y - p1.y) * dy) / lensq;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float closestX = p1.x + t * dx;
    float closestY = p1.y + t * dy;
    return std::sqrt(std::pow(pt.x - closestX, 2) + std::pow(pt.y - closestY, 2));
}

SceneName GameScene::Update() {
    // 全ステージクリア済みの場合は更新しない
    if (m_currentStageIndex >= (int)m_stages.size()) {
        if (CheckHitKey(KEY_INPUT_R)) { // Rキーで最初からリトライ
            m_currentStageIndex = 0;
            Initialize();
        }
        return SceneName::None;
    }

    if (m_target != nullptr && !m_target->IsHit()) m_target->ResetHitState();
    if (m_laser != nullptr)  m_laser->Update();

    // -----------------------------------------------------------------
    // 状態①：プレイ中の更新処理
    // -----------------------------------------------------------------
    if (m_state == GameState::Playing) {

        // （既存のマウスによる鏡の設置・個別削除処理をここにそのまま入れる）
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);
        m_dragCurrentPos = VGet((float)mouseX, (float)mouseY, 0.0f);
        static int prevMouseInput = 0;
        int mouseInput = GetMouseInput();

        if (mouseInput & MOUSE_INPUT_LEFT) {
            if (!m_isDragging && m_mirrors.size() < (size_t)m_maxMirrors) {
                m_isDragging = true;
                m_dragStartPos = m_dragCurrentPos;
            }
        }
        else {
            if (m_isDragging) {
                m_isDragging = false;
                float dx = m_dragCurrentPos.x - m_dragStartPos.x;
                float dy = m_dragCurrentPos.y - m_dragStartPos.y;
                if ((dx * dx + dy * dy) > 100.0f && m_mirrors.size() < (size_t)m_maxMirrors) {
                    m_mirrors.push_back(Mirror(m_dragStartPos, m_dragCurrentPos));
                    if (m_laser != nullptr) m_laser->Reset();
                }
            }
        }

        if ((mouseInput & MOUSE_INPUT_RIGHT) && !(prevMouseInput & MOUSE_INPUT_RIGHT)) {
            int targetIndex = -1;
            float minDistance = 999999.0f;
            for (int i = 0; i < (int)m_mirrors.size(); ++i) {
                float dist = GetDistanceLineToPoint(m_mirrors[i].GetStart(), m_mirrors[i].GetEnd(), m_dragCurrentPos);
                if (dist < 12.0f && dist < minDistance) { minDistance = dist; targetIndex = i; }
            }
            if (targetIndex != -1) {
                m_mirrors.erase(m_mirrors.begin() + targetIndex);
                if (m_laser != nullptr) m_laser->Reset();
            }
        }
        prevMouseInput = mouseInput;

        // ★【新規】クリア判定チェック
        // レーザーのDrawが走った後に的の IsHit() を見たいので、
        // このUpdateの最後、またはDrawの直後で判定します（DxLibの標準的な流れに合わせます）
        if (m_target != nullptr && m_target->IsHit()) {
            m_clearTimer++;
            if (m_clearTimer >= 30) { // 30フレーム（約0.5秒）当て続けたらクリア！
                m_state = GameState::Clear;
                m_isDragging = false; // ドラッグ中なら強制解除
            }
        }
        else {
            m_clearTimer = 0; // 外れたらタイマーリセット
        }
    }
    // -----------------------------------------------------------------
    // 状態②：クリア演出中の更新処理
    // -----------------------------------------------------------------
    else if (m_state == GameState::Clear) {
        m_stateTransitionTimer++;

        // クリアして90フレーム（約1.5秒）経ったら次のステージへ
        if (m_stateTransitionTimer >= 90) {
            m_currentStageIndex++;
            if (m_currentStageIndex < (int)m_stages.size()) {
                LoadStage(m_currentStageIndex); // 次のステージへ
            }
        }
    }

    // デバッグ用ステージ切り替えはプレイ中のみ有効にする
    if (m_state == GameState::Playing) {
        if (CheckHitKey(KEY_INPUT_1)) { m_currentStageIndex = 0; LoadStage(0); }
        if (CheckHitKey(KEY_INPUT_2)) { m_currentStageIndex = 1; LoadStage(1); }
    }

    return SceneName::None;
}

void GameScene::Draw() {
    // ---- エンディング画面（全ステージクリア時） ----
    if (m_currentStageIndex >= (int)m_stages.size()) {
        DrawString(200, 200, "🎉 ALL STAGE CLEAR !!! 🎉", GetColor(255, 255, 0));
        DrawString(180, 250, "おめでとうございます！天才レーザーパズラー誕生です！", GetColor(255, 255, 255));
        DrawString(230, 320, "【R】キーを押すと最初から遊べます", GetColor(150, 150, 150));
        return;
    }

    // UI描画
    char stageBuf[32];
    sprintf_s(stageBuf, "--- STAGE %d ---", m_currentStageIndex + 1);
    DrawString(10, 10, stageBuf, GetColor(255, 255, 0));
    DrawString(10, 30, "【操作】左ドラッグ：設置 / 鏡の上で右クリック：消去", GetColor(150, 150, 150));

    int remaining = m_maxMirrors - (int)m_mirrors.size();
    unsigned int uiColor = (remaining > 0) ? GetColor(255, 255, 255) : GetColor(255, 100, 100);
    char buf[64];
    sprintf_s(buf, "手持ちの鏡：あと %d 枚 / %d 枚", remaining, m_maxMirrors);
    DrawString(10, 60, buf, uiColor);

    // オブジェクト描画
    for (auto& mirror : m_mirrors) mirror.Draw();
    for (auto& obstacle : m_obstacles) obstacle.Draw();
    if (m_isDragging) DrawLine((int)m_dragStartPos.x, (int)m_dragStartPos.y, (int)m_dragCurrentPos.x, (int)m_dragCurrentPos.y, GetColor(255, 255, 0), 2);

    // レーザーと的の描画（これによって当たり判定のフラグが内部で立ちます）
    if (m_laser != nullptr && m_target != nullptr) m_laser->Draw(m_mirrors, m_obstacles, *m_target);
    if (m_target != nullptr) m_target->Draw();

    // ★【新規】ステージクリア時の画面エフェクト表示
    if (m_state == GameState::Clear) {
        // 画面中央に大きな文字を表示
        DrawString(250, 200, "STAGE CLEAR!!", GetColor(0, 255, 255));

        // ちょっとした演出：画面全体をふんわり白くする（フェードアウト風）
        // タイマーが進むほど白が濃くなる
        int alpha = m_stateTransitionTimer * 2;
        if (alpha > 150) alpha = 150;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(0, 0, 640, 480, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを戻す
    }
}