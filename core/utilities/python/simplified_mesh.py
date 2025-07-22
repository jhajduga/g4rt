#!/usr/bin/env python3
import sys
import argparse

import numpy as np
import pandas as pd
import trimesh
import pymeshlab
from loguru import logger


# ----------------------------------------------------------------------
def snap_and_merge_vertices(mesh: trimesh.Trimesh, tol=1e-5):
    vertices = mesh.vertices.copy()
    faces = mesh.faces.copy()

    edges_set = set()
    for face in faces:
        edges_set.update([(face[i], face[j]) for i, j in [(0, 1), (1, 2), (2, 0)]])
    edges = np.array(list(edges_set))

    moved = 0
    for idx, v in enumerate(vertices):
        for edge in edges:
            if idx in edge:
                continue
            p1, p2 = vertices[edge[0]], vertices[edge[1]]
            edge_vec = p2 - p1
            edge_len = np.linalg.norm(edge_vec)
            if edge_len < tol:
                continue
            edge_dir = edge_vec / edge_len
            v_vec = v - p1
            proj_len = np.dot(v_vec, edge_dir)
            if proj_len < 0 or proj_len > edge_len:
                continue
            proj_point = p1 + proj_len * edge_dir
            dist_to_edge = np.linalg.norm(v - proj_point)
            if dist_to_edge < tol:
                dist_to_p1 = np.linalg.norm(v - p1)
                dist_to_p2 = np.linalg.norm(v - p2)
                nearest_point = p1 if dist_to_p1 < dist_to_p2 else p2
                vertices[idx] = nearest_point
                moved += 1
                break

    logger.info(f"Snapped {moved} vertices onto edge endpoints.")

    # Odbuduj siatkę, by zapewnić spójność
    mesh_fixed = trimesh.Trimesh(vertices=vertices, faces=faces, process=False)

    # Scal wierzchołki – użyj precyzyjniejszego podejścia
    mesh_fixed.merge_vertices()

    # Kompleksowa odbudowa bez wywołania .process()
    mesh_fixed.remove_duplicate_faces()
    mesh_fixed.remove_degenerate_faces()
    mesh_fixed.remove_unreferenced_vertices()

    # Alternatywnie: wymuś spójność używając konwersji do numpy i odbudowy
    mesh_fixed = trimesh.Trimesh(
        vertices=mesh_fixed.vertices.copy(),
        faces=mesh_fixed.faces.copy(),
        process=False
    )

    logger.info(f"Mesh has {len(mesh_fixed.vertices)} vertices and {len(mesh_fixed.faces)} faces after merge.")

    return mesh_fixed


# ----------------------------------------------------------------------
def clean_mesh(mesh: trimesh.Trimesh,
               min_area: float = 1e-8) -> trimesh.Trimesh:
    """
    1) process+validate
    2) remove faces with area <= min_area
    3) remove exact zero-area faces
    4) remove unreferenced vertices
    5) fill small holes
    6) fix normals
    7) keep only the largest connected component
    """
    mesh.process(validate=True)

    areas = mesh.area_faces
    keep = np.nonzero(areas > min_area)[0]
    if len(keep) < len(mesh.faces):
        mesh.update_faces(keep)

    mesh.remove_degenerate_faces()
    mesh.remove_unreferenced_vertices()
    mesh.fill_holes()
    mesh.fix_normals()

    parts = mesh.split(only_watertight=False)
    if len(parts) > 1:
        parts.sort(key=lambda m: len(m.faces), reverse=True)
        mesh = parts[0]

    return mesh

# ----------------------------------------------------------------------

def remove_self_intersections(mesh: trimesh.Trimesh,
                              hole_close_size: int = 1000) -> trimesh.Trimesh:
    """
    1) Select & delete self-intersecting faces
    2) Repair non-manifold edges & vertices
    3) Close holes
    """
    ms = pymeshlab.MeshSet()
    pm = pymeshlab.Mesh(
        vertex_matrix=mesh.vertices.astype(np.float64),
        face_matrix=mesh.faces.astype(np.int32)
    )
    ms.add_mesh(pm)

    # 1) zaznacz i usuń przecinające się twarze
    ms.apply_filter('compute_selection_by_self_intersections_per_face')
    ms.apply_filter('meshing_remove_selected_faces')

    # 2) napraw non-manifold:
    #    - najpierw krawędzie, potem wierzchołki
    ms.apply_filter('meshing_repair_non_manifold_edges')
    ms.apply_filter('meshing_repair_non_manifold_vertices')

    # 3) zamknij dziury (teraz, gdy krawędzie są manifold)
    ms.apply_filter('meshing_close_holes', maxholesize=hole_close_size)

    out = ms.current_mesh()
    return trimesh.Trimesh(
        vertices=out.vertex_matrix(),
        faces=   out.face_matrix(),
        process=False
    )

