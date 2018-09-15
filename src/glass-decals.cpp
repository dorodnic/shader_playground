#include "glass-decals.h"
#include "easylogging++.h"

glass_decals_shader::glass_decals_shader()
    : texture_2d_shader(shader_program::load(
        "resources/shaders/planar/decal-atlas-vertex.glsl",
        "resources/shaders/planar/decal-atlas-fragment.glsl"))
{
    _width_location = _shader->get_uniform_location("imageWidth");
    _height_location = _shader->get_uniform_location("imageHeight");
    _horizontal_location = _shader->get_uniform_location("horizontal");
}

void glass_decals_shader::set_width_height(bool horizontal, int w, int h)
{
    _shader->load_uniform(_width_location, w);
    _shader->load_uniform(_height_location, h);
    _shader->load_uniform(_horizontal_location, horizontal ? 1.0f : 0.0f);
}

radial_shader::radial_shader()
    : texture_2d_shader(shader_program::load(
        "resources/shaders/planar/plane-vertex.glsl",
        "resources/shaders/planar/radial-fragment.glsl"))
{
    _decals_count_location = _shader->get_uniform_location("decalsCount");
}

void radial_shader::set_decals_count(int count)
{
    _shader->load_uniform(_decals_count_location, count);
}

normal_mapper_shader::normal_mapper_shader()
    : simple_shader(shader_program::load(
        "resources/shaders/simple/simp-vertex.glsl",
        "resources/shaders/simple/normal-mapper.glsl"))
{
}

void glass_atlas::release()
{
    glasses.clear();
    glass_impact.reset();
    glass_impact2.reset();
    texture_atlas.reset();
}

void glass_atlas::generate_decals(texture_handle white)
{
    reload_fbos();

    LOG(INFO) << "Generating Glass Decals:";
    glasses.clear();

    simple_shader shader;

    glass_impact->bind();
    shader.begin();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.set_light({ 0.f, 0.f, -1.f });
    shader.set_material_properties(1.f, 1.f, 0.f);

    auto projection = create_orthographic_projection_matrix(
        glass_impact->get_width(),
        glass_impact->get_height(), glass_impact->get_width(), 0.1f, 1000.f);
    shader.set_mvp(identity_t{},
        scaling_matrix(float3{ 2.f / glass_variations, 2.f / glass_variations, -0.5f }),
        projection);

    _textures.with_texture(white, 0, [&]() {
        int index = 0;
        for (auto& gm : _glass_models)
        {
            std::vector<std::pair<std::shared_ptr<vao>, glass_peice>> glass;
            for (auto& p : gm)
            {
                glass.emplace_back(
                    vao::create(p.peice),
                    p
                );
            }

            for (auto& g : glass)
            {
                if (g.second.dist == 0) continue;

                auto view =
                    translation_matrix(float3{
                    g.second.pos.x,
                    g.second.pos.y,
                    0.f });

                int i = index / glass_variations - glass_variations / 2;
                int j = index % glass_variations - glass_variations / 2;

                view = mul(
                    translation_matrix(float3{ i + 0.1f, j + 1.f - 0.1f, 1.f }),
                    scaling_matrix(float3{ 0.8f, 0.8f, 1.f }), // to prevent any pixels from sticking to the edge
                    view);

                shader.set_model(view);

                g.first->draw();
            }

            glasses.emplace_back(glass);

            index++;
        }
    });

    glass_impact->unbind();
    shader.end();

    {
        texture_atlas->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto shader = std::make_shared<radial_shader>();
        shader->begin();
        shader->set_decals_count(glass_variations);
        visualizer_2d viz(shader);
        viz.draw_texture(_textures.get(white));

        auto shader2 = std::make_shared<normal_mapper_shader>();
        shader2->begin();
        shader2->set_light({ 0.f, 0.f, -1.f });
        shader2->set_material_properties(1.f, 1.f, 0.f);
        shader2->set_mvp(identity_t{},
            scaling_matrix(float3{ 2.f / glass_variations, 2.f / glass_variations, -0.5f }),
            projection);

        _textures.with_texture(white, 0, [&]() {
            int index = 0;
            for (auto& glass : glasses)
            {
                int i = index / glass_variations - glass_variations / 2;
                int j = index % glass_variations - glass_variations / 2;

                for (auto& g : glass)
                {
                    auto view =
                        translation_matrix(float3{
                        g.second.pos.x,
                        g.second.pos.y,
                        0.f });

                    auto dist = length2(g.second.pos - float3{ 0.5f, -0.5f, 0.f });
                    auto sf = std::max(0.f, std::min(1.f, dist));
                    sf = sqrt(sqrt(sqrt(sqrt(sf))));

                    view = mul(view, scaling_matrix(float3{ sf, sf, 1.f })
                    );

                    view = mul(
                        translation_matrix(float3{ i + 0.3f, j + 1.f - 0.3f, 1.f }),
                        scaling_matrix(float3{ 0.4f, 0.4f, 1.f }), // to prevent any pixels from sticking to the edge
                        view);

                    shader2->set_model(view);

                    g.first->draw();
                }

                //break;

                index++;
            }
        });
        texture_atlas->unbind();
        shader2->end();
    }

    LOG(INFO) << "Applying horizontal blur:";
    glass_impact2->bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glass_decals_shader blur_shader;
    blur_shader.begin();
    {
        texture_visualizer fbo_visualizer(float2{ 0.0f, 0.0f }, float2{ 1.f, 1.f });
        blur_shader.set_width_height(true, glass_impact->get_width(), glass_impact->get_height());
        fbo_visualizer.draw(blur_shader, _textures.get(final_glass_atlas));
    }

    blur_shader.end();
    glass_impact2->unbind();

    LOG(INFO) << "Applying vertical blur:";
    glass_impact->bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    blur_shader.begin();
    {
        texture_visualizer fbo_visualizer(float2{ 0.0f, 0.0f }, float2{ 1.f, 1.f });
        blur_shader.set_width_height(false, glass_impact2->get_width(), glass_impact2->get_height());
        fbo_visualizer.draw(blur_shader, _textures.get(temp_glass_atlas));
    }

    blur_shader.end();
    glass_impact->unbind();
}

