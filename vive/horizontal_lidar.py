# from networktables import NetworkTables
import sys
import math
import time
from os import system
from threading import Thread, Lock
from math import cos, sin, pi, floor, sqrt, tan, asin
from adafruit_rplidar import RPLidar, RPLidarException
from vive import *

sys.path.insert(0, '/home/po/2023-pi-software/path_planning/map_client')

from TCPServerBinding import TCPServerBinding

# Setup the RPLidar
PORT_NAME = '/dev/ttyUSB1'

# half of the range you want to view from
HALF_RANGE = 60

# robot's max turning width in inches
ROBOT_WIDTH = 36

ROBOT_CENTER_TO_LIDAR_CENTER = 16.5 * 25.4

robot_yaw = 0
robot_pitch = 0
robot_roll = 0
rx = 0
ry = 0

vive = vive()

def main(): 
    global robot_yaw
    global robot_pitch
    global robot_roll
    global rx
    global ry
    global v
    global vive
    conn = TCPServerBinding("localhost", 8080)
    
    try:
        lidar = RPLidar(None, PORT_NAME, timeout=3)
        for measurement in lidar.iter_measurements(max_buf_meas=1):
            res_pos, res_angle = vive.get_pose()
            if res_pos != None and res_angle != None:
                rx = res_pos[0]
                ry = res_pos[1]

                # print(rx, ry)


                robot_yaw = (round(res_angle[2], 2)) % 360
                if(round(res_angle[2], 2) < 0):
                    robot_yaw = (360 - abs(round(res_angle[2], 2))) % 360
                    
                # robot_yaw = (360 - abs(round(res_angle[2], 2))) % 360
                # if(round(res_angle[2], 2) < 0):
                #     robot_yaw = (round(res_angle[2], 2)) % 360

                robot_pitch = (round(res_angle[1], 2))
                robot_roll = (round(res_angle[0], 2))


                distance = measurement[3]
                angle = measurement[2]
                # print(distance, angle)
                # wanted LiDAR view range. 
                if (angle <= 170 and angle >= 10) and (distance > 0):# and (distance > 0) and (abs(robot_pitch - 90) < 10) and (abs(robot_roll - 180) < 10):

                    # First get the object relative to the vive tracker
                    dist2 = distance * distance
                    rclc2 = ROBOT_CENTER_TO_LIDAR_CENTER * ROBOT_CENTER_TO_LIDAR_CENTER
                    dist_angle = 90 + angle
                    if angle < 90:
                        dist_angle = 270 - angle

                    # dist_angle = 180 - angle
                    # if angle > 180:
                    #     dist_angle = angle - 180
                    dist = sqrt((dist2) + (rclc2) - 2*(distance * ROBOT_CENTER_TO_LIDAR_CENTER) * cos(dist_angle * pi / 180))
                    
                    # print("vive to obj", dist / 25.4)
                    new_angle = asin((sin(dist_angle * pi / 180) * distance) / dist) * 180 / pi
                    if angle > 90:
                        new_angle = new_angle * -1

                    # Then find the object relative to the arena origin point
                    angle_relative_arena = (new_angle + robot_yaw) % 360
                    x = rx + ((dist) * cos((angle_relative_arena) * pi / 180) / 25.4)
                    y = ry + ((dist) * sin((angle_relative_arena) * pi / 180) / 25.4)
                    
                    # Add the obstacle only if it is in bounds of the weight_map
                    if (x < 270 and x >= 0) and (y < 50 and y > -50):
                        print(measurement)
                        # print("rob pos", rx, ry)
                        # print("obj pos", x, y)
                        conn.add_obstacle(x, y, ROBOT_WIDTH, 255)
                        # conn.set_weight(x, y, 255)

    except RPLidarException as e:
        lidar.stop()
        lidar.disconnect()
        print("RPLidarException: " + str(e))
        main()
    except KeyboardInterrupt:
        lidar.stop()
        lidar.disconnect()
    except Exception as e:
        lidar.stop()
        lidar.disconnect()
        print("Exception: " + str(e))
        main()           

if __name__ == "__main__":
    main()
    # p = cProfile.Profile()
    # p.enable()
    # try:
    #     main()
    # except:
    #     p.disable()

    # p.print_stats()
    