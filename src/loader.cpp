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
        std::vector<float3> tangents(positions.size(), { 0.f,0.f,0.f });

        auto calc_tangent = [](const objl::Vertex& v0,
            const objl::Vertex& v1,
            const objl::Vertex& v2) {
            float3 p0{ v0.Position.X, v0.Position.Y,v0.Position.Z };
            float3 p1{ v1.Position.X, v1.Position.Y,v1.Position.Z };
            float3 p2{ v2.Position.X, v2.Position.Y,v2.Position.Z };

            float2 t0{ v0.TextureCoordinate.X, v0.TextureCoordinate.Y };
            float2 t1{ v1.TextureCoordinate.X, v1.TextureCoordinate.Y };
            float2 t2{ v2.TextureCoordinate.X, v2.TextureCoordinate.Y };

            auto deltaPos1 = p1 - p0;
            auto deltaPos2 = p2 - p0;

            auto deltaUv1 = t1 - t0;
            auto deltaUv2 = t2 - t0;

            auto r = 1 / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);
            auto tangent = (deltaPos1 * deltaUv2.y - deltaPos2 * deltaUv1.y) * r;
            return tangent;
        };

        for (int i = 0; i < curMesh.Indices.size(); i += 3)
        {
            auto v0 = curMesh.Vertices[curMesh.Indices[i]];
            auto v1 = curMesh.Vertices[curMesh.Indices[i + 1]];
            auto v2 = curMesh.Vertices[curMesh.Indices[i + 2]];

            tangents[curMesh.Indices[i]] = calc_tangent(v0, v1, v2);
            tangents[curMesh.Indices[i + 1]] = calc_tangent(v1, v2, v0);
            tangents[curMesh.Indices[i + 2]] = calc_tangent(v2, v0, v1);
        }

        obj_mesh r;
        r.name = curMesh.MeshName;
        r.indexes = idx;
        r.positions = positions;
        r.normals = normals;
        r.uvs = uvs;
        r.tangents = tangents;
        result.push_back(r);
    }

    _queue.enqueue(std::move(result));
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