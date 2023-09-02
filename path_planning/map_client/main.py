from threading import Thread
from ServerBindingsFactory import ServerBindingFactory
import subprocess
# import PathGUI
# import PathGUI
# from .lidar.HorizontalLidar import HorizontalLidar
# from .lidar.AngledLidar import AngledLidar

# threads = list()
# HorizontalLidarThread = Thread(
#     target=HorizontalLidar.main(), args=(), daemon=True)
# AngledLidarThread = Thread(target=AngledLidar.main(), args=(), daemon=True)

#

ServerBindingFactory.init("10.11.11.3", 8080)


def start_GUI():
    # import PathGUI
    subprocess.Popen(['python', 'pathplanning/PathGUI.py'])


ServerBindingFactory.get_instance().add_obstacle(10, 0, 15, 255, True)
# WeightMap.wm.add_obstacle(20, 0, 15, 255, True)
PathGUIThread = Thread(target=start_GUI, args=(), daemon=True)
PathGUIThread.start()
print("added")

while True:
    pass
