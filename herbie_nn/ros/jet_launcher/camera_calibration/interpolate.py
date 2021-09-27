import numpy as np
from solvepnp_new import camera_transform

class Interpolate:
    def __init__(self):
        self.c = camera_transform()
        self.img_points = self.c.imagePoints
        self.obj_points = self.c.objectPoints
        self.img_rows = self.img_points[0].shape[0]
        self.dataset_x = np.zeros([self.img_rows, 3])
        self.dataset_y = np.zeros([self.img_rows, 3])
        self.dataset_side = np.full((360,640),-5000)
        self.dataset_front = np.full((360,640),-5000)

    def nearest_neighbor_interpolation(self,data, x, y, p=0.5):
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

    def create_data(self):
        for i in range(self.img_rows):
            self.img_point = (self.img_points[0])[i]
            self.obj_point = (self.obj_points[0])[i]
            self.dataset_x[i,0] = self.img_point[0]
            self.dataset_x[i,1] = self.img_point[1]
            self.dataset_x[i,2] = self.obj_point[0]

            self.dataset_side[int(self.dataset_x[i,1]), int(self.dataset_x[i,0])] = self.dataset_x[i,2]
        print(self.dataset_x)

        for i in range(self.img_rows):
            self.img_point = (self.img_points[0])[i]
            self.obj_point = (self.obj_points[0])[i]
            self.dataset_y[i,0] = self.img_point[0]
            self.dataset_y[i,1] = self.img_point[1]
            self.dataset_y[i,2] = self.obj_point[2]

            self.dataset_front[int(self.dataset_y[i,1]), int(self.dataset_y[i,0])] = self.dataset_y[i,2]
        print(self.dataset_y)

        counter = 0
        for row in range(360):
            for col in range(640):
                if self.dataset_side[row,col] == -5000:
                    temp = interp.nearest_neighbor_interpolation(self.dataset_x, col, row, p=6)
                    self.dataset_side[row,col] = temp
                else:
                    print ('existing')

                if self.dataset_front[row,col] == -5000:
                    temp = interp.nearest_neighbor_interpolation(self.dataset_y, col, row, p=6)
                    self.dataset_front[row,col] = temp
            counter += 1
            print(counter)


    def generate_output(self):
        col_max = 640
        row_max = 360

        print("float front[360][640] = {")
        for row in range(row_max):
            print ("{", end='')
            for col in range(col_max):
                print(str(self.dataset_front[row,col]), end='')

                if col != (col_max-1):
                    print(",", end='')

            print ("}", end='')

            if row != (row_max-1):
                print (',')
        print ('};')

        print("float side[360][640] = {")
        for row in range(row_max):
            print ("{", end='')
            for col in range(col_max):
                print(str(self.dataset_side[row,col]), end='')

                if col != (col_max-1):
                    print(",", end='')

            print ("}", end='')

            if row != (row_max-1):
                print (',')
        print ('};')



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

interp = Interpolate()

print(interp.nearest_neighbor_interpolation(data, 33.4, 87.93))
interp.create_data()
interp.generate_output()
