#pragma once

#include "../lib/util.hpp"
#include "../src/math.cpp"

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec3 color;
    Vec2 uv;
};

struct EdgeWrapping
{
    Int from;
    Int to;
    Int back;
    Bool skip;
};

// TODO: This function tries to calculate convex hull with 3d gift wrapping algorithm, but produces messy self intersecting triangles and has bugs.
Array<UInt32> convex_hull_gift_wrapping(Array<Vertex> vertices)
{
    Array<UInt32> hull_indices = create_array<UInt32>();
    Array<EdgeWrapping> hull_edges = create_array<EdgeWrapping>();
    *hull_indices.push() = 0;
    *hull_indices.push() = 1;
    *hull_indices.push() = 2;
    *hull_edges.push() = EdgeWrapping{0, 1, 2, false};
    *hull_edges.push() = EdgeWrapping{1, 2, 0, false};
    *hull_edges.push() = EdgeWrapping{2, 0, 1, false};
    Int head = 0;
    while (hull_edges.count - head > 0)
    {
        EdgeWrapping *edge = &hull_edges[head++];
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
        EdgeWrapping new_edges[2] = {{vertex_index, edge->to, edge->from}, {edge->from, vertex_index, edge->to}};
        for (Int i = 0; i < hull_edges.count; i++)
        {
            EdgeWrapping *edge = &hull_edges[i];
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

struct Face
{
    Int a;
    Int b;
    Int c;
    Bool visible;
    Face *new_face;
};

struct Edge
{
    Int a;
    Int b;
    Face *faces[2];
};

Face *copy_face(Array<Face> *faces, Face *old_face)
{
    Face *new_face = faces->push();
    new_face->a = old_face->a;
    new_face->b = old_face->b;
    new_face->c = old_face->c;
    new_face->visible = false;
    new_face->new_face = null;
    return new_face;
}

Face *add_new_face(Array<Face> *faces, Face *old_face)
{
    if (old_face->new_face)
    {
        return old_face->new_face;
    }
    else
    {
        Face *new_face = copy_face(faces, old_face);
        old_face->new_face = new_face;
        return new_face;
    }
}

Face *add_new_face(Array<Face> *faces, Int a, Int b, Int c)
{
    Face *new_face = faces->push();
    new_face->a = a;
    new_face->b = b;
    new_face->c = c;
    new_face->visible = false;
    new_face->new_face = null;
    return new_face;
}

Edge *copy_edge(Array<Edge> *edges, Edge *old_edge)
{
    Edge *new_edge = edges->push();
    new_edge->a = old_edge->a;
    new_edge->b = old_edge->b;
    new_edge->faces[0] = old_edge->faces[0];
    new_edge->faces[1] = old_edge->faces[1];
    return new_edge;
}

Edge *add_new_edge(Array<Edge> *edges, Int a, Int b, Face *face0, Face *face1)
{
    Edge *new_edge = edges->push();
    new_edge->a = a;
    new_edge->b = b;
    new_edge->faces[0] = face0;
    new_edge->faces[1] = face1;
    return new_edge;
}

Array<UInt32> convex_hull_incremental(Array<Vertex> vertices)
{
    Vec3 base_min_x = {10000, 0, 0};
    Int base_min_x_i;
    Vec3 base_max_x = {-10000, 0, 0};
    Int base_max_x_i;
    Vec3 base_max_z = {0, 0, -10000};
    Int base_max_z_i;
    Vec3 top = {0, -10000, 0};
    Int top_i;
    for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
    {
        Vec3 pos = vertices[vertex_i].pos;
        if (ABS(pos.y) < 0.15)
        {
            if (pos.x < base_min_x.x)
            {
                base_min_x = pos;
                base_min_x_i = vertex_i;
            }
            if (pos.x > base_max_x.x)
            {
                base_max_x = pos;
                base_max_x_i = vertex_i;
            }
            if (pos.z > base_max_z.z)
            {
                base_max_z = pos;
                base_max_z_i = vertex_i;
            }
        }
        else if (pos.y > top.y)
        {
            top = pos;
            top_i = vertex_i;
        }
    }

    Array<Face> hull_faces = create_array<Face>(5000);
    *hull_faces.push() = {base_max_x_i, base_max_z_i, base_min_x_i, false, null};
    *hull_faces.push() = {base_min_x_i, top_i, base_max_x_i, false, null};
    *hull_faces.push() = {base_max_z_i, top_i, base_min_x_i, false, null};
    *hull_faces.push() = {base_max_x_i, top_i, base_max_z_i, false, null};
    Array<Edge> hull_edges = create_array<Edge>(5000);
    *hull_edges.push() = {base_max_x_i, base_min_x_i, {&hull_faces[1], &hull_faces[0]}};
    *hull_edges.push() = {base_min_x_i, base_max_z_i, {&hull_faces[2], &hull_faces[0]}};
    *hull_edges.push() = {base_max_z_i, base_max_x_i, {&hull_faces[3], &hull_faces[0]}};
    *hull_edges.push() = {base_min_x_i, top_i, {&hull_faces[1], &hull_faces[2]}};
    *hull_edges.push() = {base_max_z_i, top_i, {&hull_faces[2], &hull_faces[3]}};
    *hull_edges.push() = {base_max_x_i, top_i, {&hull_faces[3], &hull_faces[1]}};

    Array<Face> hull_faces_staging = create_array<Face>(5000);
    Array<Edge> hull_edges_staging = create_array<Edge>(5000);
    Array<Face *> hull_faces_new = create_array<Face *>(5000);

    for (Int vertex_i = 0; vertex_i < vertices.count; vertex_i++)
    {
        Bool has_visible_face = false;
        Vec3 pos = vertices[vertex_i].pos;
        for (Int face_i = 0; face_i < hull_faces.count; face_i++)
        {
            Face *face = &hull_faces[face_i];
            Vec3 pos_a = vertices[face->a].pos;
            Vec3 pos_b = vertices[face->b].pos;
            Vec3 pos_c = vertices[face->c].pos;

            Real volume = dot(pos - pos_a, cross(pos_b - pos_a, pos_c - pos_b));
            if (volume > 0.1)
            {
                face->visible = true;
                has_visible_face = true;
            }
        }

        if (has_visible_face)
        {
            hull_faces_staging.count = 0;
            hull_edges_staging.count = 0;
            hull_faces_new.count = 0;
            for (Int edge_i = 0; edge_i < hull_edges.count; edge_i++)
            {
                Edge *edge = &hull_edges[edge_i];
                if (edge->faces[0]->visible && !edge->faces[1]->visible)
                {
                    Edge *edge_staging = copy_edge(&hull_edges_staging, edge);
                    edge_staging->faces[0] = add_new_face(&hull_faces_staging, edge_staging->a, edge_staging->b, vertex_i);
                    edge_staging->faces[1] = add_new_face(&hull_faces_staging, edge->faces[1]);
                    *hull_faces_new.push() = edge_staging->faces[0];
                }
                else if (!edge->faces[0]->visible && edge->faces[1]->visible)
                {
                    Edge *edge_staging = copy_edge(&hull_edges_staging, edge);
                    edge_staging->faces[0] = add_new_face(&hull_faces_staging, edge->faces[0]);
                    edge_staging->faces[1] = add_new_face(&hull_faces_staging, edge_staging->b, edge_staging->a, vertex_i);
                    *hull_faces_new.push() = edge_staging->faces[1];
                }
                else if (!edge->faces[0]->visible && !edge->faces[1]->visible)
                {
                    Edge *edge_staging = copy_edge(&hull_edges_staging, edge);
                    edge_staging->faces[0] = add_new_face(&hull_faces_staging, edge->faces[0]);
                    edge_staging->faces[1] = add_new_face(&hull_faces_staging, edge->faces[1]);
                }
            }

            for (Int face_i = 0; face_i < hull_faces_new.count; face_i++)
            {
                Face *face = hull_faces_new[face_i];
                Face *adj_face = null;
                for (Int face_j = 0; face_j < hull_faces_new.count; face_j++)
                {
                    if (hull_faces_new[face_j]->b == face->a)
                    {
                        if (adj_face == null)
                        {
                            adj_face = hull_faces_new[face_j];
                        }
                        else
                        {
                            ASSERT(false);
                        }
                    }
                }
                ASSERT(adj_face != null);
                add_new_edge(&hull_edges_staging, face->c, face->a, face, adj_face);
            }

            swap(&hull_faces, &hull_faces_staging);
            swap(&hull_edges, &hull_edges_staging);
        }
    }

    Array<UInt32> hull_indices = create_array<UInt32>();
    for (Int face_i = 0; face_i < hull_faces.count; face_i++)
    {
        Face *face = &hull_faces[face_i];
        *hull_indices.push() = face->a;
        *hull_indices.push() = face->b;
        *hull_indices.push() = face->c;
    }
    return hull_indices;
}
