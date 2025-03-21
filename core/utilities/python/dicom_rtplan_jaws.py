import pydicom
import numpy as np


def return_position(dataFile, current_beam, current_controlpoint, n_o_jaw):
    ds = pydicom.dcmread(dataFile)
    position = 0.0
    cp = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint]
    if len(cp[0x300a, 0x011a].value._list) == 3:
        if n_o_jaw == 0:
            flag = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][0][0x300a, 0x011a][0][0x300a, 0x011c].value
            JawXPositions = np.zeros(2, np.single)
            JawXPositions[n_o_jaw] = flag.pop(0)
            position = JawXPositions[n_o_jaw]
        if n_o_jaw == 1:
            flag = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][0][0x300a, 0x011a][0][0x300a, 0x011c].value
            JawXPositions = np.zeros(2, np.single)
            JawXPositions[n_o_jaw] = flag.pop(1)
            position = JawXPositions[n_o_jaw]
        if n_o_jaw == 2:
            flag = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][0][0x300a, 0x011a][1][0x300a, 0x011c].value
            JawYPositions = np.zeros(2, np.single)
            JawYPositions[n_o_jaw-2] = flag.pop(0)
            position = JawYPositions[n_o_jaw-2]
        if n_o_jaw == 3:
            flag = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][0][0x300a, 0x011a][1][0x300a, 0x011c].value
            JawYPositions = np.zeros(2, np.single)
            JawYPositions[n_o_jaw-2] = flag.pop(1)
            position = JawYPositions[n_o_jaw-2]
    return position


def return_number_of_beams(dataFile):
    ds = pydicom.dcmread(dataFile)
    BeamCounter = ds[0x300a, 0x0070][0][0x300a, 0x0080].value
    return BeamCounter


def return_number_of_controlpoints(dataFile, number_of_beams):
    ds = pydicom.dcmread(dataFile)
    ControlPoints = 0
    for i in range(0, number_of_beams):
        ControlPoints += ds[0x300a, 0x00b0][i][0x300a, 0x0110].value
    return ControlPoints


def return_number_of_jaws(dataFile):
    return 4
