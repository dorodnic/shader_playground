#include "procedural.h"

#include <math.h>
#include <map>
#include <set>

#include <easylogging++.h>

#include "VoronoiDiagramGenerator.h"

#include <random>

bool sitesOrdered(const Point2& s1, const Point2& s2) {
    if (s1.y < s2.y)
        return true;
    if (s1.y == s2.y && s1.x < s2.x)
        return true;

    return false;
}

float dist(float x)
{
    if (x < 0.5f) return fabsf(x);
    else return fabsf(1.f - x);
}

float dist(const Point2& p)
{
    return std::min(dist(p[0]), dist(p[1]));
}

std::default_random_engine generator;

void generate_broken_glass(
    std::vector<glass_peice>& peices)
{
    obj_mesh res;

    //unsigned int dimension = 1000000;
    int numSites = rand() % 100 + 20;

    BoundingBox bbox(0, 1, 1, 0);

    std::vector<Point2> tmpSites, sites;

    tmpSites.reserve(numSites);
    sites.reserve(numSites);

    Point2 s;

    std::normal_distribution<float> distribution(0.5f, 0.15f);
    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);

    for (unsigned int i = 0; i < numSites; ++i) {
        s.x = distribution(generator);
        s.y = distribution(generator);
        tmpSites.push_back(s);
    }

    //remove any duplicates that exist
    std::sort(tmpSites.begin(), tmpSites.end(), sitesOrdered);
    sites.push_back(tmpSites[0]);
    for (Point2& s : tmpSites) {
        if (s != sites.back()) sites.push_back(s);
    }

    VoronoiDiagramGenerator gen;
    auto diagram = gen.compute(sites, bbox);

    for (Cell* c : diagram->cells) 
    {
        Point2* first = nullptr;
        float3 pos;
        int k = 0;

        float min_dist = 1.f;

        for (HalfEdge* e : c->halfEdges)
        {
            if (e->startPoint() && e->endPoint()) {
                if (first == nullptr)
                {
                    first = e->startPoint();
                }
                else
                {
                    Point2& p1 = *e->startPoint();
                    Point2& p2 = *e->endPoint();

                    auto dist_from_edge = std::min(dist(p1), dist(p2));
                    if (dist_from_edge < min_dist)
                        min_dist = dist_from_edge;
                }

                Point2& p3 = *e->startPoint();
                pos += float3{ (float)p3[0], (float)-p3[1], dist(p3[1]) * 0.3f };
                k++;
            }
        }

        pos = pos * (1.f / k);

        for (HalfEdge* e : c->halfEdges)
        {
            if (e->startPoint() && e->endPoint()) {
                Point2& p1 = *e->startPoint();
                Point2& p2 = *e->endPoint();
                Point2& p3 = *first;

                float3 a{ (float)p1[0], (float)-p1[1], dist(p1[1]) * 0.3f + 0.04f };
                float3 b{ (float)p2[0], (float)-p2[1], dist(p2[1]) * 0.3f + 0.04f };
                float3 c{ (float)p3[0], (float)-p3[1], dist(p3[1]) * 0.3f + 0.04f };

                float3 a0{ (float)p1[0], (float)-p1[1], dist(p1[1]) * 0.3f + 0.f };
                float3 b0{ (float)p2[0], (float)-p2[1], dist(p2[1]) * 0.3f + 0.f };
                float3 c0{ (float)p3[0], (float)-p3[1], dist(p3[1]) * 0.3f + 0.f };

                int idx = res.positions.size();

                if (min_dist > 0.001f)
                {
                    res.positions.push_back(a - pos);
                    res.positions.push_back(b - pos);
                    res.positions.push_back(c - pos);

                    res.indexes.emplace_back(idx, idx + 1, idx + 2);

                    res.positions.push_back(a0 - pos);
                    res.positions.push_back(b0 - pos);
                    res.positions.push_back(c0 - pos);

                    res.indexes.emplace_back(idx + 3, idx + 5, idx + 4);

                    res.positions.push_back(a - pos);
                    res.positions.push_back(b - pos);
                    res.positions.push_back(c - pos);
                    res.positions.push_back(a0 - pos);
                    res.positions.push_back(b0 - pos);
                    res.positions.push_back(c0 - pos);

                    res.indexes.emplace_back(idx + 6, idx + 6 + 3, idx + 6 + 1);
                    res.indexes.emplace_back(idx + 6 + 1, idx + 6 + 3, idx + 6 + 4);

                    res.normals.push_back(a - float3{ 0.f, 0.f, -3.f });
                    res.normals.push_back(b - float3{ 0.f, 0.f, -3.f });
                    res.normals.push_back(c - float3{ 0.f, 0.f, -3.f });

                    res.normals.push_back(float3{ 0.f, 0.f, -3.f } - a);
                    res.normals.push_back(float3{ 0.f, 0.f, -3.f } - b);
                    res.normals.push_back(float3{ 0.f, 0.f, -3.f } - c);

                    auto norm = a - float3{ 0.f, 0.f, -3.f };
                    auto edge = b - a;
                    auto side_norm = cross(norm, edge);

                    res.normals.push_back(side_norm);
                    res.normals.push_back(side_norm);
                    res.normals.push_back(side_norm);
                    res.normals.push_back(side_norm);
                    res.normals.push_back(side_norm);
                    res.normals.push_back(side_norm);

                    for (int i = 0; i < 4; i++)
                    {
                        res.uvs.push_back({ a.x / 2 + 0.2f, a.y / 2 });
                        res.uvs.push_back({ b.x / 2 + 0.2f, b.y / 2 });
                        res.uvs.push_back({ c.x / 2 + 0.2f, c.y / 2 });
                    }
                }
                else
                {
                    /*res.positions.push_back(a - pos);
                    res.positions.push_back(b - pos);
                    res.positions.push_back(c - pos);

                    res.positions.push_back((a + b) / 2.f - pos);
                    res.positions.push_back((a + c) / 2.f - pos);
                    res.positions.push_back((b + c) / 2.f - pos);

                    res.indexes.emplace_back(idx, idx + 3, idx + 4);
                    res.indexes.emplace_back(idx + 4, idx + 3, idx + 5);
                    res.indexes.emplace_back(idx + 5, idx + 2, idx + 4);
                    res.indexes.emplace_back(idx + 5, idx + 2, idx + 4);
                    res.indexes.emplace_back(idx + 1, idx + 5, idx + 3);

                    res.normals.push_back({ 0.f, 0.f, 1.f });
                    res.normals.push_back({ 0.f, 0.f, 1.f });
                    res.normals.push_back({ 0.f, 0.f, 1.f });
                    res.normals.push_back({ 0.f, 0.f, 1.f });
                    res.normals.push_back({ 0.f, 0.f, 1.f });
                    res.normals.push_back({ 0.f, 0.f, 1.f });

                    res.uvs.push_back({ a.x, a.y / 2 });
                    res.uvs.push_back({ b.x, b.y / 2 });
                    res.uvs.push_back({ c.x, c.y / 2 });

                    res.uvs.push_back({ (a.x + b.x) / 2, (a.y + b.y) / 4 });
                    res.uvs.push_back({ (a.x + c.x) / 2, (a.y + c.y) / 4 });
                    res.uvs.push_back({ (b.x + c.x) / 2, (b.y + c.y) / 4 });*/
                }
            }
        }

        if (res.positions.size() == 0) continue;

        res.calculate_tangents();

        float3 rotation{
            uniform(generator),
            uniform(generator),
            uniform(generator)
        };
        rotation = normalize(rotation);

        peices.push_back({ res, min_dist, rotation, pos });

        res = obj_mesh();
    }
}

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

