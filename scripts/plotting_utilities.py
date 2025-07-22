# Plot gamma index
# Plot: histogram różnic
# Parametr: Separation = średnia z modułu różnic
import numpy as np
import pandas as pd

iks = 'position [cm]'
igrek = 'XPROF'


def multiplot_gamma_index(ref_df, test_df, dta=0.3, dd=0.015):

    """
    A function to determine the matrix of local gamma indexes for two dose distributions.
    :param ref_dat: reference matrix in relative values
    :param test_dat: tested matrix in relative values
    :param dta: maximum distance-to-agreement
    :param dd: maximum dose difference
    """


    test_data = np.empty([0, 2])
    ref_data = np.empty([0, 2])


    if test_df['position [cm]'].values[-1] > ref_df['position [cm]'].values[-1]:
        frame = test_df
        to_match = ref_df
        merged = pd.merge_asof(to_match, frame, left_on='position [cm]', right_on='position [cm]')
    else:
        frame = ref_df
        to_match = test_df
        merged = pd.merge_asof(to_match, frame, left_on='position [cm]', right_on='position [cm]')

    selected_columns = merged[['position [cm]', 'XPROF (Geant4RT)']]
    ref_df = selected_columns.copy()
    ref_dat = ref_df.to_numpy()

    selected_columns = merged[['position [cm]', 'XPROF (PRIMO)']]
    test_df = selected_columns.copy()
    test_dat = test_df.to_numpy()

    # --------------------------------------------------------------------------------

    for x in test_dat:
        for n in range(10):
            test_data = np.append(test_data, x)

    test_data = np.reshape(test_data, [int(0.5 * np.size(test_data)), 2])

    a = test_data[0, 0]
    delta_y = (test_data[10, 0] - test_data[0, 0])/10

    denser_data = np.empty([0, 2])
    order_data = np.empty([0, 1])

    for x in range(int(0.5 * np.size(test_data))):
        a = a + delta_y
        order_data = np.append(order_data, a)
        if (int(0.5 * np.size(test_data))) - 10 > x > 1:
            denser_data = np.append(denser_data, denser_data[x-1] + ((test_data[x+10, 1] - test_data[x+0, 1])/(11)))
        else:
            denser_data = np.append(denser_data, test_data[x, 1])
    for x in range(int(0.5 * np.size(test_data))):
        test_data[x, 0] = order_data[x]
        test_data[x, 1] = denser_data[x]

    # --------------------------------------------------------------------------------

    for x in ref_dat:
        for n in range(10):
            ref_data = np.append(ref_data, x)

    ref_data = np.reshape(ref_data, [int(0.5 * np.size(ref_data)), 2])

    a = ref_data[0, 0]
    delta_y = (ref_data[10, 0] - ref_data[0, 0])/10

    denser_data = np.empty([0, 2])
    order_data = np.empty([0, 1])

    for x in range(int(0.5 * np.size(ref_data))):
        a = a + delta_y
        order_data = np.append(order_data, a)
        if (int(0.5 * np.size(ref_data))) - 10 > x > 1:
            denser_data = np.append(denser_data, denser_data[x-1] + ((ref_data[x+10, 1] - ref_data[x+0, 1])/(11)))
        else:
            denser_data = np.append(denser_data, ref_data[x, 1])
    for x in range(int(0.5 * np.size(ref_data))):
        ref_data[x, 0] = order_data[x]
        ref_data[x, 1] = denser_data[x]



    # Result matrix
    output = np.ndarray(ref_data.shape)

    for x in ref_data:
        do_something() #kod skończę wieczorem bo niestety ta część co miała liczyć gamma index u mnie dawałą w wyniku prostą o nachyleniu 30 stopni...




def multiplot_difference_histogram():
    a = 80


def separation_parameter():
    a = 90