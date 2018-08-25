#include "procedural.h"

#include <math.h>

obj_mesh generate_tube(float length, 
                       float radius,
                       float bend,
                       int a, int b)
{
    obj_mesh res;

    float pi = 2 * acos(-1);
    int start = 0;

    auto toidx = [&](int i, int j) {
        return i * (b + 1) + j + start;
    };

    auto generate = [&](float r) {
        for (int i = 0; i <= a; i++)
        {
            auto ti = (float)i / a;
            for (int j = 0; j <= b; j++)
            {
                auto tj = (float)j / b;

                auto z = (length / 2) * ti + (-length / 2) * (1 - ti);

                float3 tube_center{ 0.f, 0.f, z };

                auto x = sinf(tj * pi) * r;
                auto y = -cosf(tj * pi) * r;

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
    };

    generate(radius);
    start = res.positions.size();
    generate(0.85f * radius);

    res.calculate_tangents();

    return res;
}