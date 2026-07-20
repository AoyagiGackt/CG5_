#include "MeshManager.h"
#include <cmath>

namespace {

MeshData GenerateSphereMesh(int subdivision = 16, float radius = 1.0f)
{
    MeshData mesh;
    mesh.transform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    const float kPi = 3.14159265358979323846f;
    const float kTwoPi = kPi * 2.0f;

    for (int lat = 0; lat < subdivision; ++lat) {
        float lat0 = kPi * (float(lat) / subdivision - 0.5f);
        float lat1 = kPi * (float(lat + 1) / subdivision - 0.5f);
        
        for (int lon = 0; lon < subdivision; ++lon) {
            float lon0 = kTwoPi * float(lon) / subdivision;
            float lon1 = kTwoPi * float(lon + 1) / subdivision;

            Vector4 p00 = { radius * cosf(lat0) * cosf(lon0), radius * sinf(lat0), radius * cosf(lat0) * sinf(lon0), 1.0f };
            Vector4 p01 = { radius * cosf(lat0) * cosf(lon1), radius * sinf(lat0), radius * cosf(lat0) * sinf(lon1), 1.0f };
            Vector4 p10 = { radius * cosf(lat1) * cosf(lon0), radius * sinf(lat1), radius * cosf(lat1) * sinf(lon0), 1.0f };
            Vector4 p11 = { radius * cosf(lat1) * cosf(lon1), radius * sinf(lat1), radius * cosf(lat1) * sinf(lon1), 1.0f };

            Vector2 uv00 = { float(lon) / subdivision, 1.0f - float(lat) / subdivision };
            Vector2 uv01 = { float(lon + 1) / subdivision, 1.0f - float(lat) / subdivision };
            Vector2 uv10 = { float(lon) / subdivision, 1.0f - float(lat + 1) / subdivision };
            Vector2 uv11 = { float(lon + 1) / subdivision, 1.0f - float(lat + 1) / subdivision };

            mesh.vertices.push_back({ p00, uv00, { p00.x, p00.y, p00.z } });
            mesh.vertices.push_back({ p10, uv10, { p10.x, p10.y, p10.z } });
            mesh.vertices.push_back({ p11, uv11, { p11.x, p11.y, p11.z } });

            mesh.vertices.push_back({ p00, uv00, { p00.x, p00.y, p00.z } });
            mesh.vertices.push_back({ p11, uv11, { p11.x, p11.y, p11.z } });
            mesh.vertices.push_back({ p01, uv01, { p01.x, p01.y, p01.z } });
        }
    }
    return mesh;
}

MeshData GenerateCubeMesh(float size = 1.0f)
{
    MeshData mesh;
    mesh.transform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    const float s = size * 0.5f;

    struct Face {
        Vector4 p0, p1, p2, p3;
        Vector2 uv0, uv1, uv2, uv3;
        Vector3 normal;
    };

    Face faces[6] = {
        // +X
        { { s, -s, -s, 1 }, { s, -s, s, 1 }, { s, s, s, 1 }, { s, s, -s, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { 1, 0, 0 } },
        // -X
        { { -s, -s, s, 1 }, { -s, -s, -s, 1 }, { -s, s, -s, 1 }, { -s, s, s, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 0, 0 } },
        // +Y
        { { -s, s, s, 1 }, { s, s, s, 1 }, { s, s, -s, 1 }, { -s, s, -s, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 1, 0 } },
        // -Y
        { { -s, -s, -s, 1 }, { s, -s, -s, 1 }, { s, -s, s, 1 }, { -s, -s, s, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, -1, 0 } },
        // +Z
        { { -s, -s, s, 1 }, { s, -s, s, 1 }, { s, s, s, 1 }, { -s, s, s, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0, 1 } },
        // -Z
        { { s, -s, -s, 1 }, { -s, -s, -s, 1 }, { -s, s, -s, 1 }, { s, s, -s, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0, -1 } },
    };

    for (const auto& f : faces) {
        mesh.vertices.push_back({ f.p0, f.uv0, f.normal });
        mesh.vertices.push_back({ f.p1, f.uv1, f.normal });
        mesh.vertices.push_back({ f.p2, f.uv2, f.normal });

        mesh.vertices.push_back({ f.p0, f.uv0, f.normal });
        mesh.vertices.push_back({ f.p2, f.uv2, f.normal });
        mesh.vertices.push_back({ f.p3, f.uv3, f.normal });
    }

    return mesh;
}

MeshData GeneratePlaneMesh(float width = 1.0f, float height = 1.0f)
{
    MeshData mesh;
    mesh.transform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float w = width * 0.5f;
    float h = height * 0.5f;
    Vector4 p0 = { -w, 0, -h, 1.0f };
    Vector4 p1 = { w, 0, -h, 1.0f };
    Vector4 p2 = { w, 0, h, 1.0f };
    Vector4 p3 = { -w, 0, h, 1.0f };
    Vector2 uv0 = { 0, 0 };
    Vector2 uv1 = { 1, 0 };
    Vector2 uv2 = { 1, 1 };
    Vector2 uv3 = { 0, 1 };
    Vector3 normal = { 0, 1, 0 };

    mesh.vertices.push_back({ p0, uv0, normal });
    mesh.vertices.push_back({ p1, uv1, normal });
    mesh.vertices.push_back({ p2, uv2, normal });

    mesh.vertices.push_back({ p0, uv0, normal });
    mesh.vertices.push_back({ p2, uv2, normal });
    mesh.vertices.push_back({ p3, uv3, normal });

    return mesh;
}
}

MeshManager::MeshManager()
    : currentMeshType_(MeshType_Sphere)
{
    InitMeshes();
}

void MeshManager::SetCurrentMeshType(MeshType type) { currentMeshType_ = type; }
MeshType MeshManager::GetCurrentMeshType() const { return currentMeshType_; }
MeshData& MeshManager::GetCurrentMesh() { return meshes[(int)currentMeshType_]; }

void MeshManager::InitMeshes()
{
    meshes.clear();
    meshes.push_back(GenerateSphereMesh());
    meshes.push_back(GenerateCubeMesh());
    meshes.push_back(GeneratePlaneMesh());
}

MeshManager* MeshManager::GetInstance()
{
    static MeshManager instance;
    return &instance;
}

void MeshManager::Finalize()
{
    meshes.clear();
}