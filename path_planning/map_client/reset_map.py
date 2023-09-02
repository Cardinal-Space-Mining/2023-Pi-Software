from TCPServerBinding import TCPServerBinding
from map_util import WeightMapBoarderPlace


conn = TCPServerBinding("localhost", 8080)

conn.reset_map()
conn.add_boarder(30, 255, WeightMapBoarderPlace.TOP + WeightMapBoarderPlace.BOTTOM)
# conn.add_obstacle(120, 0, 35, 255, True)