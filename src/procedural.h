#pragma once

#include "util.h"
#include "loader.h"

obj_mesh apply(const obj_mesh& input, const float3x3& trans, const float3& t = { 0.f, 0.f, 0.f }, bool flip_normals = false);
obj_mesh fuse(const obj_mesh& a, const obj_mesh& b);
obj_mesh filter(const obj_mesh& a, std::function<bool(const float3&)> pred, std::vector<int>& edge);
int nearest(const obj_mesh& a, const std::vector<int>& subset, int i);

obj_mesh make_grid(int a, int b, float x, float y);

obj_mesh generate_tube(float length,
    float radius, float bend, int a, int b);
obj_mesh generate_tube3(float length,
    float radius, int a, int b);
obj_mesh generate_tube4(float length,
    float radius, float bend, int a, int b);

obj_mesh generate_tube_new(float length, float radius, float q);

obj_mesh generate_cap(float length, float radius, float q);

class curve
{
public:
    virtual float3 eval(int i, float q) const = 0;
    virtual int get_k(float q) const = 0;
};

class composite_curve : public curve
{
public:
    float3 eval(int i, float q) const override
    {
        if (!_curves.size()) throw;
        int k = get_k(q); // Todo: loopup in a sorted array
        int j = 0;
        auto cr = _curves.front();
        for (auto c : _curves)
        {
            int ck = c->get_k(q);
            if (j + ck < i)
                j += ck;
            else
            {
                cr = c;
                break;
            }
        }
        int ci = i - j;
        return cr->eval(ci, q);
    }
    void add_curve(std::shared_ptr<curve> c) { _curves.push_back(c); }
    int get_k(float q) const override
    {
        int total = 0;
        for (auto& c : _curves)
            total += c->get_k(q);
        return total;
    }

private:
    std::vector<std::shared_ptr<curve>> _curves;
};

class linear_curve : public curve
{
public:
    linear_curve(const float3& a,
                 const float3& b)
        : a(a), b(b) {}

    float3 a, b;

    float3 eval(int i, float q) const override
    {
        auto t = (float)i / get_k(q);
        return t * b + (1 - t) * a;
    }

    int get_k(float q) const override { return 2 + 6 * q; }
};

class quadratic_curve : public curve
{
public:
    quadratic_curve(const float3& a,
                    const float3& b, 
                    const float3& c)
        : a(a), b(b), c(c) {}

    float3 a, b, c;

    float3 eval(int i, float q) const override
    {
        auto k = get_k(q);
        auto t = (float)i / k;
        auto x = lerp(a, b, t);
        auto y = lerp(b, c, t);
        return lerp(x, y, t);
    }

    int get_k(float q) const override {
        return (int)(3 + q * 29);
    }
};

class cubic_curve : public curve
{
public:
    float3 a, b, c, d;

    cubic_curve(const float3& a,
                const float3& b,
                const float3& c,
                const float3& d)
        : a(a), b(b), c(c), d(d) {}

    cubic_curve(const cubic_curve& prev, 
                const float3& c,
                const float3& d,
                float mag = 1.f)
        : c(c), d(d)
    {
        a = prev.d;
        auto v = prev.d - prev.c;
        b = a + v;
    }

    float3 eval(int i, float q) const override
    {
        auto k = get_k(q);
        auto t = (float)i / k;
        auto x1 = lerp(a, b, t);
        auto y1 = lerp(b, c, t);
        auto q1 = lerp(x1, y1, t);
        auto x2 = lerp(b, c, t);
        auto y2 = lerp(c, d, t);
        auto q2 = lerp(x2, y2, t);
        return lerp(q1, q2, t);
    }

    int get_k(float q) const override {
        return (int)(4 + q * 28);
    }
};

class arc_curve : public curve
{
public:
    float3 a, b, c;

    arc_curve(const float3& a,
        const float3& b,
        const float3& c)
        : a(a), b(b), c(c) {}

    float3 eval(int i, float q) const override
    {
        auto k = get_k(q);
        auto t = (float)i / k;
        
        auto ca = a - c;
        auto cb = b - c;
        auto angle = std::acosf(dot(ca, cb) / (length(ca) * length(cb)));
        auto proj_b = (dot(ca, cb) / length(cb)) * ca;
        auto n = cb - proj_b;
        auto cn = normalize(n) * length(ca);
        auto cr = cross(cn, ca);
        float3x3 M{
            ca, cn, cr
        };
        auto x = std::cosf(t * angle);
        auto y = std::sinf(t * angle);
        float3 xyz{ x, y, 0.f };
        return mul(M, xyz);
    }

    int get_k(float q) const override {
        return (int)(3 + q * 29);
    }
};