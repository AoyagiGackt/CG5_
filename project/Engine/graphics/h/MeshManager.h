/**
 * @file MeshManager.h
 * @brief 3D描画に使用する基本図形（メッシュ）の頂点データやトランスフォームを一元管理するファイル
 */
#pragma once
#include "MakeAffine.h"
#include <vector>

/**
 * @brief エンジンに組み込み済みの基本図形の種類
 */
enum MeshType {
    MeshType_Sphere, /// 球体
    MeshType_Cube, /// 立方体
    MeshType_Plane, /// 平面
    MeshType_Count /// メッシュの種類の総数
};


/**
 * @brief 3Dモデルを構成する1つの頂点が持つデータ
 */
struct VertexData {
    
    /** @brief 頂点のローカル座標（X, Y, Z, W） */
    Vector4 position;
    
    /** @brief テクスチャのUV座標（U, V） */
    Vector2 texcoord;
    
    /** @brief 頂点法線ベクトル（ライティングの陰影計算に使用） */
    Vector3 normal;
};

/**
 * @brief 1つのメッシュ（3D形状）の全データを保持する構造体
 */
struct MeshData {
    
    /** @brief メッシュを構成する全頂点のリスト */
    std::vector<VertexData> vertices;

    /** @brief メッシュ固有のトランスフォーム（座標・回転・スケール） */
    Transform transform;
};

/**
 * @brief 複数のメッシュデータを保持し、描画に使用するメッシュを切り替えるシングルトンクラス
 */
class MeshManager {
public:

    /**
     * @brief コンストラクタ。内部で基本的なメッシュ（球、立方体、平面など）を生成・初期化する
     */
    MeshManager();

    /**
     * @brief 現在選択されているメッシュのデータを取得する
     * @return MeshData& 現在のメッシュデータへの参照
     */
    MeshData& GetCurrentMesh();

    /**
     * @brief 描画に使用するメッシュの種類を変更する
     * @param type 変更先のメッシュタイプ（MeshType 列挙型の値）
     */
    void SetCurrentMeshType(MeshType type);

    /**
     * @brief 現在選択されているメッシュの種類を取得する
     * @return MeshType 現在のメッシュタイプ
     */
    MeshType GetCurrentMeshType() const;
    
    /** @brief マネージャーが保持している全メッシュのリスト（インデックスは MeshType に対応） */
    std::vector<MeshData> meshes;
    
    /**
     * @brief MeshManagerの唯一のインスタンスを取得する
     * @return MeshManager* シングルトンインスタンスへのポインタ
     */
    static MeshManager* GetInstance();

    /**
     * @brief マネージャーの終了処理を行う（リストのクリアなど）
     */
    void Finalize();

private:

    /** @brief 現在選択されているメッシュのタイプ */
    MeshType currentMeshType_;

    /**
     * @brief 初期化時に各基本図形（球体、立方体、平面）の頂点データを計算してリストに登録する
     */
    void InitMeshes();
};