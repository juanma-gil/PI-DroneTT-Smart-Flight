import json
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Read JSON data from file
with open('res/coordinates.json', 'r') as file:
    json_data = file.read()

# Load JSON data
data = json.loads(json_data)

# Extract points
points = data['points']

# Extract x, y, and z coordinates
x_coords = [point['x'] for point in points]
y_coords = [point['y'] for point in points]
z_coords = [point['z'] for point in points]

# Create a 3D scatter plot
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.scatter(x_coords, y_coords, z_coords, c='r', marker='o')

# Set labels and title
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_title('Órbita original - escalado x 100')

plt.show()
