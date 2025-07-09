from loguru import logger
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import argparse
import os


def mask_to_matrix(plan_file, out_csv=None, out_pickle=None, out_png=None):
    logger.info(f"Starting mask_to_matrix for file: {plan_file}")

    x_min, x_max, x_step = -200.0, 200.0, 0.1
    logger.debug(f"x range: [{x_min}, {x_max}] with step {x_step}")
    x_grid = np.arange(x_min, x_max + x_step, x_step)
    n_cols = len(x_grid)
    logger.debug(f"Grid has {n_cols} columns")
    mlc_rows = []

    try:
        with open(plan_file, 'r') as f:
            lines = f.readlines()
        logger.debug(f"Read {len(lines)} lines from file")
    except FileNotFoundError:
        logger.error(f"File not found: {plan_file}")
        raise

    mlc_start = None
    for i, line in enumerate(lines):
        if line.strip().startswith("# MLC"):
            mlc_start = i + 1
            logger.info(f"Found MLC section at line {i + 1}")
            break
    if mlc_start is None:
        logger.error("No MLC section found in file.")
        raise ValueError("No MLC section found in file.")

    for j, line in enumerate(lines[mlc_start:]):
        if line.strip().startswith("#") or not line.strip():
            logger.debug(f"Stopping parse at line {mlc_start + j}")
            break
        y1_str, y2_str = line.strip().split(",")
        y1, y2 = float(y1_str), float(y2_str)
        start, end = sorted([y1, y2])
        row = np.zeros(n_cols, dtype=int)
        if np.isclose(start, end):
            # zero-length segment: leave row as all zeros
            logger.debug(f"Zero-length segment at {start}, row remains zeros")
        else:
            idx_start = int(round((start - x_min) / x_step))
            idx_end = int(round((end - x_min) / x_step))
            row[idx_start:idx_end + 1] = 1
        mlc_rows.append(row)
    logger.info(f"Parsed {len(mlc_rows)} MLC rows")

    matrix = np.vstack(mlc_rows)
    df = pd.DataFrame(matrix, columns=np.round(x_grid, 1))
    logger.debug(f"Resulting matrix shape: {df.shape}")

    if out_csv:
        df.to_csv(out_csv, index=False)
        logger.info(f"Saved mask matrix to CSV: {out_csv}")

    if out_pickle:
        df.to_pickle(out_pickle)
        logger.info(f"Saved mask matrix to pickle: {out_pickle}")

    if out_png:
        logger.info(f"Saving mask points plot to PNG: {out_png}")
        fig, ax = plt.subplots()
        rows, cols = matrix.nonzero()
        xs = x_grid[cols]
        ys = rows
        ax.scatter(xs, ys, s=1)
        ax.set_xlabel('X position')
        ax.set_ylabel('Row index')
        ax.set_title('MLC Mask Points')
        fig.tight_layout()
        fig.savefig(out_png, dpi=300)
        plt.close(fig)
        logger.info("PNG plot saved successfully")

    logger.info("mask_to_matrix completed")
    return df


def main():
    parser = argparse.ArgumentParser(description="Convert plan file(s) to MLC matrix format and save outputs")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-i", "--input_file", help="Path to a single input plan file (.dat)")
    group.add_argument("-d", "--input_dir", help="Path to a directory of plan files (.dat)")
    parser.add_argument("-o", "--out_dir", required=True, help="Base directory to save outputs")
    parser.add_argument("--out_csv", action="store_true", help="Save output as CSV")
    parser.add_argument("--out_pickle", action="store_true", help="Save output as pickle")
    parser.add_argument("--out_png", action="store_true", help="Save output as PNG plot")

    args = parser.parse_args()

    # Collect files to process
    if args.input_file:
        files = [args.input_file]
    else:
        files = []
        for root, dirs, files_in_dir in os.walk(args.input_dir):
            for fname in files_in_dir:
                if fname.lower().endswith('.dat'):
                    files.append(os.path.join(root, fname))

    if not files:
        logger.error("No .dat files found to process.")
        return

    # Process each file
    for plan_file in files:
        name = os.path.splitext(os.path.basename(plan_file))[0]
        dest_dir = os.path.join(args.out_dir, name)
        os.makedirs(dest_dir, exist_ok=True)

        csv_path = os.path.join(dest_dir, f"{name}.csv") if args.out_csv else None
        pkl_path = os.path.join(dest_dir, f"{name}.pkl") if args.out_pickle else None
        png_path = os.path.join(dest_dir, f"{name}.png") if args.out_png else None

        logger.info(f"Processing {plan_file}, saving to {dest_dir}")
        mask_to_matrix(
            plan_file=plan_file,
            out_csv=csv_path,
            out_pickle=pkl_path,
            out_png=png_path
        )

if __name__ == "__main__":
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
