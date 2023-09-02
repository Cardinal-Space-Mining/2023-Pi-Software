import os
from math import cos, sin, pi, floor, sqrt, tan
from xmlrpc.client import boolean
from adafruit_rplidar import RPLidar, RPLidarException
import sys
import time
from networktables import NetworkTables

# To see messages from networktables, you must setup logging
import logging

logging.basicConfig(
    filename='/home/pi/2021-pi-software/lidar/lidar.log', level=logging.DEBUG)

ip = '10.11.11.2'

NetworkTables.initialize(server=ip)

sd = NetworkTables.getTable('Lidar')


while not NetworkTables.isConnected():
    if (NetworkTables.isConnected()):
        break

# Setup the RPLidar
PORT_NAME = '/dev/ttyUSB0'

# half of the range you want to view from
HALF_RANGE = 55

LIDAR_MOUNT_ANGLE = 30 * (pi / 180)

DEADBAND = 5
OBJECT_TOLERANCE = 100


class Object:
    def __init__(self, width, dist, angle):
        self.width = width
        self.dist = dist
        self.angle = angle


def process_data(calc_data, angles, avg_dist):
    isObj = False
    objDist = []
    objTheta = []
    objWidth = []
    startTheta = 0
    distSum = 0
    count = 0
    d1 = 0
    d2 = 0

    for i, dist in enumerate(calc_data):
        if ((avg_dist - dist) > OBJECT_TOLERANCE):  # change to abs to find crators
            if (isObj == False):
                startTheta = angles[i]
                d1 = dist
                isObj = True
            distSum += dist
            count += 1

        elif isObj == True and (count > 1):
            d2 = calc_data[i-1]
            obj_angle = angles[(i - 1)] - startTheta
            obj_avg_dist = distSum / count
            width = sqrt((d1*d1) + (d2*d2) -
                         (2*d1*d2*cos(obj_angle * pi / 180)))

            midpoint = int((((startTheta + angles[(i - 1)]) / 2) - HALF_RANGE))

            objDist.append(int((obj_avg_dist / 25.4)))
            objWidth.append(int(width / 25.4))
            objTheta.append(midpoint)

            distSum = 0
            count = 0
            isObj = False
        else:
            distSum = 0
            count = 0
            isObj = False

    if isObj == True and (count > 1):
        d2 = calc_data[i]
        obj_angle = angles[i] - startTheta
        obj_avg_dist = distSum / count
        width = sqrt((d1*d1) + (d2*d2) -
                     (2*d1*d2*cos(obj_angle * pi / 180)))

        objDist.append(int((obj_avg_dist / 25.4) * cos(LIDAR_MOUNT_ANGLE)))
        objWidth.append(int(width / 25.4))
        objTheta.append(
            int((((startTheta + angles[i]) / 2) - HALF_RANGE)))

    sd.putNumberArray('distance', objDist)
    sd.putNumberArray('angle', objTheta)
    sd.putNumberArray('width', objWidth)
    objDist.clear()
    objTheta.clear()
    objWidth.clear()


def reorder(angles, calc_data):
    # we must reorder the data so angles 315 - 360 are at the start of the array
    angles_half_range = []
    angles_before = []
    angles_after = []
    calc_data_half_range = []
    calc_data_before = []
    calc_data_after = []
    angles_reordered = []
    calc_data_reordered = []
    start_index = -1
    end_index = -1

    for i in range(0, len(angles)):
        if angles[i] <= HALF_RANGE:
            if start_index < 0:
                start_index = i
            angles_half_range.append(
                min([2*HALF_RANGE, angles[i] + HALF_RANGE]))
            calc_data_half_range.append(calc_data[i])
        elif (start_index >= 0) and (angles[i] > HALF_RANGE) and (end_index < 0):
            end_index = i

    for i in range(0, start_index):
        if angles[i] >= (360 - HALF_RANGE):
            angles_before.append(
                min([HALF_RANGE, angles[i] - (360 - HALF_RANGE)]))
            calc_data_before.append(calc_data[i])

    for i in range(end_index, len(angles)):
        if angles[i] >= (360 - HALF_RANGE):
            angles_after.append(
                min([HALF_RANGE, angles[i] - (360 - HALF_RANGE)]))
            calc_data_after.append(calc_data[i])

    if (len(angles_before) == 0):
        angles_reordered = angles_after + angles_half_range
        calc_data_reordered = calc_data_after + calc_data_half_range

    elif (len(angles_after) == 0):
        angles_reordered = angles_before + angles_half_range
        calc_data_reordered = calc_data_before + calc_data_half_range

    elif (angles_before[0] > angles_after[0]):
        angles_reordered = angles_after + angles_before + angles_half_range
        calc_data_reordered = calc_data_after + calc_data_before + calc_data_half_range

    elif (angles_after[0] > angles_before[0]):
        angles_reordered = angles_before + angles_after + angles_half_range
        calc_data_reordered = calc_data_before + calc_data_after + calc_data_half_range

    return angles_reordered, calc_data_reordered


def main():
    calc_data = []
    angles = []
    sum_dist = 0
    avg_dist = 0
    long_avg = 0
    avg_sum = 0
    avg_count = 0
    scan_count = 0

    try:
        lidar = RPLidar(None, PORT_NAME, timeout=3)
        for scan in lidar.iter_scans(1500, 5):
            for (_, angle, distance) in scan:
                if (angle <= HALF_RANGE or angle >= (360 - HALF_RANGE)):
                    angle_int = floor(angle)
                    radians = angle * pi / 180.0
                    x = distance * cos(radians)
                    calc_data.append(x)
                    angles.append(angle_int)
                    sum_dist += x
                    scan_count += 1

            if (scan_count > 0):
                avg_dist = sum_dist / scan_count
                avg_count += 1
                avg_sum += avg_dist
                long_avg = avg_sum / avg_count
            # sd.putNumber('Average Distance', int(avg_dist))
            # sd.putNumber('Long Average', int(long_avg))

            angles_reordered, calc_data_reordered = reorder(angles, calc_data)

            process_data(calc_data_reordered, angles_reordered, long_avg)
            calc_data.clear()
            angles.clear()
            sum_dist = 0
            scan_count = 0

    except RPLidarException as e:
        lidar.stop()
        lidar.disconnect()
        print("Something Went Wrong: " + str(e))
        main()
    except KeyboardInterrupt:
        lidar.stop()
        lidar.disconnect()
    except Exception as e:
        lidar.stop()
        lidar.disconnect()
        print("Something Went Wrong: " + str(e))
        main()


if __name__ == "__main__":
    main()
