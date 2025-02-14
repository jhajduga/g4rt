import pydicom
import numpy as np

class RadiotherapyPlan:
    def __init__(self, filepath):
        self.filepath = filepath
        try:
            self.ds = pydicom.dcmread(filepath)
        except Exception as e:
            raise ValueError(f"Nie udało się odczytać pliku DICOM {filepath}: {e}")

    def get_beam_count(self):
        try:
            beam_counter = int(self.ds[0x300a, 0x0070][0][0x300a, 0x0080].value)
            return beam_counter
        except Exception as e:
            raise ValueError(f"Błąd przy odczycie liczby wiązek: {e}")

    def get_beam_type(self, beam_index):
        try:
            beam_type = self.ds[0x300a, 0x00b0][beam_index][0x300A, 0x00C4].value
            return beam_type.strip() if isinstance(beam_type, str) else beam_type
        except Exception as e:
            raise ValueError(f"Błąd przy odczycie typu wiązki dla wiązki {beam_index}: {e}")

    def get_control_point_count_in_beam(self, beam_index):
        try:
            cp_count = int(self.ds[0x300a, 0x00b0][beam_index][0x300a, 0x0110].value)
            beam_type = self.get_beam_type(beam_index)
            if beam_type.upper() == "STATIC":
                cp_count = 1
            return cp_count
        except Exception as e:
            raise ValueError(f"Błąd przy odczycie liczby punktów kontrolnych dla wiązki {beam_index}: {e}")

    def get_jaw_positions(self, beam_index, cp_index=0):
        try:
            cp = self.ds[0x300a, 0x00b0][beam_index][0x300a, 0x0111][cp_index]
            jaw_x_positions = cp[0x300a, 0x011a][0][0x300a, 0x011c].value
            jaw_y_positions = cp[0x300a, 0x011a][1][0x300a, 0x011c].value

            jaw_x_positions = np.array(jaw_x_positions, dtype=np.single)
            jaw_y_positions = np.array(jaw_y_positions, dtype=np.single)
            return {"X": jaw_x_positions, "Y": jaw_y_positions}
        except Exception as e:
            raise ValueError(f"Błąd przy odczycie pozycji szczęk dla wiązki {beam_index} CP {cp_index}: {e}")

    def get_num_of_leaves(self, beam_index):
        try:
            num_of_leaves = self.ds[0x300a, 0x00b0][beam_index][0x300a, 0x0111][0][0x300a, 0x011a][2][0x300a, 0x011c].VM
            return num_of_leaves
        except Exception as e:
            raise ValueError(f"Błąd przy odczycie liczby listów dla wiązki {beam_index}: {e}")

    def get_mlc_leaf_positions(self,side, beam_index, cp_index=0):

        beam_type = self.get_beam_type(beam_index)
        if beam_type.upper() == "STATIC" and cp_index != 0:
            raise ValueError("Dla planu statycznego używamy tylko pierwszego punktu kontrolnego (cp_index=0).")
        try:
            cp = self.ds[0x300a, 0x00b0][beam_index][0x300a, 0x0111][cp_index]
            mlc_seq = cp[0x300a, 0x011a][2]
            mlc_positions_list = mlc_seq[0x300a, 0x011c].value
            num_of_leaves = self.get_num_of_leaves(beam_index)
            mlc_positions = np.array(mlc_positions_list, dtype=np.single)

            if mlc_positions.size == num_of_leaves * 2:
                bank1 = mlc_positions[:num_of_leaves]
                bank2 = mlc_positions[num_of_leaves:]
            else:
                bank1 = mlc_positions[::2]
                bank2 = mlc_positions[1::2]
            
            if side == "Y1":
              return bank1
            if side == "Y2":
              return bank2
        except Exception as e:
            raise ValueError(f"Błąd przy odczycie pozycji MLC dla wiązki {beam_index} CP {cp_index}: {e}")

if __name__ == "__main__":
    # Przykładowe użycie:
    filepath = "/mnt/c/Users/Jakub/Desktop/CT_NIO/RP.1.2.246.352.71.5.528868944050.1691298.20241220174438.dcm"
    plan = RadiotherapyPlan(filepath)
    
    beam_count = plan.get_beam_count()
    print(f"Liczba wiązek: {beam_count}")
    
    for b in range(beam_count):
        beam_type = plan.get_beam_type(b)
        print(f"\nWiązka {b+1} typu: {beam_type}")
        cp_count = plan.get_control_point_count_in_beam(b)
        print(f"Liczba punktów kontrolnych dla wiązki {b+1}: {cp_count}")
        
        try:
            jaws = plan.get_jaw_positions(b, cp_index=0)
            print("Pozycje szczęk:")
            print("X:", jaws["X"])
            print("Y:", jaws["Y"])
        except Exception as e:
            print(e)
        
        for cp in range(cp_count):
            try:
                bank1 = plan.get_mlc_leaf_positions(b, "Y1" cp_index=cp)

            except Exception as e:
                print(f"Nie udało się odczytać pozycji MLC dla wiązki {b+1}, CP {cp+1}: {e}")
