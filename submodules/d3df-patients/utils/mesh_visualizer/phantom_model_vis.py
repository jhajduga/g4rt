import os
import math
import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import trimesh
from trimesh.transformations import (
    translation_matrix,
    rotation_matrix,
    scale_matrix,
    euler_matrix
)
from trimesh.path.entities import Text as PathText
from trimesh.path.path import Path2D
from trimesh.creation import box, axis, extrude_polygon
from shapely.geometry import Polygon
from shapely.ops import unary_union
import shapely.affinity
import pyglet
from loguru import logger
import argparse
import freetype

# --- Logger Setup ---
logger.remove()
logger.add(
    "trimesh_visualiser.log",
    level="DEBUG",
    rotation="1 MB",
    backtrace=True,
    diagnose=True
)

def _handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return
    logger.opt(exception=(exc_type, exc_value, exc_traceback)).error("Uncaught exception")
sys.excepthook = _handle_exception

# --- Argument Parsing ---
def parse_args():
    parser = argparse.ArgumentParser(description="Visualize 3D dose data from D3DF database and simulation output.")
    parser.add_argument("--db", required=True, help="Path to .csv or .xlsx D3DF file")
    parser.add_argument("--sim", required=True, help="Simulation CSV with Label and Dose [Gy]")
    parser.add_argument("--output", default="scene.png", help="Output image filename")
    parser.add_argument("--create_voxel", action="store_true", help="Enable voxel shell visualization")
    parser.add_argument("--loglevel", default="ERROR", help="Console log level")
    return parser.parse_args()

# --- Console Logger ---
def configure_console_logger(level):
    logger.remove()
    logger.add(
        sys.stderr,
        format="<green>{time:HH:mm:ss}</green> | <level>{level:<8}</level> | {message}",
        level=level.upper(),
        colorize=True
    )

# --- Pyglet Setup ---
def configure_pyglet():
    pyglet.gl.glEnable(pyglet.gl.GL_BLEND)
    pyglet.gl.glBlendFunc(pyglet.gl.GL_SRC_ALPHA, pyglet.gl.GL_ONE_MINUS_SRC_ALPHA)
    pyglet.gl.glDisable(pyglet.gl.GL_CULL_FACE)

# --- Path Derivation ---
def derive_paths(db_path):
    base, ext = os.path.splitext(db_path)
    if ext.lower() == '.csv':
        return db_path, base + '.xlsx'
    return base + '.csv', db_path

# --- Load Mesh CSV ---
def load_mesh(csv_path):
    logger.debug(f"Loading mesh CSV from {csv_path}")
    df = pd.read_csv(csv_path, sep=';', header=None)
    df.columns = df.iloc[0].astype(str).str.strip().str.strip('#')
    df = df.drop(0).reset_index(drop=True)
    logger.info(f"Loaded {len(df)} mesh entries")
    return df

# --- Load Mapping & Simulation ---
def load_mapping_and_sim(xlsx_path, sim_path):
    logger.debug(f"Loading mapping Excel from {xlsx_path}")
    mapping = pd.read_excel(xlsx_path, sheet_name='scintillator_mapping_db')
    logger.info(f"Loaded mapping: {len(mapping)} rows")
    logger.debug(f"Loading simulation CSV from {sim_path}")
    sim = pd.read_csv(sim_path)
    logger.info(f"Loaded simulation data: {len(sim)} rows")
    return mapping, sim

# --- Color Utility ---
def get_rgba(val, vmin, vmax, cmap='viridis'):
    frac = np.clip((val - vmin) / (vmax - vmin), 0, 1)
    rgba = plt.get_cmap(cmap)(frac)
    return [int(255 * c) for c in rgba[:3]]

# --- Text Mesh Generation ---
def get_text_mesh(text, scale=0.0001 , height=1.0):
    font_path = 'TempOutput/Bookerly.ttf'
    face = freetype.Face(font_path)
    face.set_char_size(2048, 2048, 2048, 2048)

    meshes = []
    pen_x = 0.0

    for char in text:
        face.load_char(char)
        outline = face.glyph.outline
        points = np.array(outline.points, dtype=np.float64)
        if points.size == 0:
            continue

        start = 0
        contours = []
        for contour_end in outline.contours:
            contour = points[start:contour_end+1].copy()
            contour[:, 0] += pen_x
            if not np.allclose(contour[0], contour[-1]):
                contour = np.vstack([contour, contour[0]])
            contours.append(contour)
            start = contour_end + 1

        classified = []
        for c in contours:
            poly = Polygon(c)
            if poly.is_valid and poly.area > 0:
                orientation = poly.exterior.is_ccw
                classified.append((poly, orientation))

        # Extract shells and holes
        shells = [poly for poly, orient in classified if not orient]
        holes = [poly for poly, orient in classified if orient]

        # Attach each hole to the smallest shell that contains it
        for shell in shells:
            shell_prepared = shapely.prepared.prep(shell)
            assigned_holes = []
            for hole in holes:
                if shell_prepared.contains(hole):
                    assigned_holes.append(hole.exterior.coords)
            try:
                poly = Polygon(shell.exterior.coords, holes=assigned_holes)
                if not poly.is_valid:
                    continue
                poly = shapely.affinity.scale(poly, xfact=scale, yfact=scale, zfact=scale, origin=(0, 0, 0))
                mesh = extrude_polygon(poly, height=height)
                rot_matrix = rotation_matrix(np.radians(90), [1, 0, 0])
                trans_mat = translation_matrix([0, 0, -4])
                transform = trans_mat.dot(rot_matrix)
                mesh.apply_transform(transform)
                meshes.append(mesh)
            except Exception as e:
                logger.warning(f"Failed to extrude character '{char}': {e}")

        pen_x += face.glyph.advance.x

    if not meshes:
        return None

    return trimesh.util.concatenate(meshes)


