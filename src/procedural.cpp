#include "procedural.h"

#include <math.h>
#include <map>
#include <set>

#include <easylogging++.h>

obj_mesh apply(const obj_mesh& input, const float3x3& trans, const float3& t, bool flip_normals)
{
    obj_mesh res;
    res.indexes = input.indexes;
    res.uvs = input.uvs;
    res.positions = input.positions;
    res.normals = input.normals;
    res.tangents = input.tangents;

    if (flip_normals)
    {
        for (auto& i : res.indexes)
            std::swap(i.y, i.z);
    }

    for (auto& p : res.positions)
        p = mul(trans, p) + t;
    for (auto& p : res.normals)
        p = mul(trans, p);
    for (auto& p : res.tangents)
        p = mul(trans, p);

    return res;
}

int nearest(const obj_mesh& a, const std::vector<int>& subset, int i)
{
    float dist = std::numeric_limits<float>::max();
    int r = 0;
    auto q = a.positions[i];
    for (auto j : subset)
    {
        auto p = a.positions[j];
        auto d = distance(q, p);
        if (d < dist)
        {
            dist = d;
            r = j;
        }
    }
    return r;
}

obj_mesh fuse(const obj_mesh& a, const obj_mesh& b)
{
    obj_mesh res;
    res.indexes = a.indexes;
    res.uvs = a.uvs;
    res.positions = a.positions;
    res.normals = a.normals;
    res.tangents = a.tangents;

    for (auto& i : b.indexes)
        res.indexes.push_back({ i.x + (int)a.positions.size(),
                                i.y + (int)a.positions.size(),
                                i.z + (int)a.positions.size() });

    res.positions.insert(res.positions.end(), b.positions.begin(), b.positions.end());
    res.normals.insert(res.normals.end(), b.normals.begin(), b.normals.end());
    res.tangents.insert(res.tangents.end(), b.tangents.begin(), b.tangents.end());
    res.uvs.insert(res.uvs.end(), b.uvs.begin(), b.uvs.end());

    return res;
}

obj_mesh filter(const obj_mesh& a, std::function<bool(const float3&)> pred, std::vector<int>& edge)
{
    obj_mesh res;

    std::map<int, int> index_map;
    int idx = 0;
    for (int i = 0; i < a.positions.size(); i++)
    {
        if (pred(a.positions[i]))
        {
            index_map[i] = idx;
            res.positions.push_back(a.positions[i]);
            res.normals.push_back(a.normals[i]);
            res.uvs.push_back(a.uvs[i]);
            res.tangents.push_back(a.tangents[i]);
            idx++;
        }
    }
    for (auto& idx : a.indexes)
    {
        std::vector<int> good;
        if (index_map.find(idx.x) != index_map.end()) good.push_back(idx.x);
        if (index_map.find(idx.y) != index_map.end()) good.push_back(idx.y);
        if (index_map.find(idx.z) != index_map.end()) good.push_back(idx.z);
        if (good.size() < 3)
        {
            edge.insert(edge.end(), good.begin(), good.end());
            continue;
        }

        int3 nidx{
            index_map[idx.x],
            index_map[idx.y],
            index_map[idx.z]
        };

        res.indexes.push_back(nidx);
    }
    for (auto& e : edge)
        e = index_map[e];

    std::set<int> r;
    for (auto e : edge)
        r.insert(index_map[e]);
    edge.clear();
    for (auto e : r)
        edge.push_back(e);

    return res;
}

