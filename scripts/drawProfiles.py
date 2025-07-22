#!/usr/bin/env python3

import pandas as pd

import sys
sys.path.append("../scripts")

import matplotlib

matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

from data_reader import get_geant4_pdd_cvsdata, get_waterphantom_pdd_data
from data_reader import get_waterphantom_xprof_data, get_geant4_xprof_cvsdata
from smoothing_functions import smoother

if __name__ == "__main__":
    x_label = 'position [cm]'

    # =============== WATER PHANTOM DATA ====================
    # X Profile
    waterphantom_xprof_file = "../data/Real/WaterTank-675x645x560mm/030x030_XPROF.dat"
    y_water_phantom_xprof_label = 'X PROF (Water phantom)'
    xprof_data_phantom = get_waterphantom_xprof_data(waterphantom_xprof_file,x_label,y_water_phantom_xprof_label)

    # PDD Profile
    waterphantom_pdd_file = "../data/Real/WaterTank-675x645x560mm/030x030_PDD.dat"
    y_water_phantom_label = 'PDD (Water phantom)'
    pdd_data_phantom = get_waterphantom_pdd_data(waterphantom_pdd_file,x_label,y_water_phantom_label)


    # =============== GEANT4 DATA ====================
    # X Profile
    y_g4_xprof_label = 'XPROF (Geant4RT)'
    g4_xprof_file = "../data/Geant4/dose1dx_5x5.csv"
    xprof_g4 = get_geant4_xprof_cvsdata(g4_xprof_file, x_label, y_g4_xprof_label)

    # PDD Profile
    y_g4_pdd_label = 'PDD (Geant4RT)'
    g4_pdd_file = "../data/Geant4/dose1dz_5x5.csv"
    pdd_g4 = get_geant4_pdd_cvsdata(g4_pdd_file, x_label, y_g4_pdd_label)

    # =============== GEANT4 SMOOTH OUT DATASET ====================
    y_g4_smooth_pdd_label = 'PDD Smooth (Geant4RT)'
    y_g4_smooth_xprof_label = 'X PROF Smooth (Geant4RT)'
    # Smoother function:
    # First variable - pandas dataframe to be smoothed.
    # Second variable - level of increase in density of dataframe in range 1-200
    # Third variable - level of smoothing in range 1-20
    temp_smooth_x_prof_g4 = smoother(xprof_g4, 80, 8)
    smooth_x_prof_g4 = pd.DataFrame(temp_smooth_x_prof_g4)
    smooth_x_prof_g4.columns = ['position [cm]', 'X PROF Smooth (Geant4RT)']

    temp_smooth_pdd_g4 = smoother(pdd_g4, 80, 8)
    smooth_pdd_g4 = pd.DataFrame(temp_smooth_pdd_g4)
    smooth_pdd_g4.columns = ['position [cm]', 'PDD Smooth (Geant4RT)']

    # =============== PLOTTING SECTION ====================
    # ---------------    X Profiles    --------------------
    xprof_frame = xprof_data_phantom.plot(x=x_label,y=y_water_phantom_xprof_label)
    # xprof_data_phantom.plot(x=x_label,y=y_water_phantom_xprof_label)

    xprof_g4.plot(ax=xprof_frame,x=x_label,y=y_g4_xprof_label)
    # xprof_g4.plot(x=x_label, y=y_g4_xprof_label)

    smooth_x_prof_g4.plot(ax=xprof_frame, x=x_label, y=y_g4_smooth_xprof_label)
    # smooth_x_prof_g4.plot(x=x_label, y=y_g4_smooth_xprof_label)

    # ---------------    PDD Profiles  --------------------
    pdd_frame = pdd_data_phantom.plot(x=x_label,y=y_water_phantom_label)
    # pdd_data_phantom.plot(x=x_label,y=y_water_phantom_label)

    pdd_g4.plot(ax=pdd_frame,x=x_label,y=y_g4_pdd_label)
    # pdd_g4.plot(x=x_label, y=y_g4_pdd_label)

    smooth_pdd_g4.plot(ax=pdd_frame, x=x_label, y=y_g4_smooth_pdd_label)
    # smooth_pdd_g4.plot(x=x_label, y=y_g4_smooth_pdd_label)

    plt.show()