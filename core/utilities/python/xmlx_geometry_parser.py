#!/usr/bin/env python3
from loguru import logger
import pandas as pd
import ast
import argparse
import sys
import csv

# Configure loguru: print INFO and above to stdout without extra formatting
logger.remove()
logger.add(lambda msg: print(msg, end=''), level="INFO")


def parse_list(field_name: str, text: str, row_idx: int):
    """
    Parse a Python list from a string; return empty list on error.
    field_name: column name for logging
    text: string content to parse
    row_idx: zero-based row index for logging
    """
    if not isinstance(text, str) or pd.isna(text):
        logger.warning(f"Row {row_idx+2}: missing '{field_name}' data")
        return []
    try:
        return ast.literal_eval(text)
    except Exception as e:
        logger.warning(f"Row {row_idx+2}: failed to parse '{field_name}': {e}")
        return []


def get_geometries(excel_file: str,
                   csv_file: str,
                   sheet_name=None,
                   csv_sep: str = ';'):
    """
    Load metadata from an Excel sheet and vertices from a CSV file, join by component.
    Returns list of dicts with keys: component, com, sc_id, vertices.
    """
    logger.info(f"Loading Excel metadata from: {excel_file}, sheet: '{sheet_name or 'first'}'\n")
    try:
        meta_df = pd.read_excel(
            excel_file,
            sheet_name=(0 if sheet_name is None else sheet_name),
            engine='openpyxl',
            dtype=str
        )
    except Exception as e:
        logger.error(f"Cannot read Excel file: {e}")
        raise

    logger.info(f"Loading CSV mesh data from: {csv_file} (sep='{csv_sep}')\n")
    try:
        mesh_df = pd.read_csv(
            csv_file,
            dtype=str,
            sep=csv_sep,
            engine='python',
            quoting=csv.QUOTE_NONE,
            on_bad_lines='warn'
        )
    except Exception as e:
        logger.error(f"Cannot read CSV file: {e}")
        raise

    # Normalize CSV column names: strip whitespace, leading '#', convert spaces to '_'
    mesh_df.columns = (
        mesh_df.columns
        .str.strip()
        .str.lstrip('#')
        .str.replace(' ', '', regex=False)
    )
    # Normalize Excel column names: strip whitespace and spaces to '_'
    meta_df.columns = (
        meta_df.columns
        .str.strip()
        .str.replace(' ', '', regex=False)
    )

    # Rename for consistency: ComponentName -> Component_Name
    if 'ComponentName' in meta_df.columns:
        meta_df.rename(columns={'ComponentName': 'Component_Name'}, inplace=True)
        meta_df.rename(columns={'BodyName': 'Body_Name'}, inplace=True)
    if 'ComponentName' in mesh_df.columns:
        mesh_df.rename(columns={'ComponentName': 'Component_Name'}, inplace=True)
        mesh_df.rename(columns={'BodyName': 'Body_Name'}, inplace=True)

    # Required metadata columns
    required_meta = ['Component_Name', 'Body_Name', 'Co_M_X', 'Co_M_Y', 'Co_M_Z', 'SC_id', 'Physical_Material']
    missing_meta = [c for c in required_meta if c not in meta_df.columns]
    if missing_meta:
        raise KeyError(f"Missing metadata columns in Excel: {missing_meta}")

    # Required mesh column
    required_mesh = ['Component_Name', 'MeshVertices', "MeshNodes", "MeshNormalVectors"]
    missing_mesh = [c for c in required_mesh if c not in mesh_df.columns]
    if missing_mesh:
        raise KeyError(f"Missing mesh columns in CSV: {missing_mesh}")

    # Merge on Component_Name
    mesh_df['Component_Name'] = mesh_df['Component_Name'].str.strip() + "::" + mesh_df['Body_Name'].str.strip()
    meta_df['Component_Name'] = meta_df['Component_Name'].str.strip() + "::" + meta_df['Body_Name'].str.strip()

    df = pd.merge(
        meta_df[required_meta],
        mesh_df[required_mesh],
        on='Component_Name',
        how='inner'
    )
    logger.info(f"Merged {len(df)} rows by 'Component_Name'\n")

    # Optional: column rename map for output (no-op by default)
    rename_map = {
        'Component_Name':       'Component_Name',
        'Body_Name':            'Body_Name',
        'Physical_Material':    'Physical_Material',
        'Co_M_X':               'Co_M_X',
        'Co_M_Y':               'Co_M_Y',
        'Co_M_Z':               'Co_M_Z',
        'SC_id':                'SC_id',
        'MeshVertices':         'MeshVertices',
        'MeshNodes':            'MeshNodes',
        'MeshNormalVectors':    'MeshNormalVectors'
    }
    df.rename(columns=rename_map, inplace=True)

    results = []
    for idx, row in df.iterrows():
        # Component identifier
        component = str(row['Component_Name']).strip()
        
        body = str(row['Body_Name']).strip()
        # Combine Co_M_X/Y/Z into a single vector 'com'
        try:
            com = [
                float(row['Co_M_X']),
                float(row['Co_M_Y']),
                float(row['Co_M_Z'])
            ]
        except Exception as e:
            logger.warning(f"Row {idx+2}: invalid COM values: {e}")
            continue
        # SC_id (empty string if missing)
        sc_raw = row.get('SC_id', '')
        sc_id = '' if pd.isna(sc_raw) or str(sc_raw).strip() == '' else str(sc_raw).strip()
        # Parse vertices list
        vertices = parse_list('MeshVertices', row['MeshVertices'], idx)
        nodes = parse_list('MeshNodes', row['MeshNodes'], idx)
        normals = parse_list('MeshNormalVectors', row['MeshNormalVectors'], idx)
        mat = str(row['Physical_Material']).strip()
        
        results.append({
            'component': component,
            'body':      body,
            'com':       com,
            'material':  mat,
            'sc_id':     sc_id,
            'vertices':  vertices,
            'nodes':     nodes,
            'normals':   normals            
        })

    logger.info(f"Parsed {len(results)} geometry entries\n")
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Parse geometry: Excel meta + CSV vertices -> dicts"
    )
    parser.add_argument('excel_file', help='Path to Excel metadata file')
    parser.add_argument('csv_file',   help='Path to CSV mesh file')
    parser.add_argument('--sheet',    default=None,
                        help='Sheet name or index (default: first)')
    parser.add_argument('--csv-sep',  default=';',
                        help="CSV separator (default: ';')")
    args = parser.parse_args()

    try:
        geoms = get_geometries(
            args.excel_file,
            args.csv_file,
            sheet_name=args.sheet,
            csv_sep=args.csv_sep
        )
        logger.info(f"Got {len(geoms)} geometries")
    except Exception:
        logger.exception("Error parsing geometries")
        sys.exit(1)