obj_mesh generate_tube_one_side(float length,
    float radius,
    float bend,
    int a, int b)
{
    obj_mesh res;

    float pi = 2 * acos(-1);

    auto toidx = [&](int i, int j) {
        return i * (b + 1) + j;
    };

    for (int i = 0; i <= a; i++)
    {
        auto ti = (float)i / a;
        for (int j = 0; j <= b; j++)
        {
            auto tj = (float)j / b;

            auto z = (length / 2) * ti + (-length / 2) * (1 - ti);

            float3 tube_center{ 0.f, 0.f, z };

            auto x = sinf(tj * pi) * radius;
            auto y = -cosf(tj * pi) * radius;

            auto curved_part = 0.6f;
            auto first_part = (1.f - curved_part) / 2.f;

            float2 xz{ x, 0 };
            if (!bend) xz.y = z;
            else
            {
                float2 tc{ 0.f, 0.f };

                if (ti <= first_part)
                {
                    auto K = (length - radius * 2.f) / 2.f;
                    xz.y = -xz.x - K;
                    tc.y = -K;
                    xz.x = (length / 2) * ti + (-length / 2) * (1 - ti) - K;
                    tc.x = (length / 2) * ti + (-length / 2) * (1 - ti) - K;

                    tube_center = { tc.x, 0, tc.y };
                }
                else if (ti <= (1.f - first_part))
                {
                    float t = (ti - first_part) / curved_part;
                    t = std::max(0.f, std::min(1.f, t));
                    float theta = bend * (1 - t) * pi / 4;
                    float2x2 R{
                        { cosf(theta), -sin(theta) },
                        { sinf(theta), cosf(theta) },
                    };
                    xz = mul(inverse(R), xz);
                    tc = mul(inverse(R), tc);

                    float C = length * (curved_part + first_part) - radius;
                    float2 translation{ cosf(theta) * C - C,
                        sinf(theta) * C + length / 2 - length * first_part };

                    xz += translation;
                    tc += translation;

                    tube_center = { tc.x, 0, tc.y };
                }
                else
                {
                    xz.y = (length / 2) * ti + (-length / 2) * (1 - ti);
                }
            }
            float3 point{ xz.x, y, xz.y };
            res.positions.push_back(point);

            float3 n = point - tube_center;

            res.normals.push_back(normalize(n));

            res.uvs.emplace_back(ti, tj * length / radius);

            if (i < a && j < b)
            {
                auto curr = toidx(i, j);
                auto next_a = toidx(i + 1, j);
                auto next_b = toidx(i, j + 1);
                auto next_ab = toidx(i + 1, j + 1);
                res.indexes.emplace_back(curr, next_b, next_a);
                res.indexes.emplace_back(next_a, next_b, next_ab);
            }
        }
    }

    res.calculate_tangents();

    return res;
}

obj_mesh generate_tube(float length, 
                       float radius,
                       float bend,
                       int a, int b)
{
    auto x = generate_tube_one_side(length, radius, bend, a, b);
    //auto y = generate_tube_one_side(length, radius * 0.85f, bend, a, b);

    //return fuse(x, y);
    return x;
}

obj_mesh make_grid(int a, int b, float x, float y)
{
    obj_mesh res;

    auto toidx = [&](int i, int j) {
        return i * (b + 1) + j;
    };

    for (auto i = 0; i <= a; i++)
    {
        for (auto j = 0; j <= b; j++)
        {
            float3 point{ (i * x) - (a * x) / 2.f,
                          0.f,
                          (j * y) - (b * y) / 2.f};
            res.positions.push_back(point);
            res.normals.push_back({ 0.f, 1.f, 0.f });

            res.uvs.emplace_back(0.f, 0.f);

            if (i < a && j < b)
            {
                auto curr = toidx(i, j);
                auto next_a = toidx(i + 1, j);
                auto next_b = toidx(i, j + 1);
                auto next_ab = toidx(i + 1, j + 1);
                res.indexes.emplace_back(curr, next_b, next_a);
                res.indexes.emplace_back(next_a, next_b, next_ab);
            }
        }
    }

    res.calculate_tangents();

    return res;
}

