#include "fbxsdk.h"

#include "../src/util.cpp"

int main()
{
    FbxManager *manager = FbxManager::Create();
    FbxIOSettings *io_settings = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(io_settings);
    FbxImporter *importer = FbxImporter::Create(manager, "");
    assert(importer->Initialize("asset/Board.fbx", -1, manager->GetIOSettings()));

    FbxScene *scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);

    FbxNode *node = scene->GetRootNode()->FindChild("Board");
    FbxMesh *mesh = (FbxMesh*) node->GetNodeAttribute();
}