void glass_atlas::reload_fbos()
{
    glass_impact = std::make_shared<fbo>(1024, 1024);
    glass_impact->createTextureAttachment(_textures.get(final_glass_atlas));
    
    glass_impact2 = std::make_shared<fbo>(2048, 2048);
    glass_impact2->createTextureAttachment(_textures.get(temp_glass_atlas));
    
    texture_atlas = std::make_shared<fbo>(2048, 2048);
    texture_atlas->createTextureAttachment(_textures.get(diffuse_glass_atlas), false);
}

glass_atlas::glass_atlas(textures& tex) : _textures(tex)
{
    temp_glass_atlas = _textures.add_texture("temp_glass_atlas");
    final_glass_atlas = _textures.add_texture("final_glass_atlas");
    diffuse_glass_atlas = _textures.add_texture("diffuse_glass_atlas");

    for (int i = 0; i < glass_variations*glass_variations; i++)
    {
        std::vector<glass_peice> t;
        generate_broken_glass(t);
        _glass_models.emplace_back(t);
    }

    uvs = { 0.f, 0.f };
    tsp = identity_t{};
}

void glass_atlas::randomize_hitpoint(
    const obj_mesh& mesh, 
    const float4x4& transform)
{
    auto n = mesh.positions.size();

    glass_id = rand() % glasses.size();

    float3 normal;

    float4x4 mat = transform;

    auto k = int(0.2 * n + rand() % int(n * 0.6));

    auto pos = mul(mat, float4(mesh.positions[k], 1.0)).xyz();

    normal = normalize(mesh.normals[k]);
    auto tangent = normalize(mesh.tangents[k]);
    auto third = normalize(cross(normal, tangent));
    tsp = {
        { tangent, 0.f },
        { third, 0.f },
        { normal, 0.f },
        { pos, 1.f }
    };

    uvs = mesh.uvs[k];
}

void glass_atlas::draw_scatter(float t, tube_shader& shader)
{
    float ambient;
    float reflectivity;
    float shine;
    shader.get_material_properties(ambient, shine, reflectivity);

    shader.enable_normal_mapping(false);
    shader.set_decal_id(glass_id, glass_variations);

    shader.set_decal_uvs(uvs);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (auto& g : glasses[glass_id])
    {
        if (g.second.dist == 0) continue;

        auto st = t * 15.f;
        auto base = floor(st / 6.f) * 6.f;

        auto t0 = st - base;

        auto local_t = t0 * g.second.dist * 1.f;
        double w = cos(local_t * 7.0);
        float4 q(sinf(local_t * 7.0) * g.second.rotation, w);
        q = normalize(q);

        shader.set_material_properties(ambient * std::min(1.f + local_t, 1.5f),
            shine * std::min(1.f + local_t * 50.f, 60.f),
            reflectivity * std::min(1.f + local_t * 50.f, 60.f));

        auto out_vec = 10.f * (g.second.pos - float3{ 0.5f, -0.5f, 0.f });

        auto view = mul(
            scaling_matrix(float3{ 1.f, 1.f, 1.f }),
            translation_matrix(float3{
            -0.5f + g.second.pos.x + out_vec.x * local_t,
            0.5f + g.second.pos.y + out_vec.y * local_t,
            5.f * local_t - 0.03f }),
            rotation_matrix(q)
            );

        view = mul(tsp, view);

        shader.set_model(view);

        g.first->draw();
    }
};