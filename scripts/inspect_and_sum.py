import h5py
import argparse

def print_hdf5_structure(name, obj):
    print(name)

def inspect_and_sum(h5file, run_group, dataset_name):
    with h5py.File(h5file, "r") as f:
        print("HDF5 File Structure:")
        f.visititems(print_hdf5_structure)
        base_path = f"/{run_group}/{dataset_name}"
        if base_path + "/VoxelIdX" not in f:
            print(f"Dataset path '{base_path}' not found.")
            return
        # if found, perform sum
        x = f[f"{base_path}/VoxelIdX"][...]
        y = f[f"{base_path}/VoxelIdY"][...]
        z = f[f"{base_path}/VoxelIdZ"][...]
        dose = f[f"{base_path}/VoxelDose"][...]
        import pandas as pd
        df = pd.DataFrame({"X": x, "Y": y, "Z": z, "Dose": dose})
        summed = df.groupby(["X", "Y", "Z"], as_index=False)["Dose"].sum()
        summed.to_csv('out.csv', index=False)
        print("\nSummed Dose per Voxel:")
        print(summed)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Inspect HDF5 and sum voxel doses")
    parser.add_argument("h5file", help="Input HDF5 filename")
    parser.add_argument("--run_group", default="", help="HDF5 run group, e.g. Run_0")
    parser.add_argument("--dataset", default="", help="HDF5 dataset name, e.g. EventData")
    args = parser.parse_args()
    inspect_and_sum(args.h5file, args.run_group, args.dataset)
