import pandas as pd
import matplotlib.pyplot as plt

file = "/home/brachwal/workspace/g4rt/output/water_phantom_5/sim/cp10x10/cp10x10_watertank_voxel.csv"
# Load the CSV file
data = pd.read_csv(file)
print(data)

# Define the specific Voxel IdX and Voxel IdY values you want
voxel_x = 20
voxel_y = 20

# Extract relevant columns
z_values = data.loc[(data["Voxel IdX"] == voxel_x) & (data["Voxel IdY"] == voxel_y), "Z [mm]"]
dose_values = data.loc[(data["Voxel IdX"] == voxel_x) & (data["Voxel IdY"] == voxel_y), "Dose [Gy]"]
dose_values = dose_values/dose_values.max()

# Plot
plt.figure(figsize=(8, 5))
plt.scatter(z_values, dose_values, color='b', label='Dose Profile')
plt.xlabel("Z [mm]")
plt.ylabel("Dose [Gy]")
plt.title("Dose Profile along Z-axis")
plt.grid(True)
plt.legend()
plt.show()