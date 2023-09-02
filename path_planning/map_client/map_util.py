from enum import IntEnum
from typing import final


@final
class WeightMapBoarderPlace(IntEnum):
    TOP = 1,
    BOTTOM = 2,
    RIGHT = 4,
    LEFT = 8,
    UNKNOWN = 16


@final
class MethodHeaders(IntEnum):
    ADD_BORDER = 1
    ADD_OBSTACLE = 2
    GET_PATH = 3
    GET_WIDTH = 4
    GET_HEIGHT = 5
    GET_MAX_WEIGHT = 6
    GET_MIN_WEIGHT = 7
    GET_MAX_WEIGHT_IN_MAP = 8
    SET_WEIGHT = 9
    GET_WEIGHT = 10
    RESET_MAP = 11
    GET_WEIGHTS = 12
    GET_STRING = 13
    SET_POS = 14    
    GET_POS = 15
    DEBUG_PRINT = 16
    PATH_TO = 17
    PATH_TO_LINE = 18
    GET_ROLL_PITCH_YAW = 19
    SET_ROLL_PITCH_YAW = 20
    CLOSE_CONNECTION = 999
    CLOSE_SERVER = 1000 
    
    def __str__(self):
        if self.value == MethodHeaders.ADD_BORDER:
            return "ADD_BOARDER"
        if self.value == MethodHeaders.ADD_OBSTACLE:
            return "ADD_OBSTACLE"
        if self.value == MethodHeaders.GET_PATH:
            return "GET_PATH"
        if self.value == MethodHeaders.GET_WIDTH:
            return "GET_WIDTH"
        if self.value == MethodHeaders.GET_HEIGHT:
            return "GET_HEIGHT"
        if self.value == MethodHeaders.GET_MAX_WEIGHT:
            return "GET_MAX_WEIGHT"
        if self.value == MethodHeaders.GET_MAX_WEIGHT_IN_MAP:
            return "GET_MAX_WEIGHT_IN_MAP"
        if self.value == MethodHeaders.SET_WEIGHT:
            return "SET_WEIGHT"
        if self.value == MethodHeaders.GET_WEIGHT:
            return "GET_WEIGHT"
        if self.value == MethodHeaders.RESET_MAP:
            return "RESET_MAP"
        if self.value == MethodHeaders.CLOSE_CONNECTION:
            return "CLOSE_CONNECTION"
        if self.value == MethodHeaders.CLOSE_SERVER:
            return "CLOSE_SERVER"
        if self.value == MethodHeaders.GET_WEIGHTS:
            return "GET_WEIGHTS"
        if self.value == MethodHeaders.GET_STRING:
            return "GET_STRING"
        if self.value == MethodHeaders.SET_POS:
            return "SET_POS"
        if self.value == MethodHeaders.GET_POS:
            return "GET_POS"
        if self.value == MethodHeaders.GET_ROLL_PITCH_YAW:
            return "GET_ROLL_PITCH_YAW"
        if self.value == MethodHeaders.SET_ROLL_PITCH_YAW:
            return "SET_ROLL_PITCH_YAW"
        # if self.value == MethodHeaders.SET_ANGLE:
        #     return "SET_ANGLE"
        # if self.value == MethodHeaders.GET_ANGLE:
        #     return "GET_ANGLE"
        return "UNKNOWN"


@final
class ResponseHeader(IntEnum):
    SUCCESS = 0
    FAILURE = 1
    CONTINUE = 3
    ACKNOWLEDGE = 4
    SUCCESS_COMPRESSED = 5