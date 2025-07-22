
import pandas as pd
import numpy as np
import math
from data_reader import normalize



def smoother(original_data, lvl, intensity):

    # The depth of fit parameter - tells you how much we increase the density of the array by, creating a smoothed fit. It has to be a multiple of four.
    deep_range = range(2*(lvl+1))
    deep_level = 2*(lvl+1)
    deep_rate = int((lvl+1))
    deep_margin = int((lvl+1)*4)

    data = original_data.to_numpy()
    un_smooth_data = np.empty([0, 2])
    denser_data = np.empty([0, 1])
    smooth_data = np.empty([0, 1])
    order_data = np.empty([0, 1])

    # Creating an array with more dense bins.
    for x in data:
        for n in deep_range:
            un_smooth_data = np.append(un_smooth_data, x)

    un_smooth_data = np.reshape(un_smooth_data, [deep_rate*(np.size(data)), 2])

    data_placer = range(deep_rate*(np.size(data)))

    # Destinationally, the following weighted geometric mean will also be in a separate method like "smooth_cumsum"
    # The level of data interference can be changed by changing the power at parameter b.
    # Testing different values - I found 6 to be the sweet spot - it improves the quality of the data and doesn't change it too drastically.
    a = un_smooth_data[0, 0]
    b = un_smooth_data[0, 1]
    delta_y = (un_smooth_data[deep_level, 0] - un_smooth_data[0, 0])/deep_level

    for x in data_placer:
        if (deep_rate * (np.size(data))) - deep_level > x > 1:
            denser_data = np.append(denser_data, denser_data[x-1] + ((un_smooth_data[x+deep_level, 1] - un_smooth_data[x+0, 1])/(deep_level+1)))
        else:
            denser_data = np.append(denser_data, un_smooth_data[x, 1])

    for x in data_placer:
        a = a + delta_y

        if ((deep_rate * (np.size(data))) - deep_margin) > x > deep_margin:

            if denser_data[x-deep_margin] > 0 and denser_data[x+deep_margin] > 0:

                b = math.pow((denser_data[x-deep_rate] * denser_data[x] * denser_data[x+deep_rate])*((denser_data[x-deep_margin] * denser_data[x-deep_level] * denser_data[x+deep_level] * denser_data[x+deep_margin])**intensity), 1/((4*intensity)+3))

            else:
                b = denser_data[x]

        order_data = np.append(order_data, a)
        smooth_data = np.append(smooth_data, b)


    smooth_data = smooth_cumsum(smooth_data, int(deep_rate/2) * int(intensity*2/3))

    smooth_frame = pd.DataFrame(smooth_data)
    order_frame = pd.DataFrame(order_data)
    smooth_frame = pd.concat([order_frame, smooth_frame], axis=1)
    smooth_frame.columns = ['position [cm]', 'X PROF Smooth']

    return normalize(smooth_frame, 'X PROF Smooth')

# Smoothing functions using the cumulative sum method.
# It smooths intensively, but affects the slope of the function, so I considered level five smoothing to be the sweet spot.


def smooth_cumsum(arr, span):

    cumsum_vec = np.cumsum(arr)
    moving_average = (cumsum_vec[2 * span:] - cumsum_vec[:-2 * span]) / (2 * span)

    front, back = [np.average(arr[:span])], []

    for i in range(1, span):
        front.append(np.average(arr[:i + span]))
        back.insert(0, np.average(arr[-i - span:]))
    back.insert(0, np.average(arr[-2 * span:]))

    return np.concatenate((front, moving_average, back))