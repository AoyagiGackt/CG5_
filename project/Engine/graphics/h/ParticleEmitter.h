/**
 * @file ParticleEmitter.h
 * @brief 指定した座標から定期的にパーティクルを発生させるエミッター（発生源）を管理するファイル
 */
#pragma once
#include "MakeAffine.h"
#include "ParticleManager.h"

/**
 * @brief パーティクルの発生源を表現するクラス
 * @note 毎フレーム Update() を呼び出すことで、設定された頻度（frequency_）に従って
 * 自動的に ParticleManager 経由でパーティクルを生成します。
 */
class ParticleEmitter {
public:

    /**
     * @brief コンストラクタ。発生させるパーティクルの種類と初期位置を設定する
     * @param name 発生させるパーティクルグループの名前（ParticleManagerで事前登録したもの）
     * @param transform エミッターの初期トランスフォーム（座標・回転・スケール）
     */
   ParticleEmitter(const std::string& name, const Transform& transform);

    /**
    * @brief 毎フレームの更新処理。内部タイマーを進め、指定時間が経過したらパーティクルを発生させる
    */
    void Update();

    /**
     * @brief 手動でパーティクルを1回（1フレーム分）発生させる
     * @note Update() の自動発生とは別に、任意のタイミングで出したい時に呼び出す
     */
    void Emit();

    /**
     * @brief エミッターの発生位置（座標）を設定する
     * @param translate 新しいX, Y, Z座標
     * @note キャラクターの移動に合わせてエミッターを追従させる場合などに使用します。
     */
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

private:

    /** @brief 発生させるパーティクルグループの名前 */
    std::string name_;

    /** @brief エミッター自身のトランスフォーム（主に発生座標として使用） */
    Transform transform_;

    /** @brief パーティクルを発生させる間隔（秒）。デフォルトは0.5秒（0.5f）に1回 */
    float frequency_ = 0.5f;

    /** @brief 次の発生までの時間を計測する内部タイマー */
    float timeCount_ = 0.0f;
};