float filter(float t, float a)
{
    auto part = 0.2f;
    if (t < part)
    {
        return t * a;
    }
    else if (t > 1 - part)
    {
        return t * a - a + 1.f;
    }
    else
    {
        auto y1 = part * a;
        auto y2 = (1 - part) * a - a + 1.f;
        auto t1 = (t - part) / (1.f - 2 * part);
        return t1 * y2 + (1 - t1) * y1;
    }
}

obj_mesh generate_tube_new(float length, float radius, float q)
{
    obj_mesh res;

    auto c1 = std::make_shared<arc_curve>(float3{ 0.f, radius, 0.f },
                                          float3{ radius, 0.f, 0.f },
                                          float3{ 0.f, 0.f, 0.f }
                                          );
    auto c2 = std::make_shared<arc_curve>(float3{ radius, 0.f, 0.f },
                                          float3{ 0.f, -radius, 0.f }, 
                                          float3{ 0.f, 0.f, 0.f });
    composite_curve c;
    c.add_curve(c1);
    c.add_curve(c2);
    int a = c.get_k(q);

    linear_curve l(float3{ 0.f, 0.f, -length / 2.f},
                   float3{ 0.f, 0.f, length / 2.f});
    int b = l.get_k(q);

    auto toidx = [&](int i, int j) {
        return j * (a + 1) + i;
    };

    for (int j = 0; j <= b; j++)
    {
        for (int i = 0; i <= a; i++)
        {
            auto tube_center = l.eval(j, q);
            auto point = tube_center + c.eval(i, q);

            res.positions.push_back(point);

            float3 n = point - tube_center;

            res.normals.push_back(normalize(n));

            auto u = filter((float)j / b, length / (3 * radius));
            auto v = 0.5f + (float)i / (a * 0.66f);

            res.uvs.emplace_back(u, v);

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

    auto z = generate_tube_new(length * 4.f / 3.f, radius, 1.f);

    float3x3 id{
        { 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 0.f, 1.f }
    };

    z = apply(z, id, { 0.f, 0.f, -0.5f }, false);

    return fuse(x, z);
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

obj_mesh generate_cap(float length, float radius, float q)
{
    obj_mesh res;

    float pi = 2 * acos(-1);

    auto c1 = std::make_shared<arc_curve>(float3{ 0.f, radius, 0.f },
        float3{ radius, 0.f, 0.f },
        float3{ 0.f, 0.f, 0.f });
    auto c2 = std::make_shared<arc_curve>(float3{ radius, 0.f, 0.f },
        float3{ 0.f, -radius, 0.f },
        float3{ 0.f, 0.f, 0.f });
    auto c3 = std::make_shared<arc_curve>(float3{ 0.f, -radius, 0.f },
        float3{ -radius, 0.f, 0.f },
        float3{ 0.f, 0.f, 0.f });
    auto c4 = std::make_shared<arc_curve>(float3{ -radius, 0.f, 0.f },
        float3{ 0.f, radius, 0.f },
        float3{ 0.f, 0.f, 0.f });
    composite_curve c;
    c.add_curve(c1);
    c.add_curve(c2);
    c.add_curve(c3);
    c.add_curve(c4);
    int a = c.get_k(q);

    linear_curve l(float3{ 0.f, 0.f, -length / 2.f },
        float3{ 0.f, 0.f, length / 2.f });
    int b = l.get_k(q);

    auto toidx = [&](int i, int j) {
        return j * (a + 1) + i;
    };

    for (int j = 0; j <= b; j++)
    {
        for (int i = 0; i <= a; i++)
        {
            auto t = (float)j / b;
            auto tube_center = l.eval(j, q);
            auto p = c.eval(i, q);
            auto s = std::cosf(std::asinf(t));
            auto point = tube_center + p * float3(s, s, 0.f);

            res.positions.push_back(point);

            res.normals.push_back(point);

            res.uvs.emplace_back(t * 0.33f, 
                                 0.5f + (float)i / a * 3);

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