# ----------------------------------------------------------------------
def simplify_mesh(vertices: np.ndarray,
                  faces:    np.ndarray,
                  target_faces: int,
                  hole_close_size: int = 1000) -> trimesh.Trimesh:
    """
    1) Clean input mesh
    2) Remove self‐intersections
    3) Simplify via PyMeshLab
    4) Clean & remove self‐intersections again
    """
    # --- 1) Clean & validate ---
    mesh = trimesh.Trimesh(vertices=vertices, faces=faces, process=False)
    mesh = clean_mesh(mesh)
    mesh = snap_and_merge_vertices(mesh, tol=1e-3)
    

    # --- 2) Remove any self‐intersecting faces early ---
    mesh = remove_self_intersections(mesh, hole_close_size=hole_close_size)
    mesh = clean_mesh(mesh)

    # --- 3) Decimate if needed ---
    if len(mesh.faces) > target_faces:
        logger.info(f"Decimating {len(mesh.faces)} → {target_faces}")
        ms = pymeshlab.MeshSet()
        pm = pymeshlab.Mesh(
            vertex_matrix=mesh.vertices,
            face_matrix=mesh.faces
        )
        ms.add_mesh(pm)
        ms.meshing_decimation_quadric_edge_collapse(
            targetfacenum=target_faces,
            preservenormal=True
        )
        
        out_pm = ms.current_mesh()
        mesh = trimesh.Trimesh(
            vertices=out_pm.vertex_matrix(),
            faces=   out_pm.face_matrix(),
            process=False
        )

    # --- 4) Final clean & self‐intersection pass ---
    mesh = clean_mesh(mesh)
    mesh = remove_self_intersections(mesh, hole_close_size=hole_close_size)
    mesh = clean_mesh(mesh)

    logger.info(f"[simplify_mesh] final face count = {len(mesh.faces)}")
    return mesh

# ----------------------------------------------------------------------
def read_csv_db(path: str, sep: str = ';') -> pd.DataFrame:
    df = pd.read_csv(path, sep=sep, header=None)
    df.columns = df.iloc[0].str.strip().str.lstrip('#')
    return df.iloc[1:].reset_index(drop=True)

# ----------------------------------------------------------------------
def process_row(idx: int,
                row: pd.Series,
                target_faces: int) -> pd.Series:
    try:
        verts = np.array(eval(row['MeshVertices']), dtype=float)
        faces = np.array(eval(row['MeshNodes']),    dtype=int)

        mesh = simplify_mesh(verts, faces, target_faces, hole_close_size=1000)
        row['MeshVertices']      = mesh.vertices.tolist()
        row['MeshNodes']         = mesh.faces.tolist()
        row['MeshNormalVectors'] = mesh.face_normals.tolist()

        logger.info(f"[Row {idx}] '{row.get('ComponentName')}' → {len(mesh.faces)} faces")
    except Exception:
        logger.exception(f"[Row {idx}] Failed to process '{row.get('ComponentName')}'")
    return row

# ----------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser("Mesh simplification & repair pipeline")
    parser.add_argument("input_csv",  help="CSV with MeshVertices/MeshNodes")
    parser.add_argument("output_csv", help="Output CSV")
    parser.add_argument("--target_faces", "-t", type=int, default=10000,
                        help="Target number of faces")
    parser.add_argument("--sep", "-s", default=";",
                        help="CSV separator")
    args = parser.parse_args()

    logger.remove()
    logger.add(sys.stderr,
               format="{time:YYYY-MM-DD HH:mm:ss} | {level} | {message}",
               level="INFO")

    df = read_csv_db(args.input_csv, sep=args.sep)
    logger.info(f"Loaded {len(df)} rows from '{args.input_csv}'")

    out_rows = []
    for idx, row in df.iterrows():
        logger.info(f"Processing {idx+1}/{len(df)}…")
        out_rows.append(process_row(idx, row.copy(), args.target_faces))

    pd.DataFrame(out_rows).to_csv(args.output_csv,
                                  sep=args.sep,
                                  index=False,
                                  header=True)
    logger.info(f"Saved simplified CSV to '{args.output_csv}'")

if __name__ == "__main__":
    main()
