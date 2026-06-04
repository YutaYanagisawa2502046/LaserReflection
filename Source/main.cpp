#include "DxLib.h"
#include "SceneManager.h"
#include "Utility.h"
#include "Master.h"

auto Master::m_soundManager = new SoundManager();

// Windowsアプリのエントリーポイント（ここからプログラムが始まります）
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    // ---- 1. DXライブラリの初期化前設定 ----
    ChangeWindowMode(FALSE);             // ウィンドウモードで起動（FALSEにするとフルスクリーン）
    SetGraphMode(Ut::SCREEN_WIDTH, Ut::SCREEN_HEIGHT, 32);         // 画面サイズを 640x480、カラーを32bitに設定
    SetMainWindowText("レーザー反射ゲーム"); // ウィンドウのタイトルバーのテキストを設定

    // ---- 2. DXライブラリの初期化 ----

    if (DxLib_Init() == -1) {
        return -1; // 初期化に失敗したら直ちに終了
    }

    SetMouseDispFlag(TRUE);

	// SEデータの読み込み：Masterクラスのサウンドマネージャーに、レーザーSEのファイルを登録します
    Master::m_soundManager->AddSEData("Resource/SE/sen_mi_lasergun04.mp3",SE::LASER);

    // ---- 3. 描画先の設定（裏画面） ----

    // 画面のチラつきを抑えるダブルバッファリング（裏画面に描画して一気に表へ反映）を有効化
    SetDrawScreen(DX_SCREEN_BACK);

    // ---- 4. ゲームシステムの初期化 ----
	// ここでは、シーンマネージャーを初期化して、最初のシーン（タイトル）をロードします
    SceneManager sceneManager;
    sceneManager.Initialize(); // シーンマネージャーを初期化（最初のシーンをロード）

    // ---- 5. メインループ ----

    // ProcessMessage: Windowsのシステムメッセージ処理（ウィンドウの「×」ボタン対応など）
    // ClearDrawScreen: 裏画面のデータを毎フレーム綺麗に消去
    // CheckHitKey: エスケープキーが押されているかチェック（1で押されている状態）
    // これらがいずれも正常、かつESCキーが押されていない間ループを繰り返す
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {

        // ① 更新処理（計算や入力受付）
        sceneManager.Update();

        // ② 描画処理（裏画面への書き込み）
        sceneManager.Draw();

		// ③ フレームレート制御（Ut::FPSに固定）
        WaitTimer(1000 / Ut::FPS);

        // ③ 画面反映（裏画面の内容を実際のディスプレイ＝表画面に転送）
        ScreenFlip();
    }

    // ---- 6. 終了処理 ----

	// ゲームループを抜けたら、シーンマネージャーやサウンドマネージャーなどのリソースを解放します
    delete Master::m_soundManager;

    DxLib_End(); // DXライブラリの仕様終了に伴うクリーンアップ処理

    return 0; // プログラムを正常終了
}