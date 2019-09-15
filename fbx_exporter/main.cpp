#include "fbxsdk.h"

#include "../lib/util.hpp"
#include "../src/math.cpp"
#include <cstdio>

struct VertexCopy
{
    Int index;
    Vec3 normal;
    Vec3 color;
};

Vec3 convert_vec3(FbxVector4 fbx_vec4)
{
    Real x = fbx_vec4.mData[0];
    Real y = fbx_vec4.mData[1];
    Real z = fbx_vec4.mData[2];
    return {x, y, z};
}

Vec3 convert_vec3(FbxDouble3 fbx_double3)
{
    Real x = fbx_double3.mData[0];
    Real y = fbx_double3.mData[1];
    Real z = fbx_double3.mData[2];
    return {x, y, z};
}

Bool vec3_equal(Vec3 u, Vec3 v)
{
    return u.x == v.x && u.y == v.y && u.z == v.z;
}

int main(Int argc, CStr *argv)
{
    argc--;
    argv++;

    ASSERT(argc == 2);
    CStr input_filename = argv[0];
    CStr output_filename = argv[1];

    FbxManager *manager = FbxManager::Create();
    FbxIOSettings *io_settings = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(io_settings);
    FbxImporter *importer = FbxImporter::Create(manager, "");
    ASSERT(importer->Initialize(input_filename, -1, manager->GetIOSettings()));

    FILE *output_file = fopen(output_filename, "wb");
    ASSERT(fseek(output_file, 0, SEEK_SET) == 0);

    FbxScene *scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);

    FbxNode *node = scene->GetRootNode()->GetChild(0);
    FbxMesh *mesh = (FbxMesh *)node->GetNodeAttribute();

    FbxLayerElementMaterial *material_layer = mesh->GetLayer(0)->GetMaterials();
    ASSERT(material_layer);

    Array<VertexCopy> vertex_copies = create_array<VertexCopy>();
    Array<UInt32> indices = create_array<UInt32>();

    Int found_times = 0;
    Int polygon_count = mesh->GetPolygonCount();
    for (Int polygon_index = 0; polygon_index < polygon_count; polygon_index++)
    {
        Int material_index = material_layer->GetMappingMode() == FbxLayerElement::eAllSame ? material_layer->GetIndexArray()[0] : material_layer->GetIndexArray()[polygon_index];
        FbxSurfaceLambert *material = (FbxSurfaceLambert *)scene->GetMaterial(material_index);
        ASSERT(material);
        FbxDouble3 fbx_color = material->Diffuse.Get();
        Vec3 vertex_color = convert_vec3(fbx_color);

        for (Int polygon_vertex_index = 0; polygon_vertex_index < 3; polygon_vertex_index++)
        {
            Int32 vertex_index = mesh->GetPolygonVertex(polygon_index, polygon_vertex_index);
            ASSERT(vertex_index != -1);
            FbxVector4 fbx_normal;
            ASSERT(mesh->GetPolygonVertexNormal(polygon_index, polygon_vertex_index, fbx_normal));
            Vec3 vertex_normal = convert_vec3(fbx_normal);

            Bool found_copy = false;
            for (Int vertex_copy_index = 0; vertex_copy_index < vertex_copies.count; vertex_copy_index++)
            {
                VertexCopy *vertex_copy = &vertex_copies[vertex_copy_index];
                if (vertex_index == vertex_copy->index && vec3_equal(vertex_copy->normal, vertex_normal) && vec3_equal(vertex_copy->color, vertex_color))
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
                vertex_copy->color = vertex_color;
                *indices.push() = vertex_copies.count - 1;
            }
        }
    }

    ASSERT(fwrite(&vertex_copies.count, sizeof(Int32), 1, output_file) == 1);
    for (Int i = 0; i < vertex_copies.count; i++)
    {
        VertexCopy *vertex_copy = &vertex_copies[i];

        FbxVector4 fbx_vertex_pos = mesh->GetControlPointAt(vertex_copy->index);
        Vec3 vertex_pos = convert_vec3(fbx_vertex_pos);
        ASSERT(fwrite(&vertex_pos.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.z, sizeof(Real), 1, output_file) == 1);

        Vec3 vertex_normal = normalize(vertex_copy->normal);
        ASSERT(fwrite(&vertex_normal.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_normal.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_normal.z, sizeof(Real), 1, output_file) == 1);

        Vec3 vertex_color = vertex_copy->color;
        ASSERT(fwrite(&vertex_color.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_color.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_color.z, sizeof(Real), 1, output_file) == 1);
    }

    ASSERT(fwrite(&indices.count, sizeof(Int32), 1, output_file) == 1);
    ASSERT(fwrite(indices.data, sizeof(UInt32), indices.count, output_file) == (size_t)indices.count);
}
