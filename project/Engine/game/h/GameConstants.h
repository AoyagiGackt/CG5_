/**
 * @file GameConstants.h
 * @brief ゲーム全体で使う「決まった数値」をひとまとめにしたファイル
 *
 * コード中に数値を直書き（例: 60.0f, 3.14159f）すると
 * 「この数字は何を意味するの？」となって読みにくくなる。
 * ここに名前をつけて登録しておけば、意味がわかりやすく、
 * 値を変えたいときもこの1か所を直すだけでゲーム全体に反映される。
 */
#pragma once

namespace GameConstants {

    // -------------------------------------------------------
    // フレームレート（画面を1秒間に何回更新するか）
    // -------------------------------------------------------

    // 目標フレームレート。60 = 1秒間に60回描画する
    inline constexpr float kTargetFps      = 60.0f;

    // 1フレームあたりの経過時間（秒）。60FPSなら約0.0167秒
    // 「Update の中で時間を進める」ときに使う基準値
    inline constexpr float kFrameDeltaTime = 1.0f / kTargetFps;

    // -------------------------------------------------------
    // 画面サイズ（1280×720 ピクセル固定）
    // -------------------------------------------------------

    // 画面の横方向の中心（ピクセル単位）
    inline constexpr float kScreenCenterX = 640.0f;

    // 画面の縦方向の中心（ピクセル単位）
    inline constexpr float kScreenCenterY = 360.0f;

    // -------------------------------------------------------
    // 数学定数
    // -------------------------------------------------------

    // 円周率 π（パイ）。360度 = 2π ラジアン
    inline constexpr float kPi    = 3.14159265358979323846f;

    // 2π（= 360度）。1周分の角度をラジアンで表したもの
    inline constexpr float kTwoPi = kPi * 2.0f;

    // -------------------------------------------------------
    // 楕円パーティクル（リングの周りをぐるぐる回る光の粒）
    // -------------------------------------------------------

    // リングを1周するのにかかる秒数
    inline constexpr float kOrbitPeriodSeconds  = 3.0f;

    // 円周方向への移動速度（大きいほど速く飛ぶ）
    inline constexpr float kEllipseTangentSpeed = 0.03f;

    // 上方向への浮き上がり速度（大きいほど高く上がる）
    inline constexpr float kEllipseYVelocity    = 0.04f;

    // パーティクルが消えるまでの時間（秒）
    inline constexpr float kEllipseLifetime     = 1.5f;

    // パーティクルの基本の大きさ
    inline constexpr float kEllipseBaseScale    = 0.4f;

    // 大きさのランダムなばらつき幅（kEllipseBaseScale ± この値）
    inline constexpr float kEllipseScaleRandom  = 0.2f;

    // -------------------------------------------------------
    // 白パーティクル（画面全体にふわふわ漂う白い粒）
    // -------------------------------------------------------

    // パーティクルをばら撒く範囲の半径（単位はワールド座標）
    inline constexpr float kWhiteParticleScatterRadius = 20.0f;

    // パーティクルが消えるまでの最短時間（秒）
    inline constexpr float kWhiteParticleLifetimeMin   = 2.0f;

    // パーティクルが消えるまでの最長時間（秒）
    inline constexpr float kWhiteParticleLifetimeMax   = 5.0f;
}
