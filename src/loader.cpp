#include "loader.h"
#include <OBJ_Loader.h>

void loader::thread_proc(std::string name)
{
    objl::Loader loader;
    bool loadout = loader.LoadFile(name,
        [&](float p) { _progress = p; });

    obj_file result;

    for (int i = 0; i < loader.LoadedMeshes.size(); i++)
    {
        objl::Mesh curMesh = loader.LoadedMeshes[i];

        std::vector<float3> positions;
        for (auto&& v : curMesh.Vertices)
        {
            positions.emplace_back(v.Position.X, v.Position.Y, v.Position.Z);

            auto length = v.Position.X * v.Position.X +
                v.Position.Y * v.Position.Y +
                v.Position.Z * v.Position.Z;
        }

        std::vector<float3> normals;
        for (auto&& v : curMesh.Vertices)
            normals.emplace_back(v.Normal.X, v.Normal.Y, v.Normal.Z);
        std::vector<float2> uvs;
        for (auto&& v : curMesh.Vertices)
            uvs.emplace_back(v.TextureCoordinate.X, v.TextureCoordinate.Y);
        std::vector<int3> idx;
        for (int i = 0; i < curMesh.Indices.size(); i += 3)
            idx.emplace_back(curMesh.Indices[i], curMesh.Indices[i + 1], curMesh.Indices[i + 2]);
        

        obj_mesh r;
        r.name = curMesh.MeshName;
        r.indexes = idx;
        r.positions = positions;
        r.normals = normals;
        r.uvs = uvs;
        r.calculate_tangents();
        result.push_back(r);
    }

    _queue.enqueue(std::move(result));
}

void obj_mesh::calculate_tangents()
{
    std::vector<float3> tangents(positions.size(), { 0.f,0.f,0.f });

    auto calc_tangent = [](const float3& v0,
                           const float3& v1,
                           const float3& v2,
                           const float2& uv0,
                           const float2& uv1,
                           const float2& uv2
        ) {
        float3 p0{ v0.x, v0.y,v0.z };
        float3 p1{ v1.x, v1.y,v1.z };
        float3 p2{ v2.x, v2.y,v2.z };

        float2 t0{ uv0.x, uv0.y };
        float2 t1{ uv1.x, uv1.y };
        float2 t2{ uv2.x, uv2.y };

        auto deltaPos1 = p1 - p0;
        auto deltaPos2 = p2 - p0;

        auto deltaUv1 = t1 - t0;
        auto deltaUv2 = t2 - t0;

        auto r = 1 / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);
        auto tangent = (deltaPos1 * deltaUv2.y - deltaPos2 * deltaUv1.y) * r;
        return tangent;
    };

    for (int i = 0; i < indexes.size(); i++)
    {
        auto v0 = positions[indexes[i].x];
        auto v1 = positions[indexes[i].y];
        auto v2 = positions[indexes[i].z];

        auto uv0 = uvs[indexes[i].x];
        auto uv1 = uvs[indexes[i].y];
        auto uv2 = uvs[indexes[i].z];

        tangents[indexes[i].x] = calc_tangent(v0, v1, v2, uv0, uv1, uv2);
        tangents[indexes[i].y] = calc_tangent(v1, v2, v0, uv1, uv2, uv0);
        tangents[indexes[i].z] = calc_tangent(v2, v0, v1, uv2, uv0, uv1);
    }
    this->tangents = tangents;
}


loader::~loader()
{
    _thread.join();
}

std::vector<obj_mesh>& loader::get() 
{
    if (!ready()) throw std::runtime_error("Results are not ready!");
    return _result;
}

bool loader::ready() const
{
    if (!_ready)
    {
        if (_queue.try_dequeue(_result))
        {
            _ready = true;
        }
    }
    return _ready;
}

loader::loader(std::string filename)
    : _thread([this, filename]() { thread_proc(filename); }), 
      _queue(1), _result()
{
}