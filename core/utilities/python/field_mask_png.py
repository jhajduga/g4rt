
from matplotlib import pyplot as plt
import pandas as pd
import os


def save_mask_as_png(path):
    
    plt.rcParams.update({'font.size': 25})
    
    df_0 = pd.read_csv(path)
    
    x_values = df_0['X [mm]']
    
    y_values = df_0['Y [mm]']
    
    fig = plt.figure(figsize=(24,24))
    
    ax1 = fig.add_subplot(111)
    
    ax1.scatter(x_values, y_values, s=30, c='b', marker='o', label='Field mask')
    
    plt.title('Field Mask')
    
    output = (os.path.splitext(path)[0]+'.png')
    
    plt.savefig(output, bbox_inches='tight')
