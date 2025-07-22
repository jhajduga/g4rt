
from matplotlib import pyplot as plt
import pandas as pd
import numpy as np

def plot_mask_from_csv(path):
    df_0 = pd.read_csv(path)
    x_values = df_0['X [mm]']
    y_values = df_0['Y [mm]']
    fig = plt.figure()
    ax1 = fig.add_subplot(111)

    ax1.scatter(x_values, y_values, s=3, c='b', marker="s", label='field mask')
    plt.legend(loc='upper left')
    plt.show()

def plot_scoring_volume_mask_from_csv(mask_path_file, volume_mask_path,slice):
    df_mask = pd.read_csv(mask_path_file)
    mask_x_values = df_mask['X [mm]']
    mask_y_values = df_mask['Y [mm]']

    df_0 = pd.read_csv(volume_mask_path)
    # print(df_0)
    z_values = df_0['Z [mm]'].unique().tolist()
    z_values.sort()    
    df_1 = df_0[df_0['Z [mm]']==z_values[slice]]
    df_2 = df_1[df_1['MaskTag']==1]
    
    # print(df_2)
    
    x_values = df_1['X [mm]']
    y_values = df_1['Y [mm]']
    
    mx_values = df_1['mX [mm]']
    my_values = df_1['mY [mm]']
    
    mx_values_ifield = df_2['mX [mm]']
    my_values_ifield = df_2['mY [mm]']
    
    
    fig = plt.figure()
    ax1 = fig.add_subplot(111)

    ax1.scatter(mx_values,my_values, s=3, c='b', marker="o", label='volume at field mask plane ')
    ax1.scatter(mx_values_ifield, my_values_ifield, s=3, c='r', marker="o", label='volume mask (inField)')
    ax1.scatter(mask_x_values,mask_y_values, s=3, c='y', marker="o", label='field mask')
    plt.legend(loc='upper left')
    plt.show()
    
    

def plot_from_csv(path, slice, plane, observable="Dose"):

    df_0 = pd.read_csv(path)
    new_df_sorted = df_0.copy()
    new_df_sorted = new_df_sorted.sort_values(by = ['X [mm]', 'Y [mm]', 'Z [mm]'])
    new_df_sorted[observable] = new_df_sorted[observable]/new_df_sorted[observable].max()
    v = (new_df_sorted[observable].values).reshape(8,10,2)

    

    x_values = new_df_sorted['X [mm]'].unique()
    y_values = new_df_sorted['Y [mm]'].unique()
    z_values = new_df_sorted['Z [mm]'].unique()

    print('x values:', x_values, 'y values:', y_values, 'z values:', z_values)

    if(plane == "yz"):
        plane = v[slice, :, :].T
        # Extract Y, Z coordinates and dose values
        on_plot_x_coords = np.tile(y_values, len(z_values))
        on_plot_y_coords = np.repeat(z_values, len(y_values))

    elif(plane == "xz"):
        plane = v[:, slice, :].T
        # Extract X, Z coordinates and dose values
        on_plot_x_coords = np.tile(x_values, len(z_values))
        on_plot_y_coords = np.repeat(z_values, len(x_values))

    elif(plane == "xy"):
        plane = v[:, :, slice].T
        # Extract X, Y coordinates and dose values
        on_plot_x_coords = np.tile(x_values, len(y_values))
        on_plot_y_coords = np.repeat(y_values, len(x_values))

    dose_values = plane.flatten()

    plt.scatter(on_plot_x_coords, on_plot_y_coords, c=dose_values, cmap='viridis', s=25)
    plt.colorbar()
    plt.xlabel('Y [mm]')
    plt.ylabel('X [mm]')
    plt.title('YX Plane')
    plt.show()


if __name__=="__main__":
    job = 21
    
    job_name = "cp-2_dose3d_voxel.csv"
    # file = f"/home/g4rt/workDir/develop/g4rt/output/mlsr_4x4x4_10x10x10_flsz-ellipse_20x20mm_2e4_{job}/sim/{job_name}"
    file = f"/home/g4rt/workDir/develop/g4rt/output/tray_basic_setup_6/sim/cp-0_tray001_voxel.csv"
    plot_from_csv(file,0,"xy","Dose")
    # plot_from_csv(file,1,"xy","Dose")
    
    # mask_job_name = "cp-0_field_mask_sim.csv"
    # # mask_job_name = "cp-0_field_mask_plan.csv"
    # mask_file = f"/home/g4rt/workDir/develop/g4rt/output/mlsr_4x4x4_10x10x10_flsz-ellipse_20x20mm_2e4_{job}/sim/{mask_job_name}"
    # # # vol_job_name = "cp-0_scoring_volume_mask.csv"
    # # vol_job_name = "cp-0_scoring_volume_voxelised_mask.csv"
    # # vol_file_mask = f"/home/g4rt/workDir/develop/g4rt/output/mlsr_4x4x4_10x10x10_flsz-ellipse_20x20mm_2e4_{job}/{vol_job_name}"
    
    # plot_mask_from_csv(mask_file)
    
    