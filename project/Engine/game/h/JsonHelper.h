/**
 * @file JsonHelper.h
 * @brief JSON ファイルから値を読み取るための簡易ヘルパー関数群
 *
 * JSON とは「テキストで設定値を書き残すための形式」のこと。
 * 例: { "camera_pos_x": 14.5, "count": 3 }
 *
 * フルスペックの JSON ライブラリを使わず、
 * シンプルな「キー: 値」形式だけを読み取る最小実装。
 * キーが見つからなければデフォルト値を返すので、ファイルが不完全でも安全に動く。
 */
#pragma once
#include <string>

namespace JsonHelper {
    // JSON 文字列 src から、キー key に対応する小数値を取り出す。
    // キーが見つからなければ def（デフォルト値）を返す。
    float ReadFloat(const std::string& src, const std::string& key, float def);

    // JSON 文字列 src から、キー key に対応する整数値を取り出す。
    // キーが見つからなければ def（デフォルト値）を返す。
    int ReadInt(const std::string& src, const std::string& key, int def);

    // JSON 文字列 src から、キー key に対応する文字列を取り出す。
    // キーが見つからなければ def（デフォルト値）を返す。
    std::string ReadString(const std::string& src, const std::string& key, const std::string& def);
}
