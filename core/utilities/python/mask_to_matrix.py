from loguru import logger
import numpy as np
import matplotlib.pyplot as plt
import argparse
import os
import polars as pl

def mask_to_matrix(plan_file: str,
                   out_csv: str = None,
                   out_pickle: str = None,
                   out_png: str = None) -> pl.DataFrame:
    logger.info(f"Processing file: {plan_file}")

    # Read file lines
    try:
        with open(plan_file, 'r') as f:
            lines = f.read().splitlines()
        logger.debug(f"Read {len(lines)} lines")
    except Exception as e:
        logger.error(f"Cannot open {plan_file}: {e}")
        raise

    # Grid parameters
    x_min, x_max, x_step = -200.0, 200.0, 0.1
    x_positions = np.arange(x_min, x_max + x_step, x_step)

    # Locate MLC section
    idx = next((i for i, ln in enumerate(lines) if ln.strip().startswith('# MLC')), None)
    if idx is None:
        raise ValueError('No MLC section found')

    # Parse mask segments
    segments = []
    for ln in lines[idx+1:]:
        txt = ln.strip()
        if not txt or txt.startswith('#'):
            break
        y1, y2 = map(float, txt.split(','))
        segments.append(sorted([y1, y2]))
    logger.debug(f"Found {len(segments)} segments")

    # Build full matrix rows
    rows = []
    for start, end in segments:
        row = np.zeros_like(x_positions, dtype=np.uint8)
        if not np.isclose(start, end):
            i0 = int(round((start - x_min)/x_step))
            i1 = int(round((end   - x_min)/x_step))
            row[i0:i1+1] = 1
        rows.append(row)
    matrix = np.vstack(rows)

    # Trim columns to X in [-45.3, 45.3]
    col_mask = (x_positions >= -45.3) & (x_positions <= 45.3)
    x_trim = x_positions[col_mask]
    matrix = matrix[:, col_mask]

    # Trim to central 36 rows
    total_rows = matrix.shape[0]
    start_row = max((total_rows - 26)//2, 0)
    matrix = matrix[start_row:start_row+26, :]

    # Create Polars DataFrame
    df = pl.DataFrame(
        data=matrix.tolist(),
        schema=[f"{x:.1f}" for x in x_trim],
        orient='row'
    )
    df = df.with_columns([
        pl.col(col).cast(pl.Boolean) for col in df.columns
    ])

    # Integer version for exports
    df_int = df.select([
        pl.col(col).cast(pl.UInt8) for col in df.columns
    ])

    # Save CSV (no header)
    if out_csv:
        df_int.write_csv(out_csv, include_header=False)
        # df_int.write_ipc(out_csv+'out.feather', compression='lz4')
        logger.info(f"Saved CSV: {out_csv}")

    # Save NPY array
    if out_pickle:
        np.save(out_pickle, matrix)
        logger.info(f"Saved NPY: {out_pickle}")



    if out_png:
        nrows, ncols = matrix.shape
        total_h = nrows * 2.5   
        half_h = total_h / 2   

        fig = plt.figure(figsize=(6, 6))
        ax_mm = fig.add_axes([0.1, 0.1, 0.85, 0.85])
        ax_mm.patch.set_visible(False)


        for r in range(nrows):
            row = matrix[r]
            indices = np.where(row == 1)[0]
            x0 = x_trim[0]
            x1 = x_trim[indices[0]] if indices.size else x_trim[-1]
            y0_mm = r * 2.5 - half_h
            h_mm  = 2.5
            if indices.size:
                xs = x_trim[indices]
                # różowy środek
                ax_mm.add_patch(plt.Rectangle(
                    (xs[0], y0_mm), xs[-1] - xs[0], h_mm,
                    facecolor='pink', alpha=0.95, edgecolor=None, clip_on=False
                ))


        red_box = plt.Rectangle(
            (-32.5, -half_h),
            65.0, total_h,
            fill=False, edgecolor='red', linewidth=0.95, clip_on=False
        )
        ax_mm.add_patch(red_box)

        nums = [-23, -11, 1, 13]

        for a in nums:
            for b in nums:
                blue_box = plt.Rectangle(
                    (a, b),  #
                    10, 10,             
                    fill=False, edgecolor='blue',
                    linewidth=0.5, linestyle='--', clip_on=False
                )
                ax_mm.add_patch(blue_box)


        # 4) Limity osi na centr. ±32.5 mm
        ax_mm.set_xlim(x_trim[0] - 2.5, x_trim[-1] + 2.5)
        ax_mm.set_ylim(-half_h-2.5, half_h+2.5)

        # 5) Etykiety i proporcje
        ax_mm.set_xlabel('X position (mm)')
        ax_mm.set_ylabel('Y position (mm)')
        ax_mm.set_aspect('equal', anchor='C')

        # 6) Zapis do pliku
        fig.savefig(out_png, dpi=450, bbox_inches='tight')
        plt.close(fig)
        logger.info(f"Saved PNG: {out_png}")



    return df


def main():
    parser = argparse.ArgumentParser(
        description="Convert .dat mask files to trimmed mask matrices"
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-i', '--input_file', help='Single .dat file')
    group.add_argument('-d', '--input_dir',  help='Directory of .dat files')
    parser.add_argument('-o', '--out_dir',    required=True, help='Output folder')
    parser.add_argument('--out_csv',  action='store_true', help='Save CSV')
    parser.add_argument('--out_pickle', action='store_true', help='Save .npy')
    parser.add_argument('--out_png',  action='store_true', help='Save PNG')
    args = parser.parse_args()

    # Gather input files
    files = []
    if args.input_file:
        files.append(args.input_file)
    else:
        for root, _, fnames in os.walk(args.input_dir):
            for f in fnames:
                if f.lower().endswith('.dat'):
                    files.append(os.path.join(root, f))

    # Process each file
    for fpath in files:
        base = os.path.splitext(os.path.basename(fpath))[0]
        odir = os.path.join(args.out_dir, base)
        os.makedirs(odir, exist_ok=True)
        mask_to_matrix(
            fpath,
            out_csv=os.path.join(odir, base + '_mask.csv') if args.out_csv else None,
            out_pickle=os.path.join(odir, base + '_mask.npy') if args.out_pickle else None,
            out_png=os.path.join(odir, base + '_mask.png') if args.out_png else None
        )

if __name__ == '__main__':
    main()

# # mask\_to\_matrix.py Usage

# ## Example: Single file

#
# python mask_to_matrix.py \
#   -i /path/to/example.dat \
#   -o /path/to/output_dir \
#   --out_csv --out_pickle --out_png
#

# * `-i` / `--input_file`: path to a single `.dat` file
# * `-o` / `--out_dir`: directory where results will be saved
# * `--out_csv`, `--out_pickle`, `--out_png`: flags to enable saving in CSV, pickle, and PNG respectively

# ## Example: Entire directory

#
# python mask_to_matrix.py \
#   -d /path/to/dat_folder \
#   -o /path/to/output_dir \
#   --out_csv --out_pickle --out_png
#

# * `-d` / `--input_dir`: path to a directory; all `.dat` files in this directory and its subdirectories will be processed
# * Other options function as described above
