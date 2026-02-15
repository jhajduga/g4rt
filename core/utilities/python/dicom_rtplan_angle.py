import pydicom

def return_angle_in_controlpoint(dataFile, current_beam, current_controlpoint):
    ds = pydicom.dcmread(dataFile)
    if current_controlpoint == 0:
        controlpoint_angle = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x011e].value
        return controlpoint_angle
    else:
        string = "Ten przypadek będzie obsłużony później - dla sekfencji run'ów ze strony C++"
        return string

# Poniższe funkcje się powtarzają mniej więcej (minimalne zmiany) więc przeniosę je do osobnefo modułu - zredukuje to kod w C++


def return_number_of_beams(dataFile):
    ds = pydicom.dcmread(dataFile)
    BeamCounter = ds[0x300a, 0x0070][0][0x300a, 0x0080].value
    return BeamCounter


def return_number_of_controlpoints(dataFile, number_of_beams):
    ds = pydicom.dcmread(dataFile)
    ControlPointsMaxValue = 0
    for i in range(0, number_of_beams):
        if (ds[0x300a, 0x00b0][i][0x300a, 0x0110].value) > ControlPointsMaxValue:
            ControlPointsMaxValue = ds[0x300a, 0x00b0][i][0x300a, 0x0110].value
        else:
            continue
    return ControlPointsMaxValue
