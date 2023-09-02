import os
from math import cos, sin, pi, floor, sqrt, tan, asin
from xmlrpc.client import boolean
from adafruit_rplidar import RPLidar, RPLidarException
import sys
import time
from networktables import NetworkTables
import logging

# sys.path.insert(0, '/home/pi/2023-pi-software/path_planning/map_client')

# from PyServerBindings import PyWeightMapServerBindings

logging.basicConfig(filename = os.getcwd() + '/lidar.log', filemode='w', level=logging.DEBUG)

# Setup the RPLidar
PORT_NAME = '/dev/ttyUSB0'

# half of the range you want to view from
HALF_RANGE = 60

# robot's max turning width in inches
ROBOT_WIDTH = 15

ROBOT_CENTER_TO_LIDAR_CENTER = 17.25

TEAM_NUMBER = 1111

# MODE = "online"

# if MODE == "online":
#     print("Connecting to team " + str(TEAM_NUMBER))
#     NetworkTables.startClientTeam(TEAM_NUMBER)
#     while not NetworkTables.isConnected():
#         if(NetworkTables.isConnected()):
#             break
#     print("Connected to team " + str(TEAM_NUMBER))
#     sd = NetworkTables.getTable('Gyro')

def main():
    try:
        lidar = RPLidar(None, PORT_NAME, timeout=3)
        for scan in lidar.iter_scans(1500, 5):
            for (_, angle, distance) in scan:
                  if((angle >= 10) and (angle < 170)):
                # if (angle <= HALF_RANGE or angle >= (360 - HALF_RANGE)):
                    radians = angle * pi / 180.0
                    print(distance, angle)
                    # if MODE == "online":
                    #     robot_angle = sd.getNumber('Yaw', 0)
                    #     robot_angle = (360 - robot_angle) % 360
                    # else:
                    #     robot_angle = 0
                    # robot_pos = PyWeightMapServerBindings.get_pos()
                    # dist2 = distance * distance
                    # rclc2 = ROBOT_CENTER_TO_LIDAR_CENTER * ROBOT_CENTER_TO_LIDAR_CENTER
                    # dist_angle = 90 + angle
                    # if angle > 90:
                    #     dist_angle = 270 - angle
                    # dist = sqrt((dist2) + (rclc2) - 2*(distance * ROBOT_CENTER_TO_LIDAR_CENTER) * cos(dist_angle * pi / 180))
                    # new_angle = asin((sin(dist_angle * pi / 180) * distance) / dist) * 180 / pi
                    # if angle > 90:
                    #     new_angle = new_angle * -1

                    # angle_relative_arena = (new_angle + robot_angle) % 360
                    # x = robot_pos[1] + ((dist) * cos((angle_relative_arena) * pi / 180) / 25.4)
                    # y = robot_pos[0] + ((dist) * sin((angle_relative_arena) * pi / 180) / 25.4)
                    # x = int(x)
                    # y = int(y)
                    # print("pos:", robot_pos[1], robot_pos[0], robot_angle)
                    # print("dist", dist / 25.4, distance, angle_relative_arena,  angle)
                    # print("obj pos: ", x, y, angle_relative_arena, new_angle)
                    # if (x < 270 and x >= 0) and (y < 50 and y > -50):
                    #     PyWeightMapServerBindings.set_weight(x, y, 255)
                    


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