obj_mesh generate_tube2(float length,
    float radius,
    float bend,
    int a, int b)
{
    obj_mesh res;

    float pi = 2 * acos(-1);

    auto toidx = [&](int i, int j) {
        return i * (b + 1) + j;
    };

    
    for (int j = 0; j <= b; j++)
    {
        auto tj = (float)j / b;

        for (int i = 0; i <= a; i++)
        {
            auto ti = (float)i / a;

            auto z = (length / 2) * ti + (-length / 2) * (1 - ti);

            float3 tube_center{ 0.f, 0.f, z };

            auto x = -sinf(tj * pi / 4.f) * radius;
            auto y = cosf(tj * pi / 4.f) * radius;

            auto curved_part = (1.f - y) * 0.5f;

            //LOG(INFO) << curved_part;

            auto first_part = (1.f - curved_part) / 2.f;

            float2 xz{ x, 0 };
            if (!bend) xz.y = z;
            else
            {
                float2 tc{ 0.f, 0.f };

                auto K = (length / 2.f - radius);

                if (ti <= first_part)
                {
                    xz.y = -xz.x - K;
                    tc.y = -K;
                    xz.x = (length / 2) * ti + (-length / 2) * (1 - ti) - K;
                    tc.x = (length / 2) * ti + (-length / 2) * (1 - ti) - K;

                    tube_center = { tc.x, 0, tc.y };
                }
                else if (ti <= (1.f - first_part))
                {
                    float t = (ti - first_part) / curved_part;
                    t = std::max(0.f, std::min(1.f, t));
                    float theta = bend * (1 - t) * pi / 4;
                    float2x2 R{
                        { cosf(theta), -sin(theta) },
                        { sinf(theta), cosf(theta) },
                    };
                    xz = mul(inverse(R), xz);
                    tc = mul(inverse(R), tc);

                    float C = length * (curved_part + first_part) - radius;
                    float2 translation{ cosf(theta) * C - C,
                        sinf(theta) * C + length / 2 - length * first_part };

                    xz += translation;
                    tc += translation;

                    tube_center = { tc.x, 0, tc.y };
                }
                else
                {
                    xz.y = (length / 2) * ti + (-length / 2) * (1 - ti);
                }

                if (fabsf(ti - 0.5f) + fabsf(curved_part) < 0.01f)
                {
                    xz.x = 0.f;
                    xz.y = -K;
                }
            }
            float3 point{ xz.x, y, xz.y };
            res.positions.push_back(point);

            float3 n = point - tube_center;

            res.normals.push_back(normalize(n));

            res.uvs.emplace_back(ti, (tj + 0.66f) * length / (4 * radius));

            if (i < a && j < b)
            {
                auto curr = toidx(i, j);
                auto next_a = toidx(i + 1, j);
                auto next_b = toidx(i, j + 1);
                auto next_ab = toidx(i + 1, j + 1);
                res.indexes.emplace_back(curr, next_a, next_b);
                res.indexes.emplace_back(next_a, next_ab, next_b);
            }
        }
    }

    res.calculate_tangents();

    return res;
}

obj_mesh generate_tube3(float length,
    float radius,
    int a, int b)
{
    auto x = generate_tube2(length, radius, -1.f, a / 4, b / 4);

    float3x3 flipY{
        { 1.f, 0.f, 0.f },
        { 0.f, -1.f, 0.f },
        { 0.f, 0.f, 1.f }
    };

    auto y = apply(x, flipY, { 0.f, 0.f, 0.f }, true);

    x = fuse(x, y);

    float3x3 flipZ{
        { 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 0.f, -1.f }
    };

    y = apply(x, flipZ, { 0.f, 0.f, -radius }, true);

    x = fuse(x, y);

    auto z = generate_tube2(length * 4.f / 3.f, radius, 0.f, a / 4, b / 4);

    float3x3 flipX{
        { -1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 0.f, 1.f }
    };

    z = apply(z, flipX, { 0.f, 0.f, -0.5f }, true);
    y = apply(z, flipY, { 0.f, 0.f, 0.f }, true);
    z = fuse(z, y);
    x = fuse(x, z);

    return x;
}

obj_mesh generate_tube4(float length,
    float radius,
    float bend,
    int a, int b)
{
    auto x = generate_tube2(length, radius, bend, a / 4, b / 4);

    float3x3 flipY{
        { 1.f, 0.f, 0.f },
        { 0.f, -1.f, 0.f },
        { 0.f, 0.f, 1.f }
    };

    auto y = apply(x, flipY, { 0.f, 0.f, 0.f }, true);

    x = fuse(x, y);

    float3x3 flipZ{
        { 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 0.f, -1.f }
    };

    y = apply(x, flipZ, { 0.f, 0.f, -radius }, true);

    x = fuse(x, y);

    float3x3 flipX{
        { -1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 0.f, 1.f }
    };

    y = apply(x, flipX, { 0.f, 0.f, 0.f }, true);

    x = fuse(x, y);

    return x;
}