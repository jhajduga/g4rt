import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colors
from matplotlib import cm

phantom_counts_cmap = cm.magma

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def add_voxel(ax, x_center, y_center, z_center, voxel_side_len, dose, min, max):
        x = [x_center - voxel_side_len / 2, x_center + voxel_side_len / 2]
        y = [y_center - voxel_side_len / 2, y_center + voxel_side_len / 2]
        z = [z_center - voxel_side_len / 2, z_center + voxel_side_len / 2]
        xx, yy, zz = np.meshgrid(x, y, z, indexing='ij')
        
        phantom_counts_norm = colors.LogNorm(vmin=min, vmax=max)
        # phantom_counts_norm = colors.Normalize(vmin=min, vmax=max)
        pc_normalized = phantom_counts_norm(dose)
        pc_colored = np.empty((*pc_normalized.shape, 4))
        with np.nditer(pc_normalized, flags=['multi_index']) as it:
            for el in it:
                pc_colored[it.multi_index] = phantom_counts_cmap(el, alpha=((pc_normalized)))
        ax.voxels(xx, yy, zz, np.ones((1,1,1), dtype=bool), facecolors=pc_colored, edgecolor=None)

if __name__ == "__main__":
    cell_df = pd.read_csv('/home/g4rt/workDir/develop/g4rt/output/test_6/sim/cp-0_d3ddetector_voxel.csv')
    # cell_df = pd.read_csv('/mnt/c/Users/Jakub/Downloads/mlsr_4x4x4_10x10x10_flsz_snake_v2_cell.csv')
    cell_df = cell_df.sort_values(by=['X [mm]', 'Y [mm]', 'Z [mm]'])

    # voxel_side_len = 10.4
    voxel_side_len = 0.985

    fig = plt.figure(figsize=(16, 12))
    ax = fig.add_subplot(111, projection='3d')
    dose_max = cell_df['Dose'].max()/cell_df['Dose'].max()
    dose_min = cell_df[cell_df['Dose']>0]['Dose'].min()/cell_df['Dose'].max()

    for _, row in cell_df.iterrows():
        if row['Dose'] == 0:
            continue
        x_center = row['X [mm]']
        y_center = row['Y [mm]']
        z_center = row['Z [mm]']

        dose = row['Dose']/cell_df['Dose'].max()
        add_voxel(ax, x_center, y_center, z_center, voxel_side_len, dose, dose_min, dose_max)

    # Ustawienie limitów osi
    x_min, x_max = cell_df['X [mm]'].min() - voxel_side_len, cell_df['X [mm]'].max() + voxel_side_len
    y_min, y_max = cell_df['Y [mm]'].min() - voxel_side_len, cell_df['Y [mm]'].max() + voxel_side_len
    z_min, z_max = cell_df['Z [mm]'].min() - voxel_side_len, cell_df['Z [mm]'].max() + voxel_side_len

    ax.set_xlim([-26, 26])
    ax.set_ylim([-26, 26])
    ax.set_zlim([0, 50])

    x_scale = 52
    y_scale = 52
    z_scale = 52
    ax.set_box_aspect([1.2*x_scale, 1.2*y_scale, 1.2*z_scale])
    ax.set_xlabel("x [mm]", fontsize=18,labelpad=10)
    ax.set_ylabel("y [mm]", fontsize=18,labelpad=10)
    ax.set_zlabel("z [mm]", fontsize=18,labelpad=10)
    ax.tick_params(labelsize=20)
    plt.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.05)
    scalar_mappable = cm.ScalarMappable(norm=colors.LogNorm(vmin=dose_min/100, vmax=dose_max), cmap=phantom_counts_cmap)
    # scalar_mappable = cm.ScalarMappable(norm=colors.Normalize(vmin=dose_min, vmax=dose_max), cmap=phantom_counts_cmap)
    colorbar_axes = fig.add_axes([0.9, 0.1, 0.03, 0.8])  # Adjust the position as needed
    cbar = fig.colorbar(scalar_mappable, cax=colorbar_axes, shrink=1.0, fraction=0.1, pad=0)
    cbar.ax.tick_params(labelsize=20)
    plt.show()



