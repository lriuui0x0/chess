#include "fbxsdk.h"

#include "../lib/util.hpp"
#include "../src/math.cpp"
#include <cstdio>
#include "convex_hull.cpp"

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

Void output(Array<Vertex> vertices, Array<UInt32> indices, Vec3 *box_center, Vec3 *box_radius, Array<UInt32> hull_indices, CStr output_filename)
{
    FILE *output_file = fopen(output_filename, "wb");
    ASSERT(fseek(output_file, 0, SEEK_SET) == 0);

    ASSERT(fwrite(&vertices.count, sizeof(Int32), 1, output_file) == 1);
    for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
    {
        Vertex *vertex = &vertices[vertex_i];
        Vec3 vertex_pos = vertex->pos;
        ASSERT(fwrite(&vertex_pos.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.z, sizeof(Real), 1, output_file) == 1);

        Vec3 vertex_normal = vertex->normal;
        ASSERT(fwrite(&vertex_normal.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_normal.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_normal.z, sizeof(Real), 1, output_file) == 1);

        Vec3 vertex_color = vertex->color;
        ASSERT(fwrite(&vertex_color.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_color.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_color.z, sizeof(Real), 1, output_file) == 1);
    }

    ASSERT(fwrite(&indices.count, sizeof(Int32), 1, output_file) == 1);
    ASSERT(fwrite(indices.data, sizeof(UInt32), indices.count, output_file) == (size_t)indices.count);

    ASSERT(fwrite(&box_center->x, sizeof(Real), 1, output_file) == 1);
    ASSERT(fwrite(&box_center->y, sizeof(Real), 1, output_file) == 1);
    ASSERT(fwrite(&box_center->z, sizeof(Real), 1, output_file) == 1);

    ASSERT(fwrite(&box_radius->x, sizeof(Real), 1, output_file) == 1);
    ASSERT(fwrite(&box_radius->y, sizeof(Real), 1, output_file) == 1);
    ASSERT(fwrite(&box_radius->z, sizeof(Real), 1, output_file) == 1);

    ASSERT(fwrite(&hull_indices.count, sizeof(Int32), 1, output_file) == 1);
    for (Int hull_index_i = 0; hull_index_i < hull_indices.count; hull_index_i++)
    {
        Vec3 vertex_pos = vertices[hull_indices[hull_index_i]].pos;
        ASSERT(fwrite(&vertex_pos.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.z, sizeof(Real), 1, output_file) == 1);
    }
}

int main(Int argc, CStr *argv)
{
    argc--;
    argv++;

    ASSERT(argc == 2 || argc == 3);
    CStr input_filename = argv[0];
    CStr output_filename = argv[1];

    FbxManager *manager = FbxManager::Create();
    FbxIOSettings *io_settings = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(io_settings);
    FbxImporter *importer = FbxImporter::Create(manager, "");
    ASSERT(importer->Initialize(input_filename, -1, manager->GetIOSettings()));

    FbxScene *scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);

    FbxNode *node = scene->GetRootNode()->GetChild(0);
    FbxMesh *mesh = (FbxMesh *)node->GetNodeAttribute();

    FbxLayerElementMaterial *material_layer = mesh->GetLayer(0)->GetMaterials();
    ASSERT(material_layer);

    Array<Vertex> vertices = create_array<Vertex>();
    Array<UInt32> indices = create_array<UInt32>();

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
            Vec3 vertex_normal = normalize(convert_vec3(fbx_normal));
            FbxVector4 fbx_vertex_pos = mesh->GetControlPointAt(vertex_index);
            Vec3 vertex_pos = convert_vec3(fbx_vertex_pos);

            Vertex *vertex = vertices.push();
            vertex->pos = vertex_pos;
            vertex->normal = vertex_normal;
            vertex->color = vertex_color;
            *indices.push() = vertices.count - 1;
        }
    }

    Vec3 min = {+10000, +10000, +10000};
    Vec3 max = {-10000, -10000, -10000};
    for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
    {
        Vec3 pos = vertices[vertex_i].pos;
        for (Int i = 0; i < 3; i++)
        {
            min[i] = MIN(min[i], pos[i]);
            max[i] = MAX(max[i], pos[i]);
        }
    }
    Vec3 box_center = 0.5 * (max + min);
    Vec3 box_radius = 0.5 * (max - min);

    Array<UInt32> hull_indices;
    if (argc == 2)
    {
        hull_indices = convex_hull_incremental(vertices);
    }
    else
    {
        hull_indices = create_array<UInt32>();
    }
    output(vertices, indices, &box_center, &box_radius, hull_indices, output_filename);
}
