from networktables import NetworkTables
import triad_openvr
import time
import sys
import numpy as np
from os import system
import math

LH0_REAL_POS = (0, 0, 0)
LH0_REAL_EULER = (0, 0, 0)
LAST_KNOWN_POS = (0, 0, 0)
TEAM_NUMBER = 1111
HZ = 30

def m_in(m):
    return m / 0.0254

def convert_to_euler(pose_mat):
    yaw = 180 / math.pi * math.atan2(pose_mat[1][0], pose_mat[0][0])
    pitch = 180 / math.pi * math.atan2(pose_mat[2][0], pose_mat[0][0])
    roll = 180 / math.pi * math.atan2(pose_mat[2][1], pose_mat[2][2])
    x = pose_mat[0][3]
    y = pose_mat[1][3]
    z = pose_mat[2][3]
    return [yaw,pitch,roll]

v = triad_openvr.triad_openvr()
v.print_discovered_objects()

MODE = "online"
for i in range(1, len(sys.argv)):
    if (sys.argv[i] == "--local"):
        MODE = "local"

# connect to networktables
if MODE == "online":
    print("Connecting to team " + str(TEAM_NUMBER))
    NetworkTables.startClientTeam(TEAM_NUMBER)
    while not NetworkTables.isConnected():
        if(NetworkTables.isConnected()):
            break
    print("Connected to team " + str(TEAM_NUMBER))
    sd = NetworkTables.getTable("Vive Position")

# limit main loop to 30hz
interval = 1/HZ
while(True):
    v.poll_vr_events()
    start = time.time()
    txt = ""
    system('clear')
    for device in v.devices.items():
        print(device[0], end=" - ")
        print(device[1].get_serial())
    print("-----------------------------------------------------------------------------")
    for device in v.devices.values():
        if device.get_serial() == "LHR-1E6A3E70" and device.get_pose_euler():
            latest_pose = device.get_pose_euler()
            if MODE == 'online':
                sd.putNumber("x", round(m_in(latest_pose[0])))
                sd.putNumber("z", round(m_in(latest_pose[1])))
                sd.putNumber("y", -round(m_in(latest_pose[2])))
                sd.putNumber("roll", round(latest_pose[3], 2))
                sd.putNumber("yaw", round(latest_pose[4], 2))
                sd.putNumber("pitch", round(latest_pose[5], 2))
            else:
                print("x: ", end="")
                print(round(m_in(latest_pose[0])))
                print("z: ", end="")
                print(round(m_in(latest_pose[1])))
                print("y: ", end="")
                print(-round(m_in(latest_pose[2])))
                print(round(latest_pose[3], 2))
                print(round(latest_pose[4], 2))
                print(round(latest_pose[5], 2))

    sleep_time = interval-(time.time()-start)
    if sleep_time>0:
        time.sleep(sleep_time)
