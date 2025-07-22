#!/usr/bin/env python3

import pandas as pd

import sys
sys.path.append("../scripts")

import matplotlib

matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

from data_reader import get_geant4_pdd_cvsdata, get_waterphantom_pdd_data, get_primo_pdd_data
from data_reader import get_waterphantom_xprof_data, get_geant4_xprof_cvsdata,get_primo_xprof_data
from smoothing_functions import smoother
from plotting_utilities import multiplot_gamma_index

if __name__ == "__main__":
    x_label = 'position [cm]'
    # flsz = "5x5"
    # flsz = "10x10"
    flsz = "20x20"
    # =============== WATER PHANTOM DATA ====================
    # X Profile
    # waterphantom_xprof_file = "../data/Results/NIO/XPROF_"+flsz+"_December2020_pomiar1.dat"
    # waterphantom_pdd_file = "../data/Results/NIO/PDD_"+flsz+"_December2020_pomiar1.dat"

    # y_water_phantom_xprof_label = "XPROF (NIO)"
    # y_water_phantom_pdd_label = "PDD (NIO)"

    # xprof_phantom = get_waterphantom_xprof_data(waterphantom_xprof_file,x_label,y_water_phantom_xprof_label)
    # pdd_phantom = get_waterphantom_pdd_data(waterphantom_pdd_file,x_label,y_water_phantom_pdd_label)

    # =============== PRIMO DATA ====================
    y_primo_pdd_label = "PDD (PRIMO)"
    y_primo_xprof_label = "XPROF (PRIMO)"
    primo_pdd_file = "../data/Results/PRIMO/PDD_"+flsz+"_February2021.dat"
    primo_xprof_file = "../data/Results/PRIMO/profileX_"+flsz+"_14mm.dat"
    xprof_primo = get_primo_xprof_data(primo_xprof_file, x_label, y_primo_xprof_label,12)
    pdd_primo = get_primo_pdd_data(primo_pdd_file, x_label, y_primo_pdd_label,7)


    # =============== GEANT4 DATA ====================
    g4_data_path = "../data/Results/Geant4RT/"
    y_g4_pdd_label = "PDD (Geant4RT)"
    y_g4_xprof_label = "XPROF (Geant4RT)"

    g4_xprof_file = g4_data_path+"dose1dx_"+flsz+"_10e9particles_14mm.csv"
    g4_pdd_file = g4_data_path+"dose1dz_"+flsz+"_10e9particles_14mm.csv"

    pdd_g4 = get_geant4_pdd_cvsdata(g4_pdd_file, x_label, y_g4_pdd_label)
    smooth_pdd_g4 = pd.DataFrame(smoother(pdd_g4, 80, 8))
    smooth_pdd_g4.columns = [x_label, y_g4_pdd_label]

    xprof_g4 = get_geant4_xprof_cvsdata(g4_xprof_file, x_label, y_g4_xprof_label)
    smooth_x_prof_g4 = pd.DataFrame(smoother(xprof_g4, 80, 8))
    smooth_x_prof_g4.columns = [x_label, y_g4_xprof_label]

    # merged = pd.merge_asof(xprof_g4, smooth_x_prof_g4, left_on='position [cm]', right_on='position [cm]')
    multiplot_gamma_index(xprof_primo, smooth_x_prof_g4)



    # =============== PLOT COMPILATION ====================

    xprof_frame = smooth_x_prof_g4.plot(x=x_label, y=y_g4_xprof_label)
    xprof_primo.plot(ax=xprof_frame,x=x_label, y=y_primo_xprof_label)
    #xprof_phantom.plot(ax=xprof_frame,x=x_label, y=y_water_phantom_xprof_label)

    pdd_frame = smooth_pdd_g4.plot(x=x_label, y=y_g4_pdd_label)
    pdd_primo.plot(ax=pdd_frame,x=x_label, y=y_primo_pdd_label)
    #pdd_phantom.plot(ax=pdd_frame,x=x_label, y=y_water_phantom_pdd_label)


    plt.show()
