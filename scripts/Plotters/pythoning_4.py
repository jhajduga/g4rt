import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colors
import seaborn as sns
from matplotlib.ticker import LogLocator, NullFormatter
import matplotlib.pyplot as plt
import numpy as np


if __name__ == "__main__":
    
    sns.set_theme()
    
    cell_df = pd.read_csv('/home/g4rt/workDir/develop/g4rt/app/Test/multiple_tray_run_test_11/sim/cp-0_d3ddetector_cell.csv')
    cell_df = cell_df.sort_values(by=['X [mm]', 'Y [mm]', 'Z [mm]'])
    # cell_df['Dose'] = cell_df['Dose']/cell_df['Dose'].max()
    # sns.scatterplot(data=cell_df, x='X [mm]', y='Dose')
    grouped = cell_df.groupby('X [mm]').sum()
    grouped['Dose'] = grouped['Dose']/grouped['Dose'].max()
    sns.scatterplot(data=grouped, x='X [mm]', y='Dose')
    plt.yscale('log')
    # Enable minor ticks
    plt.minorticks_on()

    # Customize the grid to make it more dense
    plt.grid(True, which='both', linestyle='--', linewidth=0.2, color='gray')
    plt.show()
    
