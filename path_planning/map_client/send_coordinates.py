from networktables import NetworkTables
import time
from TCPServerBinding import TCPServerBinding
from math import pow, sqrt



MODE = "online"

TEAM_NUMBER = 1112

# connect to networktables
if MODE == "online":
    print("Connecting to team " + str(TEAM_NUMBER))
    NetworkTables.startClientTeam(TEAM_NUMBER)
    while not NetworkTables.isConnected():
        if(NetworkTables.isConnected()):
            break
    print("Connected to team " + str(TEAM_NUMBER))
    sd = NetworkTables.getTable("Path Plan")
    conn = TCPServerBinding("localhost", 8080)


PERMANENT_PATH = []
PERMANENT_PATH_COMPLETE = False
END_X = 205

INITIAL_X = 40
pos = conn.get_pos()

# loo[ until we are ready to start traversing
while not sd.getNumber("t_state", 0) == 1:
    pos = conn.get_pos()


# curr_path = conn.path_to_line(pos[0], pos[1], 269)
curr_path = conn.path_to_line(pos[0], pos[1], END_X)
last_coord = [pos[0], pos[1]]
curr_coord = [pos[0], pos[1]]
#replace with actual starting coordinate
initial_pos = (pos[0], pos[1])
sd.putNumberArray("coord", last_coord)
index = 0
print(last_coord)

while(True):
    traverse_check = sd.getNumber("t_state", 0)

    if traverse_check == 1:
        pos = conn.get_pos()
        curr_path = conn.path_to_line(pos[0], pos[1], END_X)
        
        if(len(curr_path) > 0):
            curr_path.pop(0)
            dist = sqrt(pow(pos[0] - curr_path[0][0], 2) + pow(pos[1] - curr_path[0][1], 2))
            while((dist < 3) and (len(curr_path) > 1)):
                curr_path.pop(0)
                if(len(curr_path) > 0):
                    dist = sqrt(pow(pos[0] - curr_path[0][0], 2) + pow(pos[1] - curr_path[0][1], 2))
                else:
                    break
        

        # print(curr_path)
        # curr_path = PERMANENT_PATH + curr_path
        # at_next_coord = sd.getBoolean("at_pos", False)
        
        at_next_coord = sqrt(pow((pos[0] - curr_coord[0]), 2) + pow((pos[1] - curr_coord[1]), 2)) <= 2
        at_destination = abs(pos[0] - END_X) <= 2
        # print(at_next_coord)
        try:
            if(at_next_coord):
                print(pos)
                print("sent:", curr_path)
                last_coord = sd.getNumberArray("coord", [0, 0])
                curr_coord =  [curr_path[0][0], curr_path[0][1]]
                sd.putNumberArray("coord",curr_coord)
                sd.putBoolean("at_pos", True)
                # sd.putNumberArray("coord", [200, 0])
                print([curr_path[0][0], curr_path[0][1]])
            else:
                sd.putBoolean("at_pos", False)
                
            # at_destination = sd.getBoolean("at_dest", False)        

        except IndexError:
            print("list index out of range")

        if(at_destination):
            print("done")
            f_path = conn.path_to_line(initial_pos[0], initial_pos[1], END_X)
            f_path_x = []
            f_path_y = []
            for point in f_path:
                print(point)
                f_path_x.append(point[0])
                f_path_y.append(point[1])
            sd.putNumberArray("final_path_x", f_path_x)
            sd.putNumberArray("final_path_y", f_path_y)
            sd.putBoolean("at_dest", True)
            print("final path x", f_path_x)
            print("final path y", f_path_y)
            time.sleep(3)
            break
        else:
            sd.putBoolean("at_dest", False)



