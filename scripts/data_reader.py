#!/usr/bin/env python3

import pandas as pd

def normalize(df, column):
    df_norm = df.copy() # copy the dataframe
    # apply min-max scaling
    df_norm[column] = (df_norm[column] - df_norm[column].min())*100 / (df_norm[column].max() - df_norm[column].min())
    return df_norm


def get_geant4_pdd_cvsdata(file, x_label, y_label):
    separator = ','
    skip_rows = 7
    skip_footer = 0
    colX = 5
    colY = 6
    df = pd.read_csv(file, sep=separator, header=None, skiprows=skip_rows, skipfooter=skip_footer, engine='python')
    data = df.iloc[:, [colX, colY]]
    data.columns = [x_label, y_label]
    return normalize(data, y_label)


def get_geant4_xprof_cvsdata(file, x_label, y_label):
    separator = ','
    skip_rows = 7
    skip_footer = 0
    colX = 3
    colY = 6
    df = pd.read_csv(file,sep=separator,header=None,skiprows=skip_rows,skipfooter=skip_footer,engine='python')
    data = df.iloc[:,[colX,colY]]
    data.columns=[x_label,y_label]
    return normalize(data,y_label)


def get_waterphantom_pdd_data(file, x_label, y_label):
    separator = '   '
    skip_rows = 16
    skip_footer = 2
    df = pd.read_csv(file,sep=separator,header=None,skiprows=skip_rows,skipfooter=skip_footer,engine='python')
    data=df.iloc[:,[2,3]]
    data.columns=[x_label,y_label]
    return normalize(data,y_label)


def get_waterphantom_xprof_data(file, x_label, y_label):
    separator = '   '
    skip_rows = 16
    skip_footer = 2
    df = pd.read_csv(file,sep=separator,header=None,skiprows=skip_rows,skipfooter=skip_footer,engine='python')
    data=df.iloc[:, [0, 3]]
    data.columns=[x_label,y_label]
    return normalize(data,y_label)

def get_primo_pdd_data(file, x_label, y_label, skip_rows):
    separator = '    '
    skip_footer = 2
    df = pd.read_csv(file,sep=separator,header=None,skiprows=skip_rows,skipfooter=skip_footer,engine='python')
    data=df.iloc[:,[2,3]]
    data.columns=[x_label,y_label]
    return normalize(data,y_label)


def get_primo_xprof_data(file, x_label, y_label, skip_rows):
    separator = '    '
    skip_footer = 2
    df = pd.read_csv(file,sep=separator,header=None,skiprows=skip_rows,skipfooter=skip_footer,engine='python')
    data=df.iloc[:,[0,3]]
    data.columns=[x_label,y_label]
    return normalize(data,y_label)
