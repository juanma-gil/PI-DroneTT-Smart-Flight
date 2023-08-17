import json

position_path = 'res/route.json'
coordinate_path = 'res/coordinates.json'


class Coordinate:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def to_dict(self):
        return {'x': self.x, 'y': self.y, 'z': self.z}


def timestamp_filter(obj):
    unwanted_key = 'time'
    return {key: value for key, value in obj.items() if key != unwanted_key}


def distance_filter(coordinates):
    filtered_coordinates = [coordinates[0]]  # Add the first point

    for i in range(1, len(coordinates)):
        prev_point = filtered_coordinates[-1]
        current_point = coordinates[i]

        x_diff = abs(current_point['x'] - prev_point['x'])
        y_diff = abs(current_point['y'] - prev_point['y'])
        z_diff = abs(current_point['z'] - prev_point['z'])

        # Check if all the absolute differences are outside the range -20 and 20
        if not (-20 < x_diff < 20 and -20 < y_diff < 20 and -20 < z_diff < 20):
            filtered_coordinates.append(current_point)

    return filtered_coordinates


def position_to_coordinate(position, scalar):
    x = int(position[0] * scalar)
    y = int(position[1] * scalar)
    z = int(position[2] * scalar)
    return Coordinate(x, y, z).to_dict()


try:
    with open(position_path, 'r') as file:
        orbit = json.load(file)
except FileNotFoundError:
    print("File not found")

# Apply the timestamp_filter to each dictionary in the list
filtered_orbit = list(map(timestamp_filter, orbit))

scalar = 100
route = {'unit': 'cm', 'points': []}

# Apply the position_to_coordinate function to each point in the 'position' list
for point in filtered_orbit:
    route['points'].append(position_to_coordinate(point['position'], scalar))

#route['points'] = distance_filter(route['points'])

try:
    with open(coordinate_path, 'w') as file:
        json.dump(route, file)
except FileNotFoundError:
    print("File had not been created")

print(f"Points size = {route['points'].__len__()}")
