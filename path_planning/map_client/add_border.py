from TCPServerBinding import TCPServerBinding
from map_util import WeightMapBoarderPlace

conn = TCPServerBinding("localhost", 8080)

conn.add_boarder(35, 255, WeightMapBoarderPlace.TOP + WeightMapBoarderPlace.BOTTOM)