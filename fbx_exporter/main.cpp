#include "fbxsdk.h"

#include "../lib/util.hpp"
#include "../src/math.cpp"
#include <cstdio>

struct Vertex
{
    Vec3 pos;
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

struct Edge
{
    Int from;
    Int to;
    Int back;
    Bool skip;
};

Void output(Array<Vertex> vertices, Array<UInt32> indices, Array<Vec3> hull_vertices, CStr output_filename)
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

    ASSERT(fwrite(&hull_vertices.count, sizeof(Int32), 1, output_file) == 1);
    for (Int vertex_i = 0; vertex_i < hull_vertices.count; vertex_i++)
    {
        Vec3 vertex_pos = hull_vertices[vertex_i];
        ASSERT(fwrite(&vertex_pos.x, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.y, sizeof(Real), 1, output_file) == 1);
        ASSERT(fwrite(&vertex_pos.z, sizeof(Real), 1, output_file) == 1);
    }
}

// TODO: This function tries to calculate convex hull with 3d gift wrapping algorithm, but produces messy self intersecting triangles and has bugs.
Array<UInt32> calculate_convex_hull(Array<Vertex> vertices)
{
    Array<UInt32> hull_indices = create_array<UInt32>();
    Array<Edge> hull_edges = create_array<Edge>();
    *hull_indices.push() = 0;
    *hull_indices.push() = 1;
    *hull_indices.push() = 2;
    *hull_edges.push() = Edge{0, 1, 2, false};
    *hull_edges.push() = Edge{1, 2, 0, false};
    *hull_edges.push() = Edge{2, 0, 1, false};
    Int head = 0;
    while (hull_edges.count - head > 0)
    {
        Edge *edge = &hull_edges[head++];
        if (edge->skip)
        {
            continue;
        }

        Vec3 edge_segment = vertices[edge->to].pos - vertices[edge->from].pos;
        Vec3 back_segment = vertices[edge->back].pos - vertices[edge->to].pos;
        Vec3 back_cross_product = cross(edge_segment, back_segment);
        Real back_cross_product_norm = norm(back_cross_product);

        Real cos_angle = 2;
        Int vertex_index = -1;
        for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
        {
            Vec3 vertex_pos = vertices[vertex_i].pos;
            Vec3 front_segment = vertex_pos - vertices[edge->to].pos;
            Vec3 front_cross_product = cross(edge_segment, front_segment);
            Real front_cross_product_norm = norm(front_cross_product);
            if (front_cross_product_norm < 0.1)
            {
                continue;
            }

            Real front_cos_angle = dot(back_cross_product, front_cross_product) / (back_cross_product_norm * front_cross_product_norm);
            if (front_cos_angle < cos_angle)
            {
                vertex_index = vertex_i;
                cos_angle = front_cos_angle;
            }
        }
        ASSERT(vertex_index != -1);

        for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
        {
            Vec3 front_segment = vertices[vertex_index].pos - vertices[edge->to].pos;
            Vec3 test_segment = vertices[vertex_i].pos - vertices[edge->from].pos;

            Real signed_volume = dot(test_segment, cross(edge_segment, front_segment));
            ASSERT(signed_volume > -0.1);
        }

        *hull_indices.push() = edge->from;
        *hull_indices.push() = edge->to;
        *hull_indices.push() = vertex_index;

        Bool seen_new_edge[2] = {false, false};
        Edge new_edges[2] = {{vertex_index, edge->to, edge->from}, {edge->from, vertex_index, edge->to}};
        for (Int i = 0; i < hull_edges.count; i++)
        {
            Edge *edge = &hull_edges[i];
            for (Int i = 0; i < 2; i++)
            {
                if (!seen_new_edge[i])
                {
                    if (new_edges[i].from == edge->from && new_edges[i].to == edge->to ||
                        new_edges[i].to == edge->from && new_edges[i].from == edge->to)
                    {
                        edge->skip = true;
                        seen_new_edge[i] = true;
                    }
                }
            }
        }

        for (Int i = 0; i < 2; i++)
        {
            if (!seen_new_edge[i])
            {
                *hull_edges.push() = new_edges[i];
            }
        }
    }

    return hull_indices;
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

    Array<Vec3> hull_vertices;
    if (argc == 2)
    {
        hull_vertices = calculate_simple_convex_hull(vertices);
    }
    else
    {
        hull_vertices = create_array<Vec3>();
    }
    output(vertices, indices, hull_vertices, output_filename);
}
