import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Read the CSV file
# df0 = pd.read_csv('../build/weighted_influence_cp0.csv')
df = pd.read_csv('../build/weighted_influence.csv')

# Create a new figure
fig = plt.figure()

# Add a 3D subplot to the figure
ax = fig.add_subplot(111, projection='3d')

# Extract x, y, and influence columns
x = df['x']
y = df['y']
z = df['wi']

# Scatter plot
sc = ax.scatter(x, y, z, c=z, cmap='viridis')

# Add color bar which maps values to colors
plt.colorbar(sc, ax=ax, label='Influence')

# Set labels
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Influence')

# Show plot
plt.show()
