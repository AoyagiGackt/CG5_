#include "JsonHelper.h"

namespace JsonHelper {

// JSON 文字列から小数値（float）を読み取る。
// 例: src に '{ "speed": 3.5 }' があり key = "speed" なら 3.5f を返す。
float ReadFloat(const std::string& src, const std::string& key, float def)
{
    // キー文字列 "key": を探す（JSON のキーはダブルクォートで囲まれている）
    std::string needle = "\"" + key + "\": ";
    auto pos = src.find(needle);

    // キーが見つからなければデフォルト値を返す
    if (pos == std::string::npos) { return def; }

    // キーの直後から数値文字列を取り出して変換する
    pos += needle.size();
    try { return std::stof(src.substr(pos)); } catch (...) { return def; }
}

// JSON 文字列から整数値（int）を読み取る。
// 例: src に '{ "count": 10 }' があり key = "count" なら 10 を返す。
int ReadInt(const std::string& src, const std::string& key, int def)
{
    std::string needle = "\"" + key + "\": ";
    auto pos = src.find(needle);
    if (pos == std::string::npos) { return def; }
    pos += needle.size();
    try { return std::stoi(src.substr(pos)); } catch (...) { return def; }
}

// JSON 文字列から文字列値（string）を読み取る。
// 例: src に '{ "name": "HP Bar" }' があり key = "name" なら "HP Bar" を返す。
std::string ReadString(const std::string& src, const std::string& key, const std::string& def)
{
    // 文字列値はコロンの後にスペースがない場合もあるので "key": で探す
    std::string needle = "\"" + key + "\":";
    auto pos = src.find(needle);
    if (pos == std::string::npos) { return def; }

    pos += needle.size();

    // コロンの直後にスペースやタブがあればスキップする
    while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t')) { ++pos; }

    // 値の先頭が '"' でなければ形式エラーなのでデフォルト値を返す
    if (pos >= src.size() || src[pos] != '"') { return def; }

    ++pos; // 開きダブルクォートの次の文字から
    auto end = src.find('"', pos); // 閉じダブルクォートを探す
    if (end == std::string::npos) { return def; }

    return src.substr(pos, end - pos); // 開き～閉じ の間を切り出して返す
}

}
