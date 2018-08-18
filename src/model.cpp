#include "model.h"

void model::create(obj_file& loader)
{
    for (auto& mesh : loader)
    {
        if (mesh.name == id || id == "*")
        {
            id = mesh.name;
            //if (curMesh.MeshMaterial.map_Kd != "")
            //{
            //    auto dir = get_directory(loader.Path);
            //    auto diffuse_path = curMesh.MeshMaterial.map_Kd;
            //    if (dir != "") diffuse_path = dir + "/" + diffuse_path;

            //    diffuse.upload(diffuse_path);
            //}

            for (auto& p : mesh.positions)
            {
                auto length = p.x * p.x + p.y * p.y + p.z * p.z;
                _max = std::max(length, _max);
            }
            _max = sqrt(_max);
            for (auto& p : mesh.positions)
            {
                p.x /= _max;
                p.y /= _max;
                p.z /= _max;
            }

            _geometry = std::make_shared<vao>(mesh.positions.data(),
                mesh.uvs.data(),
                mesh.normals.data(), 
                mesh.tangents.data(),
                mesh.positions.size(), 
                mesh.indexes.data(), 
                mesh.indexes.size());
        }
    }
}

void model::render()
{
    if (!_geometry) return;
    if (visible) _geometry->draw();
}
