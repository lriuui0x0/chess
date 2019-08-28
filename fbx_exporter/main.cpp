#include "fbxsdk.h"

#include "../src/util.cpp"
#include "../src/math.cpp"
#include <cstdio>

struct VertexCopy
{
    Int index;
    Vec3 normal;
};

Vec3 convert_vec3(FbxVector4 fbx_vec4)
{
    Real x = fbx_vec4.mData[0];
    Real y = fbx_vec4.mData[1];
    Real z = fbx_vec4.mData[2];
    return {x, y, z};
}

Bool vec3_equal(Vec3 u, Vec3 v)
{
    return u.x == v.x && u.y == v.y && u.z == v.z;
}

int main(Int argc, RawStr *argv)
{
    argc--;
    argv++;

    assert(argc == 2);
    RawStr input_filename = argv[0];
    RawStr output_filename = argv[1];

    FbxManager *manager = FbxManager::Create();
    FbxIOSettings *io_settings = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(io_settings);
    FbxImporter *importer = FbxImporter::Create(manager, "");
    assert(importer->Initialize(input_filename, -1, manager->GetIOSettings()));

    FILE *output_file = fopen(output_filename, "wb");
    assert(fseek(output_file, 0, SEEK_SET) == 0);

    FbxScene *scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);

    FbxNode *node = scene->GetRootNode()->GetChild(0);
    FbxMesh *mesh = (FbxMesh *)node->GetNodeAttribute();

    Array<VertexCopy> vertex_copies = create_array<VertexCopy>();
    Array<UInt32> indices = create_array<UInt32>();

    Int found_times = 0;
    Int polygon_count = mesh->GetPolygonCount();
    for (Int polygon_index = 0; polygon_index < polygon_count; polygon_index++)
    {
        for (Int polygon_vertex_index = 0; polygon_vertex_index < 3; polygon_vertex_index++)
        {
            Int32 vertex_index = mesh->GetPolygonVertex(polygon_index, polygon_vertex_index);
            assert(vertex_index != -1);
            FbxVector4 fbx_normal;
            assert(mesh->GetPolygonVertexNormal(polygon_index, polygon_vertex_index, fbx_normal));
            Vec3 vertex_normal = convert_vec3(fbx_normal);

            Bool found_copy = false;
            for (Int vertex_copy_index = 0; vertex_copy_index < vertex_copies.length; vertex_copy_index++)
            {
                VertexCopy *vertex_copy = &vertex_copies[vertex_copy_index];
                if (vertex_index == vertex_copy->index && vec3_equal(vertex_copy->normal, vertex_normal))
                {
                    *indices.push() = vertex_copy_index;
                    found_copy = true;
                    found_times++;
                    break;
                }
            }

            if (!found_copy)
            {
                VertexCopy *vertex_copy = vertex_copies.push();
                vertex_copy->index = vertex_index;
                vertex_copy->normal = vertex_normal;
                *indices.push() = vertex_copies.length - 1;
            }
        }
    }

    assert(fwrite(&vertex_copies.length, sizeof(Int32), 1, output_file) == 1);
    for (Int i = 0; i < vertex_copies.length; i++)
    {
        VertexCopy *vertex_copy = &vertex_copies[i];

        FbxVector4 fbx_vertex_pos = mesh->GetControlPointAt(vertex_copy->index);
        Vec3 vertex_pos = convert_vec3(fbx_vertex_pos);
        assert(fwrite(&vertex_pos.x, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&vertex_pos.y, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&vertex_pos.z, sizeof(Real), 1, output_file) == 1);

        Vec3 vertex_normal = normalize(vertex_copy->normal);
        assert(fwrite(&vertex_normal.x, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&vertex_normal.y, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&vertex_normal.z, sizeof(Real), 1, output_file) == 1);
    }

    assert(fwrite(&indices.length, sizeof(Int32), 1, output_file) == 1);
    assert(fwrite(indices.data, sizeof(UInt32), indices.length, output_file) == (size_t)indices.length);
}
