import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Read the CSV file
# df0 = pd.read_csv('../build/weighted_influence_cp0.csv')
df0 = pd.read_csv('../build/weighted_influence.csv')
df1 = pd.read_csv('../build/weighted_influence_cp10.csv')


# Extract x, y, and influence columns
selected_data0 = df0[df0['x'] == 5]
selected_data1 = df1[df1['x'] == 5]
# print(selected_data)

# Extract y and wi columns from the DataFrame
y0 = selected_data0['y']
wi0 = selected_data0['wi']

y1 = selected_data1['y']
wi1 = selected_data1['wi']

# Plot both datasets on the same plot
plt.figure(figsize=(8, 6))

# Plot for selected_data0
plt.scatter(y0, wi0, label='CP 0', c='b', marker='o')

# Plot for selected_data1
# plt.scatter(y1, wi1, label='CP 1', c='r', marker='s')

# Add labels and title
plt.xlabel('Y')
plt.ylabel('Weighted Influence (wi)')
plt.title('Weighted Influence vs. Y')
plt.grid(True)
plt.legend()

plt.show()
