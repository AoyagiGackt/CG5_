#include "StringUtility.h"
#include <stringapiset.h> // MultiByteToWideChar / WideCharToMultiByte の宣言

namespace StringUtility {

// UTF-8 の string を wstring に変換する。
// MultiByteToWideChar は Windows API で「マルチバイト文字列 → ワイド文字列」の変換を行う関数。
// 2段階で呼ぶ理由: 1回目で「変換後に何文字必要か」を調べ、2回目で実際に変換する。
std::wstring ConvertString(const std::string& str)
{
    if (str.empty()) { return {}; } // 空文字列はそのまま空で返す

    // 1回目: 変換後のワイド文字数を取得する（バッファなし = サイズ計算だけ）
    auto n = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    if (n == 0) { return {}; } // 変換に失敗したら空を返す

    // 2回目: 必要な文字数分のバッファを確保して実際に変換する
    std::wstring result(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), n);
    return result;
}

// wstring を UTF-8 の string に変換する。
// WideCharToMultiByte は「ワイド文字列 → マルチバイト文字列」の変換を行う Windows API 関数。
std::string ConvertString(const std::wstring& str)
{
    if (str.empty()) { return {}; }

    // 1回目: 変換後のバイト数を取得する
    auto n = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    if (n == 0) { return {}; }

    // 2回目: 実際に変換する
    std::string result(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), n, nullptr, nullptr);
    return result;
}

}
