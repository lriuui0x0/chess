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

Void output(Array<Vertex> vertices, Array<UInt32> indices, Array<UInt32> hull_indices, CStr output_filename)
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

    ASSERT(fwrite(&hull_indices.count, sizeof(Int32), 1, output_file) == 1);
    for (Int hull_index_i = 0; hull_index_i < hull_indices.count; hull_index_i++)
    {
        Vec3 vertex_pos = vertices[hull_indices[hull_index_i]].pos;
        ASSERT(fwrite(&vertex_pos.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.z, sizeof(Real), 1, output_file) == 1);
    }
}

Vec3 calculate_closest_vertex(Array<Vertex> vertices, Vec3 base_from, Vec3 base_to)
{
    Vec3 base = base_to - base_from;
    Real cos_angle_min = 2;
    Int closest_vertex_index = -1;
    for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
    {
        Vec3 vertex_pos = vertices[vertex_i].pos; 
        Vec3 top = vertex_pos - base_to;
        Vec3 cross_product = cross(base, top);
        Real cos_angle = dot(cross_product, Vec3{0, 1, 0}) / norm(cross_product);

        if (cos_angle < cos_angle_min)
        {
            cos_angle_min = cos_angle;
            closest_vertex_index = vertex_i;
        }
    }

    ASSERT(closest_vertex_index != -1);
    Vec3 closest_vertex = vertices[closest_vertex_index].pos;
    ASSERT(fabs(closest_vertex.y) > 0.1);
    return closest_vertex;
}

Array<Vec3> calculate_simple_convex_hull(Array<Vertex> vertices)
{
    Real min_x[2] = {1000, 1000};
    Real max_x[2] = {-1000, -1000};
    Real min_z = 1000;
    Real max_z = -1000;
    Real max_y = -1000;
    for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
    {
        Vec3 vertex_pos = vertices[vertex_i].pos;
        if (ABS(vertex_pos.y) < 0.1)
        {
            if (min_x[0] - vertex_pos.x > 0.1)
            {
                min_x[1] = min_x[0];
                min_x[0] = vertex_pos.x;
            }
            else if (vertex_pos.x - min_x[0] > 0.1 && min_x[1] - vertex_pos.x > 0.1)
            {
                min_x[1] = vertex_pos.x;
            }

            if (vertex_pos.x - max_x[0] > 0.1)
            {
                max_x[1] = max_x[0];
                max_x[0] = vertex_pos.x;
            }
            else if (min_x[0] - vertex_pos.x > 0.1 && vertex_pos.x - max_x[1] > 0.1)
            {
                max_x[1] = vertex_pos.x;
            }

            if (min_z - vertex_pos.z > 0.1)
            {
                min_z = vertex_pos.z;
            }

            if (vertex_pos.z - max_z > 0.1)
            {
                max_z = vertex_pos.z;
            }
        }
        else
        {
            if (vertex_pos.y > max_y)
            {
                max_y = vertex_pos.y;
            }
        }
    }

    Vec3 bases[6];
    bases[0] = Vec3{max_x[0], 0, 0};
    bases[1] = Vec3{max_x[1], 0, min_z};
    bases[2] = Vec3{min_x[1], 0, min_z};
    bases[3] = Vec3{min_x[0], 0, 0};
    bases[4] = Vec3{min_x[1], 0, max_z};
    bases[5] = Vec3{max_x[1], 0, max_z};

    Array<Vec3> result = create_array<Vec3>();
    for (Int i = 0; i < 6; i++)
    {
        Vec3 base_from = bases[i];
        Vec3 base_to = bases[(i + 1) % 6];

        Vec3 top_from = base_from;
        top_from.y = max_y;
        Vec3 top_to = base_to;
        top_to.y = max_y;

        *result.push() = base_from;
        *result.push() = base_to;
        *result.push() = top_from;

        *result.push() = top_to;
        *result.push() = top_from;
        *result.push() = base_to;

        *result.push() = base_to;
        *result.push() = base_from;
        *result.push() = {0, 0, 0};

        *result.push() = top_from;
        *result.push() = top_to;
        *result.push() = {0, max_y, 0};
    }

    return result;
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

    Array<UInt32> hull_indices;
    if (argc == 2)
    {
        hull_indices = convex_hull_incremental(vertices);
    }
    else
    {
        hull_indices = create_array<UInt32>();
    }
    output(vertices, indices, hull_indices, output_filename);
}
