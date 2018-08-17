#pragma once

#include "util.h"

#include <vector>
#include <thread>
#include <readerwriterqueue.h>

struct obj_mesh
{
    std::string         name;
    std::vector<int3>   indexes;
    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> uvs;
    std::vector<float3> tangents;
};

typedef std::vector<obj_mesh> obj_file;

class loader
{
public:
    loader(std::string filename);

    bool ready() const;
    obj_file& get();

    ~loader();

private:
    void thread_proc(std::string name);

    std::thread _thread;
    mutable moodycamel::ReaderWriterQueue<obj_file> _queue;
    mutable obj_file _result;
    mutable bool _ready = false;
    float _progress = 0.f;
};