# --- Colorbar ---
def add_colorbar(scene, vmin, vmax, steps=21):
    logger.debug("Adding colorbar to scene")
    height = 180
    dh = height / steps
    for i in range(steps):
        val = 0 + (i/(steps-1))*(vmax)
        val_text = 100 * val / vmax
        color = get_rgba(val, vmin, vmax)
        bar = box(extents=[10,10,dh])
        bar.unmerge_vertices()
        bar.visual.face_colors = np.tile(color + [255], (len(bar.faces),1))
        bar.apply_translation([200,0,i*dh])
        scene.add_geometry(bar)

        try:
            text_mesh = get_text_mesh(f"{val_text:2.0f} %", height=2.0)
            if text_mesh:
                text_mesh.apply_translation([207, 0, i*dh + dh/4])
                scene.add_geometry(text_mesh)
        except Exception as e:
            logger.warning(f"Text label generation failed at z={i*dh}: {e}")

    logger.debug("Colorbar added")

# --- Build Scene ---
def build_scene(df_mesh, mapping, sim, create_voxel):
    logger.info("Building scene geometry")
    scene = trimesh.Scene()
    vmin, vmax = sim['Dose [Gy]'].min(), sim['Dose [Gy]'].max()
    logger.debug(f"Dose range: {vmin} to {vmax}")
    add_colorbar(scene, vmin, vmax)
    ax = axis(axis_length=100.0, axis_radius=1)
    ax.unmerge_vertices()
    scene.add_geometry(ax)
    for idx, row in df_mesh.iterrows():
        comp, body = row['ComponentName'], row['BodyName']
        mesh = trimesh.Trimesh(
            faces=np.array(eval(row['MeshNodes'])),
            vertices=np.array(eval(row['MeshVertices'])),
            face_normals=np.array(eval(row['MeshNormalVectors']))
        )
        mesh.unmerge_vertices()
        name = f"{comp}|{body}"
        if 'D3DF_SC_Cell_Body' in name:
            sc = mapping[mapping['Component_Name']==comp].iloc[0]
            hits = sim[sim['Label'].str.contains(str(sc['SC_id']), na=False)]['Dose [Gy]']
            dose = hits.mean() if not hits.empty else sc['TotalCounts']
            logger.debug(f"Mesh {name}: dose={dose}")
            mesh.visual.face_colors = get_rgba(dose, vmin, vmax) + [255]
        else:
            mesh.visual.face_colors = [55,185,75,15]
        scene.add_geometry(mesh)
    logger.info("Scene geometry built")
    return scene

# --- Smart Camera ---
def setup_camera(scene):
    logger.info("Setting up smart camera")
    all_bounds = np.vstack([geom.bounds for geom in scene.geometry.values()])
    pts = all_bounds.reshape(-1,3)
    center = pts.mean(axis=0)
    radius = np.linalg.norm(pts.max(axis=0)-pts.min(axis=0))/2
    distance = radius * 2.0
    scene.camera.fov = (45,45)
    scene.camera_transform = scene.camera.look_at([center], distance=distance)
    logger.debug(f"Camera centered at {center}, distance {distance}")
    return scene

# --- Render & Save ---
def render_and_save(scene, output, vmin, vmax):
    img_bytes = scene.save_image(resolution=[2400,2000])
    from PIL import Image
    import io
    scene_im = Image.open(io.BytesIO(img_bytes)).convert('RGBA')

    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig = plt.figure(figsize=(1, 8))
    ax = fig.add_axes([0.25, 0.05, 0.5, 0.9])
    norm = plt.Normalize(vmin=vmin, vmax=vmax)
    sm = plt.cm.ScalarMappable(cmap='viridis', norm=norm)
    sm._A = []
    cbar = plt.colorbar(sm, cax=ax)
    cbar.set_label('Dose [Gy]')
    buf = io.BytesIO()
    fig.savefig(buf, format='png', dpi=100, bbox_inches='tight', pad_inches=0.1)
    plt.close(fig)
    buf.seek(0)
    bar_im = Image.open(buf).convert('RGBA')

    sw, sh = scene_im.size
    bw, bh = bar_im.size
    bar_im = bar_im.resize((int(bw * sh / bh), sh), Image.LANCZOS)
    out = Image.new('RGBA', (sw + bar_im.width, sh), (0,0,0,0))
    out.paste(scene_im, (0,0))
    out.paste(bar_im, (sw,0), bar_im)

    from PIL import ImageDraw, ImageFont
    draw = ImageDraw.Draw(out)
    try:
        font = ImageFont.truetype('arial.ttf', 16)
    except:
        font = ImageFont.load_default()
    text = f"Min: {vmin:.2e}   Max: {vmax:.2e}"
    draw.text((10, sh-30), text, fill=(255,255,255,255), font=font)

    out.convert('RGB').save(output)
    logger.info(f"Scene with colorbar saved to {output}")

# --- Main Flow ---
def main():
    args = parse_args()
    configure_console_logger(args.loglevel)
    configure_pyglet()
    csv_path, xlsx_path = derive_paths(args.db)
    df_mesh = load_mesh(csv_path)
    mapping, sim_df = load_mapping_and_sim(xlsx_path, args.sim)
    vmin, vmax = sim_df['Dose [Gy]'].min(), sim_df['Dose [Gy]'].max()
    scene = build_scene(df_mesh, mapping, sim_df, args.create_voxel)
    scene = setup_camera(scene)
    logger.info("Displaying interactive scene. Close window to proceed to saving.")
    scene.show()
    render_and_save(scene, args.output, vmin, vmax)

if __name__ == '__main__':
    main()
