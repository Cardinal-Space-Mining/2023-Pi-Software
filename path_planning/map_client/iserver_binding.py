from typing import  Union

from map_util import WeightMapBoarderPlace


class IServerBinding:

    def add_boarder(self, width: int, weight: int, place: Union[int, WeightMapBoarderPlace]) -> None:
        raise RuntimeError("STUB!")

    def add_obstacle(self, x: int, y: int, radius: int, weight: int, gradiant=True) -> None:
        raise RuntimeError("STUB!")

    def get_path(self, src_x: int, src_y: int, dst_x: int, dst_y: int) -> list[tuple[int, int]]:
        raise RuntimeError("STUB!")

    def get_width(self) -> int:
        raise RuntimeError("STUB!")

    def get_height(self) -> int:
        raise RuntimeError("STUB!")

    def get_max_weight(self) -> int:
        raise RuntimeError("STUB!")

    def get_min_weight(self) -> int:
        raise RuntimeError("STUB!")

    def get_max_weight_in_map(self) -> int:
        raise RuntimeError("STUB!")

    def set_weight(self, x: int, y: int, val: int) -> None:
        raise RuntimeError("STUB!")

    def get_weight(self, x: int, y: int) -> int:
        raise RuntimeError("STUB!")

    def close_weight_map(self) -> None:
        raise RuntimeError("STUB!")

    def close_connection(self) -> None:
        raise RuntimeError("STUB!")

    def reset_map(self) -> None:
        raise RuntimeError("STUB!")

    def x_range(self) -> list[int]:
        raise RuntimeError("STUB!")

    def y_range(self) -> list[int]:
        raise RuntimeError("STUB!")

    def get_weights(self) -> list[list[int]]:
        raise RuntimeError("STUB!")

    def to_string(self) -> str:
        raise RuntimeError("STUB!")

    def set_pos(self, x: int, y: int) -> None:
        raise RuntimeError("STUB!")

    def get_pos(self) -> tuple[int, int]:
        raise RuntimeError("STUB!")

    def close(self)->None:
        raise RuntimeError("STUB!")

    def set_weights(self, weights: list[tuple[int,int,int]]) -> None:
        raise RuntimeError("STUB!")

    def path_to_line(self, x0, y0, xf) -> list[tuple[int, int]]:
        raise RuntimeError("STUB!")
    
    def get_roll_pitch_yaw(self) -> tuple[float, float, float]:
        raise RuntimeError("STUB!")

    def set_roll_pitch_yaw(self, roll, pitch, yaw) -> list[tuple[int, int]]:
        raise RuntimeError("STUB!")

