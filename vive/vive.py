import triad_openvr
import time
import sys
from os import system
import math
from scipy.spatial.transform import Rotation as r
import numpy as np
from networktables import NetworkTables
from vive_constants import *
sys.path.insert(0, '/home/po/2023-pi-software/path_planning/map_client')
from TCPServerBinding import TCPServerBinding


def rigid_transform_3D(A, B):
    assert A.shape == B.shape

    num_rows, num_cols = A.shape
    if num_rows != 3:
        raise Exception(f"matrix A is not 3xN, it is {num_rows}x{num_cols}")

    num_rows, num_cols = B.shape
    if num_rows != 3:
        raise Exception(f"matrix B is not 3xN, it is {num_rows}x{num_cols}")

    # find mean column wise
    centroid_A = np.mean(A, axis=1)
    centroid_B = np.mean(B, axis=1)

    # ensure centroids are 3x1
    centroid_A = centroid_A.reshape(-1, 1)
    centroid_B = centroid_B.reshape(-1, 1)

    # subtract mean
    Am = A - centroid_A
    Bm = B - centroid_B

    H = Am @ np.transpose(Bm)

    # sanity check
    # if linalg.matrix_rank(H) < 3:
    #    raise ValueError("rank of H = {}, expecting 3".format(linalg.matrix_rank(H)))

    # find rotation
    U, S, Vt = np.linalg.svd(H)
    R = Vt.T @ U.T

    # special reflection case
    if np.linalg.det(R) < 0:
        # print("det(R) < R, reflection detected!, correcting for it ...")
        Vt[2,:] *= -1
        R = Vt.T @ U.T

    t = -R @ centroid_A + centroid_B

    return R, t

def m_in(m):
    return m / 0.0254

def in_m(i):
    return i * 0.0254

def lh_to_pos(pos):
    return np.matrix([[pos[0]], [pos[1]], [pos[2]]])

def lh_to_rot(angle):
    return r.from_euler('xyz', angle, degrees=True).as_matrix()

def point_generator(pos_mat, rot_mat):    
    px0 = np.add(np.matmul(rot_mat, np.matrix([[.1], [0], [0]])), pos_mat)
    py0 = np.add(np.matmul(rot_mat, np.matrix([[0], [.1], [0]])), pos_mat)
    pz0 = np.add(np.matmul(rot_mat, np.matrix([[0], [0], [.1]])), pos_mat)
    px1 = np.add(np.matmul(rot_mat, np.matrix([[-.1], [0], [0]])), pos_mat)
    py1 = np.add(np.matmul(rot_mat, np.matrix([[0], [-.1], [0]])), pos_mat)
    pz1 = np.add(np.matmul(rot_mat, np.matrix([[0], [0], [-.1]])), pos_mat)
    return [px0, py0, pz0, px1, py1, pz1]

class vive:
    def __init__(self):
        self.v = triad_openvr.triad_openvr()
        self.v.print_discovered_objects()
        self.R = np.empty(0)
        self.t = np.empty(0)

        start_time = time.monotonic_ns()
        print(time.monotonic_ns() - start_time)
        while time.monotonic_ns() - start_time < CALIBRATION_TIME:
            self.v.poll_vr_events()
            system('clear')
            print('calibrating', end=": ")
            print(round(((time.monotonic_ns() - start_time) / CALIBRATION_TIME) * 100), end="%\n")
            scene = {}
            for device in self.v.devices.items():
                matrix = device[1].get_pose_matrix()
                if matrix:
                    scene[device[1].get_serial()] = {
                        'pos': np.matrix([[matrix[0][3]], [matrix[1][3]], [matrix[2][3]]]),
                        'rot': np.matrix([matrix[0][0:3], matrix[1][0:3], matrix[2][0:3]])
                    }
   
            p = []
            q = []

            if LH0_SERIAL in scene.keys():
                p = p + point_generator(scene[LH0_SERIAL]['pos'], scene[LH0_SERIAL]['rot'])
                q = q + point_generator(lh_to_pos(LH0_REAL_POS), lh_to_rot(LH0_REAL_ANGLE))
                
            if len(p) > 0 and len(q) > 0:
                p = np.concatenate(p, axis=1)
                q = np.concatenate(q, axis=1)
                self.R, self.t = rigid_transform_3D(p, q)
            else:
                start_time = time.monotonic_ns()
    
    def get_pose(self):
        self.v.poll_vr_events()
        scene = {}
        for device in self.v.devices.items():
            matrix = device[1].get_pose_matrix()
            if matrix:
                scene[device[1].get_serial()] = {
                    'pos': np.matrix([[matrix[0][3]], [matrix[1][3]], [matrix[2][3]]]),
                    'rot': np.matrix([matrix[0][0:3], matrix[1][0:3], matrix[2][0:3]])
                }

        if T0_SERIAL in scene.keys():
            # rotation
            yaw_rot = r.from_matrix(np.dot(self.R, scene[T0_SERIAL]['rot'])).as_euler('XYZ', degrees=True)
            no_yaw_rot = r.from_matrix(np.dot(r.from_euler('XYZ', [0, yaw_rot[2], 0], degrees=True).as_matrix(), np.dot(self.R, scene[T0_SERIAL]['rot']))).as_euler('XYZ', degrees=True)
            yaw = round((yaw_rot[2] + 360) % 360, 1)
            pitch = round((no_yaw_rot[0] + 270) % 360, 1)
            roll = round((no_yaw_rot[1] + 360) % 360, 1)
            res_rot = [roll, pitch, yaw]

            # position
            raw_pos = np.add(np.matmul(self.R, scene[T0_SERIAL]['pos']), self.t).flatten().tolist()[0]
            raw_pos = [m_in(raw_pos[i]) for i in range(len(raw_pos))]
            # print(raw_pos)
            x = -raw_pos[2]
            y = raw_pos[0]
            z = raw_pos[1]
            dy = -13 * math.sin(math.radians(yaw))
            dx = -13 * math.cos(math.radians(yaw))
            y = y + dy
            x = x + dx
            res_pos = [x, y, z]
            res_pos = [round(res_pos[i], 1) for i in range(len(raw_pos))]

            # cry
            return res_pos, res_rot
        
if __name__ == '__main__':
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
        conn = TCPServerBinding("localhost", 8080)

    vive = vive()
    while True:
        pose = vive.get_pose()
        # print(pose)
        if pose != None:
            if MODE == 'online':
                print(pose)
                sd.putNumber("x", pose[0][0])
                sd.putNumber("y", pose[0][1])
                sd.putNumber("z", pose[0][2])
                sd.putNumber("roll", pose[1][0])
                sd.putNumber("pitch", pose[1][1])
                sd.putNumber("yaw", pose[1][2])
                try:
                    conn.set_pos(pose[0][0], pose[0][1])
                except Exception as e:
                    pass
            else:
                system('clear')
                print("position: ", end="")
                print(pose[0])
                print("rotation: ", end="")
                print(pose[1])