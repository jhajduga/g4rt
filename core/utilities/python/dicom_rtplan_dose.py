import pydicom

# Martwa funkcja


def return_total_dose(dataFile):
    ds = pydicom.dcmread(dataFile)
    BeamCounter = ds[0x300a, 0x0070][0][0x300a, 0x0080].value
    total_dose = 0
    for k in range(0, BeamCounter):
        BeamDose = ds[0x300a, 0x0070][0][0x300c, 0x0004][k][0x300a, 0x0086].value
        total_dose = total_dose+BeamDose
    return total_dose

# Martwa funkcja


def return_beam_dose(dataFile, current_beam):
    ds = pydicom.dcmread(dataFile)
    BeamDose = ds[0x300a, 0x0070][0][0x300c, 0x0004][current_beam][0x300a, 0x0086].value
    return BeamDose


def return_dose_during_specified_controlpoint(dataFile, current_beam, current_controlpoint):
    ds = pydicom.dcmread(dataFile)
    BeamDose = ds[0x300a, 0x0070][0][0x300c, 0x0004][current_beam][0x300a, 0x0086].value
    if current_controlpoint == 0:
        controlpoint_dose = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x0134].value
        controlpoint_dose = (controlpoint_dose*BeamDose)
    else:
        cumulative_meterset_weight_in_previous_controlpoiny = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint-1][0x300a, 0x0134].value
        cumulative_meterset_weight_in_current_controlpoiny = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x0134].value
        controlpoint_dose = cumulative_meterset_weight_in_current_controlpoiny - cumulative_meterset_weight_in_previous_controlpoiny
        controlpoint_dose = (controlpoint_dose*BeamDose)
    return controlpoint_dose

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
