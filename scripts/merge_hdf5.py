import glob
import h5py
import os

def merge_hdf5_files(thread_pattern, merged_filename):
    thread_files = sorted(glob.glob(thread_pattern))
    if not thread_files:
        print(f"No files match pattern: {thread_pattern}")
        return

    # Open the first file to get structure
    with h5py.File(thread_files[0], "r") as f0, h5py.File(merged_filename, "w") as mf:
        # Create groups and empty datasets in merged file
        for run_group in f0.keys():
            mg = mf.create_group(run_group)
            for data_group in f0[run_group].keys():
                dg = mg.create_group(data_group)
                for field in f0[run_group][data_group].keys():
                    ds0 = f0[run_group][data_group][field]
                    # Prepare unlimited first dimension
                    shape = (0,) + ds0.shape[1:]
                    maxshape = (None,) + ds0.shape[1:]
                    chunks = ds0.chunks if ds0.chunks else None
                    dtype = ds0.dtype
                    mf.create_dataset(f"{run_group}/{data_group}/{field}",
                                      shape=shape,
                                      maxshape=maxshape,
                                      chunks=chunks,
                                      dtype=dtype)

        # Append data from each thread file
        for fname in thread_files:
            with h5py.File(fname, "r") as f:
                for run_group in f.keys():
                    for data_group in f[run_group].keys():
                        for field in f[run_group][data_group].keys():
                            data = f[f"{run_group}/{data_group}/{field}"][...]
                            ds = mf[f"{run_group}/{data_group}/{field}"]
                            old_size = ds.shape[0]
                            new_size = old_size + data.shape[0]
                            ds.resize((new_size,) + ds.shape[1:])
                            ds[old_size:new_size] = data

    print(f"Merged {len(thread_files)} files into {merged_filename}")

if __name__ == "__main__":
    # Example usage: adjust pattern as needed
    merge_hdf5_files("output_run0_thread*.h5", "merged_run0.h5")
