import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from mpl_toolkits.mplot3d import Axes3D
import pandas as pd
from scipy.interpolate import griddata as gd
from matplotlib import cm

from IPython.display import Image
from solvepnp_new import camera_transform

def nearest_neighbor_interpolation(data, x, y, p=0.5):
    """
    Nearest Neighbor Weighted Interpolation
    http://paulbourke.net/miscellaneous/interpolation/
    http://en.wikipedia.org/wiki/Inverse_distance_weighting

    :param data: numpy.ndarray
        [[float, float, float], ...]
    :param p: float=0.5
        importance of distant samples
    :return: interpolated data
    """
    n = len(data)
    vals = np.zeros((n, 2), dtype=np.float64)
    distance = lambda x1, x2, y1, y2: (x2 - x1)**2 + (y2 - y1)**2
    for i in range(n):
        vals[i, 0] = data[i, 2] / (distance(data[i, 0], x, data[i, 1], y))**p
        vals[i, 1] = 1          / (distance(data[i, 0], x, data[i, 1], y))**p
    z = np.sum(vals[:, 0]) / np.sum(vals[:, 1])
    return z

def create_data():
    c = camera_transform()
    print(c.imagePoints)
    img_points = c.imagePoints
    obj_points = c.objectPoints
    img_rows = img_points[0].shape[0]
    dataset = np.zeros([img_rows, 3])

    for i in range(img_rows):
        img_point = (img_points[0])[i]
        obj_point = (obj_points[0])[i]
        dataset[i,0] = img_point[0]
        dataset[i,1] = img_point[1]
    print(dataset)


#sample x,y,z data
data = np.array([
[33.44, 87.93, 105.88],
[8.81, 84.07, 103.11],
[15.62, 34.83, 105.98],
[40.16, 38.71, 108.13],
[61.45, 67.07, 108.12],
[58.81, 91.44, 107.72],
[36.97, 63.29, 107.14],
[64.71, 42.38, 109.07],
[89.11, 46.49, 109.93],
[67.24, 18.32, 109.99],
[65.90, 31.93, 109.51],
[76.55, 44.51, 109.91]])

print(nearest_neighbor_interpolation(data, 33.4, 87.93, p=6))
create_data()
