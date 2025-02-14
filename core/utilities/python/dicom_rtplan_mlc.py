import pydicom
import numpy as np

def return_possition(dataFile, side, current_beam, current_controlpoint, num_of_leaves):
    # Wczytanie pliku DICOM
    try:
        ds = pydicom.dcmread(dataFile)
    except Exception as e:
        raise ValueError(f"Nie udało się odczytać pliku {dataFile}: {e}")

    # Odczyt typu wiązki
    try:
        beam_type = ds[0x300a, 0x00b0][current_beam][0x300A, 0x00C4].value
        if isinstance(beam_type, str):
            beam_type = beam_type.strip()
    except Exception as e:
        raise ValueError(f"Błąd przy odczycie typu wiązki dla wiązki {current_beam}: {e}")

    if beam_type.upper() == "STATIC" and current_controlpoint != 0:
        raise ValueError("Dla planu statycznego używamy tylko pierwszego punktu kontrolnego (cp_index=0).")

    # Odczyt punktu kontrolnego i pozycji MLC
    try:
        cp = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint]
        # Zakładamy, że indeks 2 w sekwencji odpowiada pozycjom MLC
        if cp[0x300a, 0x011a].value._list == 3:
            mlc_seq = cp[0x300a, 0x011a][2]
        if cp[0x300a, 0x011a].value._list == 1:
            mlc_seq = cp[0x300a, 0x011a][0]
        mlc_positions_list = mlc_seq[0x300a, 0x011c].value
        mlc_positions = np.array(mlc_positions_list, dtype=np.single)
    except Exception as e:
        raise ValueError(f"Błąd przy odczycie pozycji MLC: {e}")

    # Rozdzielenie pozycji na dwa banki
    if mlc_positions.size == num_of_leaves * 2:
        bank1 = mlc_positions[:num_of_leaves]
        bank2 = mlc_positions[num_of_leaves:]
    else:
        # Alternatywnie, dzielimy parzyste i nieparzyste elementy
        bank1 = mlc_positions[::2]
        bank2 = mlc_positions[1::2]

    if side == "Y1":
        return bank1
    if side == "Y2":
        return bank2


def return_number_of_beams(dataFile):
    return pydicom.dcmread(dataFile)[0x300a, 0x0070][0][0x300a, 0x0080].value


def return_number_of_controlpoints(dataFile, beam_number):
    return pydicom.dcmread(dataFile)[0x300a, 0x00b0][beam_number][0x300a, 0x0110].value

def return_number_of_leaves(dataFile):
    ds = pydicom.dcmread(dataFile)
    number_of_leaves = ds[0x300a, 0x00b0][0][0x300a, 0x0111][0][0x300a, 0x011a][2][0x300a, 0x011c].VM
    return number_of_leaves



