import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def explode(data):
    size = np.array(data.shape)*2
    data_e = np.zeros(size - 1, dtype=data.dtype)
    data_e[::2, ::2, ::2] = data
    return data_e

if __name__ == "__main__":

    cell_df = pd.read_csv('/home/g4rt/PO1/cp-0_dose3d_cell.csv')

    cell_df = cell_df.sort_values(by=['X [mm]', 'Y [mm]', 'Z [mm]'])


    x_values = cell_df['X [mm]'].unique()
    y_values = cell_df['Y [mm]'].unique()
    z_values = cell_df['Z [mm]'].unique()



    x_borders = np.concatenate([[x - 5.2, x + 5.2] for x in x_values])
    y_borders = np.concatenate([[y - 5.2, y + 5.2] for y in y_values])
    z_borders = np.concatenate([[z - 5.2, z + 5.2] for z in z_values])

    x_arr = x_borders.reshape(-1,8,2)
    y_arr = y_borders.reshape(-1,8,2)
    z_arr = z_borders.reshape(-1,8,2)

    print(type(x_values))

    for x in x_values:
        x_borders.append(x-5.2)
        x_borders.append(x+5.2)
    x_arr = np.array(x_borders).reshape(-1,8,2)

    for y in y_values:

        y_borders.append(y-5.2)
        y_borders.append(y+5.2)
    y_arr = np.array(y_borders).reshape(-1,8,2)

    for z in z_values:
        z_borders.append(z-5.2)
        z_borders.append(z+5.2)
    z_arr = np.array(z_borders).reshape(-1,8,2)

    val = np.ones((z_arr.shape[0]//2,z_arr.shape[1]//2,z_arr.shape[2]//2), dtype=bool)

    val = explode(val)

    print (x_arr)
    print (y_arr)
    print (z_arr)
    print (val)
    colors = np.empty(val.shape, dtype=object)
    colors[val] = 'red'


    ax = plt.figure().add_subplot(projection='3d')
    ax.voxels(x_arr, y_arr, z_arr ,val, facecolors=colors, edgecolor='k')

    plt.show()