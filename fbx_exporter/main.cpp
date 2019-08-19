#include "fbxsdk.h"

#include "../src/util.cpp"
#include "../src/math.cpp"
#include <cstdio>

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

    Int4 vertex_count = mesh->GetControlPointsCount();
    Int polygon_count = mesh->GetPolygonCount();
    Int indices_count = polygon_count * 3;

    assert(fwrite(&vertex_count, sizeof(Int4), 1, output_file) == 1);
    for (Int vertex_index = 0; vertex_index < vertex_count; vertex_index++)
    {
        FbxVector4 vertex = mesh->GetControlPointAt(vertex_index);
        Real x = vertex.mData[0];
        Real y = vertex.mData[1];
        Real z = vertex.mData[2];
        assert(fwrite(&x, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&y, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&z, sizeof(Real), 1, output_file) == 1);

        Vec3 normal = {0, 0, 0};
        for (Int polygon_index = 0; polygon_index < polygon_count; polygon_index++)
        {
            for (Int polygon_vertex_index = 0; polygon_vertex_index < 3; polygon_vertex_index++)
            {
                Int4 curr_vertex_index = mesh->GetPolygonVertex(polygon_index, polygon_vertex_index);
                if (curr_vertex_index == vertex_index)
                {
                    FbxVector4 fbx_normal;
                    assert(mesh->GetPolygonVertexNormal(polygon_index, polygon_vertex_index, fbx_normal));

                    Vec3 current_normal = {(Real) fbx_normal.mData[0], (Real)fbx_normal.mData[1], (Real)fbx_normal.mData[2]};
                    normal = normal + current_normal;
                }
            }
        }

        normal = normalize(normal); 
        assert(fwrite(&normal.x, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&normal.y, sizeof(Real), 1, output_file) == 1);
        assert(fwrite(&normal.z, sizeof(Real), 1, output_file) == 1);
    }

    assert(fwrite(&indices_count, sizeof(Int4), 1, output_file) == 1);
    for (Int polygon_index = 0; polygon_index < polygon_count; polygon_index++)
    {
        for (Int polygon_vertex_index = 0; polygon_vertex_index < 3; polygon_vertex_index++)
        {
            Int4 vertex_index = mesh->GetPolygonVertex(polygon_index, polygon_vertex_index);
            assert(fwrite(&vertex_index, sizeof(Int4), 1, output_file) == 1);
        }
    }
}
