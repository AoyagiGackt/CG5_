/**
 * @file Logger.h
 * @brief ログ出力（デバッグ用文字列出力）に関する関数を定義するファイル
 */
#pragma once
#include "DirectXCommon.h"

/**
 * @namespace Logger
 * @brief ログ出力に関連するユーティリティ関数をまとめた名前空間
 */
namespace Logger {

/**
 * @brief ログメッセージを出力ウィンドウに表示する
 * @param message 出力したい文字列
 * @note 内部で OutputDebugString を呼び出し、Visual Studio の出力ウィンドウに
 * メッセージを表示します。
 */
void Log(const std::string& message);
}
