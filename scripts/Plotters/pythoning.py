#!/usr/bin/env python

import argparse
from dataclasses import dataclass
import logging
from multiprocessing import Pool, cpu_count
import os
from pathlib import Path
from re import compile
import time

import numpy as np
import pandas as pd
from numerize_denumerize.numerize import numerize

import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib import cm
from matplotlib import colors

import yaml as yml

from mpl_toolkits.axes_grid1 import make_axes_locatable

import os.path
from textwrap import wrap


@dataclass
class SettingsDC:
    timestamp_freq = 125e6
    cal_sc_number = None
    cal_coerr = None

    feb = True
    rate = None

    # Values in milimeters
    voxel_side_len = 11
    voxel_xy_spacing = 0.9
    voxel_z_spacing = 0.9


settings = SettingsDC()


def maroc_channel_from_scid(scid, pmt_mapping):
    for ch, scid_pmt in enumerate(pmt_mapping):
        if scid_pmt == scid:
            return ch
    return -1


def explode(data):
    size = np.array(data.shape)*2
    data_e = np.zeros(size - 1, dtype=data.dtype)
    data_e[::2, ::2, ::2] = data
    return data_e


def voxels_from_file(fpath: Path):
    global settings

    # ------------------------------------
    # Getting the data from the given file
    # ------------------------------------
    with pd.HDFStore(fpath) as store:
        
        print(store)
        
        metadata = store.select("metadata", start=0, stop=1,
                                columns=["extNote", "sliceID", "startConfig"])
        ext_note_raw = metadata["extNote"].values[0]

        photon_counting = store.select("photon_counting")
    # ------------------------------------
    # /Getting the data from the given file
    # ------------------------------------

    # -------------------------------------
    # Strip and parse info for the mappings
    # -------------------------------------
    ext_note_raw = ext_note_raw.strip()
    ext_note_raw = ext_note_raw.strip("'\"")
    ext_note_raw = ext_note_raw.strip()
    ext_note_p = yml.load(ext_note_raw, yml.Loader)

    try:
        this_sliceID = metadata['sliceID'].values[0]
    except KeyError:
        print(f"This datafile has no 'sliceID' info in the data")
        print()

    try:
        slice_config = yml.load(metadata['startConfig'].values[0], yml.Loader)
        settings.feb = slice_config['fwSetup']['channelReverse']
    except KeyError:
        print(f"This datafile has no 'startConfig' info in the data")
        print()
    # -------------------------------------
    # /Strip and parse info for the mappings
    # -------------------------------------

    # ------------------
    # Getting the counts
    # ------------------
    # Get an array of _unique_ measurement IDs, this is needed to sum all the
    # measurement ids' counts together and count the summed rates.
    measurement_ids = np.unique(photon_counting["mID"].values)
    pc_summed_dt = 0
    pc_counts = np.zeros(64, dtype=photon_counting.dtypes["ch0"])

    for m_id in measurement_ids:
        m_id_photon_counting = photon_counting.query(f"mID == {m_id}")
        ts_rst = m_id_photon_counting["ts_rst"].values[0]
        ts_last_snap = m_id_photon_counting["ts_snap"].values[-1]
        pc_summed_dt += ts_last_snap - ts_rst

        # For each channel...
        for ch in range(64):
            # ... add the total value of counts for channel `ch` to the total
            pc_counts[ch] += m_id_photon_counting[f"ch{ch}"].values[-1]

    pc_summed_timespan_s = pc_summed_dt/settings.timestamp_freq

    # For TN1, the Maroc's channel mapping to PMT's channel mapping is not
    # inverted, i.e. channel 1 of the PMT is connected to the channel 0 of the
    # Maroc, etc. For FEB, the mapping is reversed, so the PMT's channel 1 is
    # Maroc's channel 63. For the `pc_counts` to reflect the PMT map it
    # needs to be reversed when using FEB data.
    if settings.feb:
        pc_data = pc_counts[::-1]

    pc_total_sum = np.sum(pc_data)

    if settings.rate:
        pc_data = pc_data/pc_summed_timespan_s
    # ------------------
    # /Getting the counts
    # ------------------

    # -------------------------------------------------
    # Getting the physical layouts of the scintillators
    # -------------------------------------------------
    # Get the HW serial numbers, this is important, as if it is a list, more
    # than one slice was used during the measurement. In such case, the layouts
    # (mappings) should follow the order of the serial numbers, i.e. for the
    # second serial number (a list of IP, and serial numbers of the PCBs), the
    # second PMT mapping and the second phantom layout should be valid.
    electronics_definition = ext_note_p["Hardware"]["Electronics"]
    if "SN" in electronics_definition:
        serial_numbers = ext_note_p["Hardware"]["Electronics"]["SN"]
    elif "Slices and configurations" in electronics_definition:
        serial_numbers = ext_note_p["Hardware"]["Electronics"]["Slices and configurations"]
    else:
        print(f"The serial number or the slice IDs are not properly defined "
              f"in the extended note")
        return

    if isinstance(serial_numbers, list) and len(serial_numbers) != 1:
        # The data file name contains the IP address from which the data has
        # been gathered, but the extended note contains the mappings for *all*
        # slices used in a given measurement. By identifying the IP address from
        # the file name, we can later find the appropriate mapping in the
        # extended note.
        board_ip_regex = compile(r"addr_10_10_10_(\d{2,3})")
        board_ip = board_ip_regex.search(fpath.name)[1]

        mapping_index = None
        for i, sn in enumerate(serial_numbers):
            if type(sn) is dict and this_sliceID in sn:
                mapping_index = i
                break
            if type(sn) is list and sn[0].endswith(f".{board_ip}"):
                mapping_index = i
                break

        # Choosing the mapping corresponding to the IP identified.
        pmt_mapping = ext_note_p["Hardware"]["Mechanics"]["Info"]["Cover"]["Hole mapping"][mapping_index]
        phantom_mapping = ext_note_p["Hardware"]["Phantom"]["Layout"][mapping_index]
    else:
        # Only one SN
        pmt_mapping = ext_note_p["Hardware"]["Mechanics"]["Info"]["Cover"]["Hole mapping"]
        phantom_mapping = ext_note_p["Hardware"]["Phantom"]["Layout"]
        # When noting the PMT mappings in multi-part phantoms, sometimes, the
        # mapping will be a 1-element list of 2-dimensional mapping, unpack it
        # then.
        if len(pmt_mapping) == 1:
            pmt_mapping = pmt_mapping[0]
        if len(phantom_mapping) == 1:
            phantom_mapping = phantom_mapping[0]

    # The whole phantom might have been rotated for the measurement, which does
    # not change its layout in and of itself, but the final plot should be
    # rotated accordingly.
    # If there is a key "Rotation" beside "Layout", create a rotation list,
    # which will be applied after converting the phantom mapping to numpy array.
    if "Rotation" in ext_note_p["Hardware"]["Phantom"].keys():
        if isinstance(serial_numbers, list) and len(serial_numbers) != 1:
            rotations = ext_note_p["Hardware"]["Phantom"]["Rotation"][mapping_index]
        else:
            rotations = ext_note_p["Hardware"]["Phantom"]["Rotation"][0]
    else:
        rotations = []
    # -------------------------------------------------
    # /Getting the physical layouts of the scintillators
    # -------------------------------------------------

    # ---------------------------------------------------
    # Getting the counts or rates for each cube in space.
    # ---------------------------------------------------
    phantom_mapping = np.array(phantom_mapping)
    for r in rotations:
        phantom_mapping = np.rot90(phantom_mapping, axes=r)
    phantom_pmt_channels = np.empty_like(phantom_mapping, dtype="int64")
    phantom_counts = np.empty_like(phantom_mapping, dtype="float")
    pmt_mapping = np.array(pmt_mapping).reshape(64)

    # Find the count or rate value for each cube in the phantom using the
    # mappings.
    max_cnt = np.max(np.array(phantom_counts))
    for x in range(phantom_counts.shape[0]):
        for y in range(phantom_counts.shape[1]):
            for z in range(phantom_counts.shape[2]):
                # Find the same scintillator number in the phantom and pmt mappings
                maroc_ch = maroc_channel_from_scid(phantom_mapping[x][y][z],
                                                   pmt_mapping)
                if settings.cal_sc_number is not None and phantom_mapping[x][y][z]:
                    cal_ch = np.nan_to_num(
                        settings.cal_coeff[settings.cal_sc_number == phantom_mapping[x][y][z]],
                        nan=1.0)[0]
                else:
                    cal_ch = 1

                # Save the index mapping and the counts found into the
                # corresponding spatial position corresponding to the
                # scintillator id.
                phantom_pmt_channels[x][y][z] = maroc_ch
                phantom_counts[x][y][z] = pc_data[maroc_ch]*cal_ch if maroc_ch > 0 else 0

    # ---------------------------------------------------
    # /Getting the counts or rates for each cube in space.
    # ---------------------------------------------------

    # -------------------------------------------------
    # Generating the voxels, their positions and colors
    # -------------------------------------------------
    # The truthy table which voxels to fill, this is now all the voxels in the
    # phantom. For the gaps to appear, we will need some empty (non-filled)
    # voxels in between.
    voxels = phantom_mapping != None
    voxels_with_empty = explode(voxels)

    phantom_counts_with_empty = np.zeros(voxels_with_empty.shape)
    phantom_counts_with_empty[::2, ::2, ::2] = phantom_counts

    # Creating the dimensionally accurate array of voxels with gaps in between
    x, y, z = (np.indices(np.array(voxels_with_empty.shape) + 1).astype(float) // 2)
    x *= settings.voxel_side_len + settings.voxel_xy_spacing
    y *= settings.voxel_side_len + settings.voxel_xy_spacing
    z *= settings.voxel_side_len + settings.voxel_z_spacing
    x[1::2, :, :] += settings.voxel_side_len
    y[:, 1::2, :] += settings.voxel_side_len
    z[:, :, 1::2] += settings.voxel_side_len
    # -------------------------------------------------
    # /Generating the voxels, their positions and colors
    # -------------------------------------------------

    return x, y, z, voxels_with_empty, phantom_counts_with_empty, phantom_mapping


def main():
    global settings

    logger = logging.getLogger()
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    sh = logging.StreamHandler()
    sh.setLevel(logging.INFO)
    sh.setFormatter(formatter)
    logger.addHandler(sh)

    # Set up arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('files',
                        type=Path,
                        nargs="*",
                        help="Paths to the data files, separated by whitespace.")
    parser.add_argument('--calibration-file',
                        type=Path,
                        default=None,
                        help="A path to the calibration file for the "
                             "scintillator cubes. The first row will be "
                             "ignored. It is assumed, that the second column "
                             "contains the scintillator number and the third "
                             "the calibration coefficient, by which the "
                             "count of the channel will be multiplied.")
    parser.add_argument('--max',
                        type=float,
                        default=0.,
                        help="Maximum value of the colorbar.")
    parser.add_argument('--phantom-offsets',
                        type=str,
                        nargs="*",
                        default=[],
                        help="Geometrical offsets for consecutive phantoms in the file "
                             "list, given in x,y,z format without spaces. Floating "
                             "point numbers are allowed.")
    parser.add_argument('--voxel-alpha',
                        type=float,
                        default=0.5,
                        help="Alpha value of the voxel colour")
    parser.add_argument('--text-alpha',
                        type=float,
                        default=0.5,
                        help="Alpha value of the text and edge colour")
    parser.add_argument('--text-bbox',
                        action="store_true",
                        help="Draw a bounding box around labels.")
    parser.add_argument('--lbl-indexes',
                        action="store_true",
                        help="If set, add cubes indexes to labels")
    parser.add_argument('--labels',
                        action="store_true",
                        help="If set, plot the labels")
    parser.add_argument('-f', '--feb',
                        action="store_true",
                        help="Adjust the channel mapping for BB-FEB.")
    parser.add_argument("-r", "--rate",
                        action="store_true",
                        help="Show data in rate [cps/s]")
    parser.add_argument("-n", "--normalize",
                        action="store_true",
                        help="Show data in normalized range [0-100%%]")
    parser.add_argument("-t", "--title",
                        action="store_true",
                        help="Show plot title")

    args = parser.parse_args()

    settings.timestamp_freq = 125e6
    settings.feb = args.feb
    settings.rate = args.rate

    if len(args.phantom_offsets) > 0 and len(args.phantom_offsets) != len(args.files):
        msg = f"""Given number of offsets for the phantoms does not match the number of
        files given. Please enter the proper count of the offsets, or let the program
        decide how to draw the phantoms."""
        raise Exception(msg)

    # ------------------------
    # Get the calibration data
    # ------------------------
    if args.calibration_file:
        settings.cal_sc_number, settings.cal_coeff \
            = np.genfromtxt(args.calibration_file, skip_header=1, usecols=(1, 2),
                            unpack=True, delimiter=",", missing_values=("NaN",),
                            filling_values=(1,))
    # ------------------------
    # /Get the calibration data
    # ------------------------

    # --------------------
    # Plotting the phantom
    # --------------------
    # Get all the data from the files beforehand to calculate proper maximums and
    # normalisations afterwards
    x_all = []
    y_all = []
    z_all = []
    xlim = [0, 0]
    ylim = [0, 0]
    zlim = [0, 0]
    voxels_all = []
    phantom_counts_all = []
    phantom_mapping_all = []

    max_pc_value = 0
    last_z = 0
    for n, file in enumerate(args.files):
        # Get the voxels' coordinates and counts from file
        x, y, z, voxels, phantom_counts, phantom_mapping = voxels_from_file(file)
        
        # print("My X:")
        print(x)
        # print("My Y:")
        print(y)
        # print("My Z:")
        print(z)
        # print("My Voxels:")
        print(voxels)
        # print("My Phantom Counts:")
        # print(phantom_counts)
        # print("My Phantom Mapping:")
        # print(phantom_mapping)
        

        # Calculate the global maximum count - to be used for further use in
        # `Normalize`.
        this_file_max_pc = np.max(phantom_counts)
        if max_pc_value < this_file_max_pc:
            max_pc_value = this_file_max_pc

        # Get the user-defined offsets or calculate our own
        if len(args.phantom_offsets) > 0:
            xo, yo, zo = args.phantom_offsets[n].split(",")
            x += float(xo)
            y += float(yo)
            z += float(zo)
        else:
            z += last_z + settings.voxel_z_spacing
            last_z = z[0][0][-1]

        # Append them to lists for further use (since the dimensions might be different
        # from phantom to phantom, numpy arrays won't cut it here).
        #
        # Also, manually save minimums and maximums for axes limits, voxel plot
        # autoscales to the last ax.voxel() call
        x_all.append(x)
        if xlim[0] > np.min(x):
            xlim[0] = np.min(x)
        if xlim[1] < np.max(x):
            xlim[1] = np.max(x)

        y_all.append(y)
        if ylim[0] > np.min(y):
            ylim[0] = np.min(y)
        if ylim[1] < np.max(y):
            ylim[1] = np.max(y)

        z_all.append(z)
        if zlim[0] > np.min(z):
            zlim[0] = np.min(z)
        if zlim[1] < np.max(z):
            zlim[1] = np.max(z)
        voxels_all.append(voxels)
        phantom_counts_all.append(phantom_counts)
        phantom_mapping_all.append(phantom_mapping)

    # Calculate our limits on the axes
    dx = 0.1*(xlim[1] - xlim[0])
    xlim[0] -= dx
    xlim[1] += dx
    dy = 0.1*(ylim[1] - ylim[0])
    ylim[0] -= dy
    ylim[1] += dy
    dz = 0.1*(zlim[1] - zlim[0])
    zlim[0] -= dz
    zlim[1] += dz

    # Define the axes, especially the one for the 3D plot.
    fig, axs = plt.subplots(1, 2, gridspec_kw={'width_ratios': [9, 1]})
    axs[0].remove()
    ax = fig.add_subplot(121, projection="3d")

    # Create normalization callable (for converting the counts or rates) to 0-1
    # range of values for coloring.
    if args.normalize:
        phantom_counts_norm = colors.Normalize(vmin=0)
    else:
        if args.max > 0:
            phantom_counts_norm = colors.Normalize(vmin=0, vmax=args.max)
        else:
            phantom_counts_norm = colors.Normalize(vmin=0, vmax=max_pc_value)
    # Define which colormap to use.
    phantom_counts_cmap = cm.magma


    # Re-iterate over all the data in the memory to plot them in defined places in
    # space.
    for n, (x, y, z, v, pc, pm) \
            in enumerate(zip(x_all, y_all, z_all, voxels_all, phantom_counts_all, phantom_mapping_all)):
        # -------------------
        # Plotting the voxels
        # -------------------
        if args.normalize:
            pc = pc/max_pc_value
        pc_normed = phantom_counts_norm(pc)

        # Iterate over the whole phantom and assign proper color depending on the
        # normalized value. Make the whole phantom translucent.
        pc_colored = np.empty((*pc.shape, 4))
        with np.nditer(pc_normed, flags=['multi_index']) as it:
            for el in it:
                pc_colored[it.multi_index] = phantom_counts_cmap(el, alpha=args.voxel_alpha)

        # Generate the edges
        edges = np.zeros((*v.shape, 4))
        edges[:, :, :, 0:3] = 0.2
        edges[:, :, :, 3] = args.text_alpha*0.5
        
        # print("My X:")
        # print(x.reshape(-1,1,2))
        # print("My Y:")
        # print(y.reshape(-1,1,2))
        # print("My Z:")
        # print(z.shape)
        # print("My Voxels:")
        # print(v.shape)

        

        ax.voxels(x, y, z, v, facecolors=pc_colored, edgecolors=edges, shade=False)
        # -------------------
        # /Plotting the voxels
        # -------------------

        # ------------------------------------------------------------
        # Plotting the text labels for scintillator numbers and counts
        # ------------------------------------------------------------
        if args.labels:
            for nx in range(pm.shape[0]):
                for ny in range(pm.shape[1]):
                    for nz in range(pm.shape[2]):
                        text_settings = {
                            "horizontalalignment": "center",
                            "verticalalignment": "center",
                            "weight": "bold",
                            "color": (0, 0.4, 0, args.text_alpha),
                            "zorder": 100,
                            "fontsize": 7,
                        }
                        if args.text_bbox:
                            text_settings["backgroundcolor"] = (1, 1, 1, args.text_alpha)

                        x_t = x[0][0][0] + (nx + 0.5)*(settings.voxel_side_len + settings.voxel_xy_spacing)
                        y_t = y[0][0][0] + (ny + 0.5)*(settings.voxel_side_len + settings.voxel_xy_spacing)
                        z_t = z[0][0][0] + (nz + 0.5)*(settings.voxel_side_len + settings.voxel_z_spacing)

                        label = f"SC#{pm[nx][ny][nz]}\n"
                        if args.normalize:
                            label += f"{(pc[::2, ::2, ::2])[nx][ny][nz]:3.0f}%"
                        else:
                            label += f"{numerize((pc[::2, ::2, ::2])[nx][ny][nz])}"

                        if args.lbl_indexes:
                            label += f"\n({nx},{ny},{nz})"

                        ax.text(x_t, y_t, z_t + 1.5*settings.voxel_z_spacing, label, **text_settings)
        # ------------------------------------------------------------
        # /Plotting the text labels for scintillator numbers and counts
        # ------------------------------------------------------------

    ax.set_xlim(*xlim)
    ax.set_ylim(*ylim)
    ax.set_zlim(*zlim)

    # ------------------------------
    # Auxilliary settings and labels
    # ------------------------------
    ax.set_xlabel("x [mm]")
    ax.set_ylabel("y [mm]")
    ax.set_zlabel("z [mm]", labelpad=100)
    # ax.tick_params(pad=30)
    ax.set_aspect('equal')
    # ------------------------------
    # /Auxilliary settings and labels
    # ------------------------------

    axc = axs[1]

    if args.normalize:
        fig.colorbar(cm.ScalarMappable(norm=phantom_counts_norm, cmap=phantom_counts_cmap),
                     cax=axc, orientation='vertical', label=f"normalized counts")
    elif args.rate:
        fig.colorbar(cm.ScalarMappable(norm=phantom_counts_norm, cmap=phantom_counts_cmap),
                     cax=axc, orientation='vertical', label=f"count rate [cps]")
    else:
        fig.colorbar(cm.ScalarMappable(norm=phantom_counts_norm, cmap=phantom_counts_cmap),
                     cax=axc, orientation='vertical', label=f"total count")

    if args.title:
        files = [f.name for f in args.files]
        title = "\n".join(wrap("\n".join(files), 80))
        plt.suptitle(title, fontsize=6)

    plt.subplots_adjust(
        top=0.9,
        bottom=0.13,
        left=0.0,
        right=0.86,
        hspace=0.2,
        wspace=0.2
    )
    plt.show()
    # --------------------
    # /Plotting the phantom
    # --------------------


if __name__ == "__main__":
    main()
