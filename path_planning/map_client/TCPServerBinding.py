import socket
import threading
import zlib
import struct
from fractions import Fraction
from typing import Final, Any, Callable, Union, final

from iserver_binding import IServerBinding
from map_util import WeightMapBoarderPlace, MethodHeaders, ResponseHeader


class TCPServerBinding(IServerBinding):
    """
    A class for interfacing with the PyWeightMapServer
    """

    __SERVER_BUFFER_SIZE: Final[int] = 1024
    """
    The max buffer size in bytes that sockets will send
    """

    __ACK_BUFF: Final[bytes] = int.to_bytes(ResponseHeader.ACKNOWLEDGE, 4, "little", signed=True) + (
            b"\0" * (__SERVER_BUFFER_SIZE - 4))
    """ACK_BUFF contains the bytes the client should use to acknowledge a server response"""

    lock: threading.Lock = threading.Lock()

    @final
    class Encoders:
        """
        Encoder methods take variable arguments and encode them into a byte representation
        """

        @staticmethod
        def none_encoder(*_) -> bytes:
            """Ignores the arguments passed and returns an empty byte string"""
            return b""

        @staticmethod
        def int_cast_encoder(*args) -> bytes:
            """Casts each argument to an integer. Serializes each integer as a 4 byte signed little endian int"""
            bts: bytearray = bytearray()
            for a in args:
                if a is not None:
                    bts.extend(int.to_bytes(int(a), 4, "little", signed=True))
            return bytes(bts)
        
        @staticmethod
        def float_encoder(*args):
            bts: bytearray = bytearray()
            for a in args:
                if a is not None:
                    bts.extend(struct.pack("d", a))
            return bytes(bts)
        

    @final
    class Decoders:
        """
        Decoders take bytes and decode them to python objects
        """

        @staticmethod
        def single_int_decoder(bts: bytes) -> int:
            """Deserializes the first four byte int in the message and returns it"""
            return int.from_bytes(bts[0:4], "little")
        
        @staticmethod
        def tripple_float_decoder(bts: bytes) -> tuple[float,float,float]:
            """Deserializes the first IEEE f64 int in the message and returns it"""
            return struct.unpack("ddd", bts)

        @staticmethod
        def none_decoder(_: bytes) -> None:
            """Ignores the bytes and returns None"""
            return None

        @staticmethod
        def weights_decoder(bts: bytes) -> list[list[int]]:
            """Decodes the weights array"""

            bts = zlib.decompress(bts)

            width: int = int.from_bytes(bts[0: 2], "little", signed=False)
            height: int = int.from_bytes(bts[2: 4], "little", signed=False)

            arr: list[list[int]] = [
                [0 for _ in range(0, height)] for _ in range(0, width)]

            idx: int = 0
            for y in range(0, height):
                for x in range(0, width):
                    arr[x][y] = int.from_bytes(
                        bts[4 + (idx * 2): 6 + (idx * 2)], "little", signed=False)
                    idx += 1
            return arr

        @staticmethod
        def path_decoder(bts: bytes) -> list[tuple[int, int]]:
            """Decodes a path from bytes"""
            num_pts: int = int.from_bytes(bts[0: 4], "little", signed=True)
            numbers = []

            for x in range(0, num_pts * 2):
                val: int = int.from_bytes(
                    bts[4 + (2 * x):6 + (2 * x)], "little", signed=False)
                numbers.append(val)

            return [(numbers[i], numbers[i + 1]) for i in range(0, num_pts * 2, 2)]

        @staticmethod
        def string_decoder(bts: bytes) -> str:
            """Decodes a string from bytes"""
            return bts.decode("ASCII").strip('\0')

        @staticmethod
        def double_int_decoder(bts: bytes) -> tuple[int, int]:
            return int.from_bytes(bts[0: 4], "little", signed=True), int.from_bytes(bts[4:8], "little", signed=True)

    HANDLER_MAP: Final[dict[MethodHeaders, tuple[Callable[..., bytes], Callable[[bytes], Any]]]] = {

        MethodHeaders.SET_WEIGHT: (Encoders.int_cast_encoder, Decoders.none_decoder),
        MethodHeaders.RESET_MAP: (Encoders.none_encoder, Decoders.none_decoder),
        MethodHeaders.ADD_OBSTACLE: (Encoders.int_cast_encoder, Decoders.none_decoder),
        MethodHeaders.ADD_BORDER: (Encoders.int_cast_encoder, Decoders.none_decoder),
        MethodHeaders.CLOSE_CONNECTION: (Encoders.none_encoder, Decoders.none_decoder),
        MethodHeaders.CLOSE_SERVER: (Encoders.none_encoder, Decoders.none_decoder),

        MethodHeaders.GET_WIDTH: (Encoders.none_encoder, Decoders.single_int_decoder),
        MethodHeaders.GET_HEIGHT: (Encoders.none_encoder, Decoders.single_int_decoder),
        MethodHeaders.GET_MIN_WEIGHT: (Encoders.int_cast_encoder, Decoders.single_int_decoder),
        MethodHeaders.GET_MAX_WEIGHT: (Encoders.int_cast_encoder, Decoders.single_int_decoder),
        MethodHeaders.GET_MAX_WEIGHT_IN_MAP: (Encoders.int_cast_encoder, Decoders.single_int_decoder),
        MethodHeaders.GET_WEIGHT: (Encoders.int_cast_encoder, Decoders.single_int_decoder),

        MethodHeaders.GET_WEIGHTS: (Encoders.none_encoder, Decoders.weights_decoder),
        MethodHeaders.GET_PATH: (Encoders.int_cast_encoder, Decoders.path_decoder),
        MethodHeaders.GET_STRING: (Encoders.none_encoder, Decoders.string_decoder),

        MethodHeaders.SET_POS: (Encoders.int_cast_encoder, Decoders.none_decoder),
        MethodHeaders.GET_POS: (Encoders.none_encoder,
                                Decoders.double_int_decoder),

        MethodHeaders.PATH_TO_LINE: (
            Encoders.int_cast_encoder, Decoders.path_decoder),
        MethodHeaders.PATH_TO: (
            Encoders.int_cast_encoder, Decoders.path_decoder),
        MethodHeaders.SET_ROLL_PITCH_YAW: (Encoders.float_encoder, Decoders.none_decoder),
        MethodHeaders.GET_ROLL_PITCH_YAW: (Encoders.none_encoder, Decoders.tripple_float_decoder)
    }
    """The Handler Map maps each method header to a set of encoders and decoders. The encoder takes the arguments 
    the Method call needs, and encodes them to bytes.
    The associated decoder decodes the server's response and turns it into a python object"""

    def _coordinate_convert_to_idx(self, x: int, y: int) -> tuple[int, int]:
        """
        Converts from Sachin coordinates to array indices
        :param x: X coordinate
        :param y: Y coordinate
        :return: Array indices for a weight map
        """

        new_y = int(y + (self.__SERVER_HEIGHT / 2))
        new_x = x

        return new_x, new_y

    def _index_convert(self, x: int, y: int) -> tuple[int, int]:
        """
        Converts from array indices to sachin coordinates
        :param x: X index
        :param y: Y index
        :return: coordinates of x-y index pair
        """

        new_y = int(y - (self.__SERVER_HEIGHT / 2))
        new_x = x

        return new_x, new_y

    def __convert_path_idx_to_coordinates(self, path: list[tuple[int, int]]) -> list[tuple[int, int]]:
        return [self._index_convert(pt[0], pt[1]) for pt in path]

    def __init__(self, ip: str = "localhost", port=8080):
        try:
            self.__SOCKET = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.__SOCKET.connect((ip, port))
        except ConnectionRefusedError as e:
            self.__SOCKET = None
            raise e
        self.__SERVER_HEIGHT = self.get_height()
        self.lock = threading.Lock()

    def __del__(self):
        self.close()

    def add_boarder(self, width: int, weight: int, place: Union[int, WeightMapBoarderPlace]) -> None:
        """
        Adds a boarder to the weight map
        :param width: The width of the boarder
        :param weight: The weight of the boarder
        :param place: Acceptable values are any of WeightMapBoarderPlace.* and derived combinations
        :returns: None
        """
        return self._send_method_call(MethodHeaders.ADD_BORDER, width, weight, place)

    def add_obstacle(self, x: int, y: int, radius: int, weight: int, gradiant=True) -> None:
        """
        Adds an obstacle to the weight map
        :param x: x location of center of obstacle
        :param y: y location of center of obstacle
        :param radius: radius of obstacle
        :param weight: the weight or cost of going to that location
        :param gradiant: if this is true, it linearly decreases the weight as the points grow in distance from the center
        :return:
        """

        new_x, new_y = self._coordinate_convert_to_idx(
            x, y)

        return self._send_method_call(MethodHeaders.ADD_OBSTACLE, new_x, new_y, radius, weight,
                                      gradiant)

    @staticmethod
    def decompress_path(path: list[tuple[int, int]]) -> list[tuple[int, int]]:

        decompressed_path = [path[0]]

        for i in range(1, len(path)):
            curr_point = path[i]
            prev_point = path[i - 1]
            dx = curr_point[0] - prev_point[0]
            dy = curr_point[1] - prev_point[1]
            m = Fraction(dy, dx)
            point = [prev_point[0], prev_point[1]]
            while point[0] != curr_point[0] and point[1] != curr_point[1]:
                point[0] = point[0] + m.denominator
                point[1] = point[1] + m.numerator
                decompressed_path.append(tuple(point))
        return decompressed_path

    def get_path(self, src_x: int, src_y: int, dst_x: int, dst_y: int) -> list[tuple[int, int]]:
        """
        Returns a series of points that describe the least costly path between the two input points
        :param src_x: x location of source point
        :param src_y: y location of source point
        :param dst_x: x location of destination point
        :param dst_y: y location of destination point
        :return: a list of tuples in the form (x,y) so that point at list[a] and the point at list[b] describe one line segment of the path
        """

        new_src_x, new_src_y = self._coordinate_convert_to_idx(
            src_x, src_y)
        new_dst_x, new_dst_y = self._coordinate_convert_to_idx(
            dst_x, dst_y)

        indexes: list[tuple[int, int]] = self._send_method_call(
            MethodHeaders.GET_PATH, new_src_x, new_src_y, new_dst_x,
            new_dst_y,
            True)

        return self.__convert_path_idx_to_coordinates(indexes)

    # Accessors
    def get_width(self) -> int:
        """
        :return: The width of the map
        """
        return self._send_method_call(MethodHeaders.GET_WIDTH)

    def get_height(self) -> int:
        """
        :return: The height of the map
        """
        return self._send_method_call(MethodHeaders.GET_HEIGHT)

    def get_max_weight(self) -> int:
        """
        :return: The max weight that the map can hold
        """
        return self._send_method_call(MethodHeaders.GET_MAX_WEIGHT)

    def get_min_weight(self) -> int:
        """
        :return: The min weight the map can hold
        """
        return self._send_method_call(MethodHeaders.GET_MIN_WEIGHT)

    def get_max_weight_in_map(self) -> int:
        """
        :return: The maximum weight of any node within the map
        """
        return self._send_method_call(MethodHeaders.GET_MAX_WEIGHT_IN_MAP)

    def set_weight(self, x: int, y: int, val: int) -> None:
        """
        Sets the weight at one location in the map
        :param x: x coordinate
        :param y: y coordinate
        :param val: the new weight
        :return: None
        """

        new_x, new_y = self._coordinate_convert_to_idx(
            x, y)

        return self._send_method_call(MethodHeaders.SET_WEIGHT, new_x, new_y, val)

    def get_weight(self, x: int, y: int) -> int:
        """
        Returns the weight at a given point
        :param x: x coordinate
        :param y: y coordinate
        :return: The weight at the given point
        """
        new_x, new_y = self._coordinate_convert_to_idx(
            x, y)
        return self._send_method_call(MethodHeaders.GET_WEIGHT, new_x, new_y)

    def close(self) -> None:
        """
        closes down the wight map sever
        :return: None
        """
        if self.__SOCKET is not None:
            # Send our shutdown notice
            try:
                self.__SOCKET.shutdown(socket.SHUT_WR)
            except OSError:
                pass
            while len(self.__SOCKET.recv(TCPServerBinding.__SERVER_BUFFER_SIZE)) != 0:
                pass
            try:
                self.__SOCKET.shutdown(socket.SHUT_RD)
            except OSError:
                pass
            self.__SOCKET.close()
            self.__SOCKET = None

    def reset_map(self) -> None:
        """
        resets all values in the wight map to the minimum weight
        :return: None
        """
        return self._send_method_call(MethodHeaders.RESET_MAP)

    def x_range(self) -> list[int]:
        """
        Provides a way to easily iterate over all the viable x-points in the map
        :return: a list of all viable x points in the map
        """
        width = self.get_width()

        return [i for i in range(0, width)]

    def y_range(self) -> list[int]:
        """
        Provides a way to easily iterate over all the viable y-points in the map
        :return: a list of all viable y points in the map
        """
        top_left = self._index_convert(0, 0)

        bottom_right = self._index_convert(0, self.__SERVER_HEIGHT)

        return [i for i in range(top_left[1], bottom_right[1] - 1)]

    def __receive_bytes(self) -> bytes:
        # receive reply
        bts = bytearray(b"\00\00\00\00")  # Leave space for final return code
        dat: bytes = bytes()
        header: ResponseHeader = ResponseHeader.CONTINUE

        while header == ResponseHeader.CONTINUE:
            dat = self.__SOCKET.recv(
                TCPServerBinding.__SERVER_BUFFER_SIZE)
            bts.extend(dat[4:])  # Extend byte array with payload bytes
            header = ResponseHeader(int.from_bytes(dat[0:4], "little"))
            self.__SOCKET.sendall(TCPServerBinding.__ACK_BUFF)

        bts[0:4] = dat[0:4]  # Set the final return code

        return bytes(bts)

    def _send_method_call(self, call_type: Union[MethodHeaders, int], *args) -> Any:
        """
        Dispatches a method call to the server with the given call type and attached args
        :param call_type: Any number stored within MessageHeaders
        :param args: The args that call will require on the server side
        :return: The object returned by the server, may be None
        """
        if TCPServerBinding.HANDLER_MAP[call_type] is None:
            raise NotImplementedError("Handler not Implemented")

        encoder: Callable[..., bytes] = TCPServerBinding.HANDLER_MAP[call_type][0]
        decoder: Callable[[bytes], Any] = TCPServerBinding.HANDLER_MAP[call_type][1]

        send_bytes: bytes = int.to_bytes(call_type, 4, "little") + encoder(*args)

        send_bytes = send_bytes + (b"\0" * (TCPServerBinding.__SERVER_BUFFER_SIZE - len(send_bytes)))

        if len(send_bytes) > TCPServerBinding.__SERVER_BUFFER_SIZE:
            raise OverflowError("Too Many Arguments")

        with self.lock:
            self.__SOCKET.sendall(send_bytes)

            recv_bytes: bytes = self.__receive_bytes()

        # Assemble Response Header from first four bytes
        response_code: ResponseHeader = ResponseHeader(
            int.from_bytes(recv_bytes[0:4], "little"))

        # Handle failure
        if response_code != ResponseHeader.SUCCESS:
            try:
                error_msg = TCPServerBinding.Decoders.string_decoder(
                    recv_bytes[4:])
            except UnicodeDecodeError:
                error_msg = "Could not decode error string"

            raise RuntimeError(error_msg)

        # Decode payload bytes
        return decoder(recv_bytes[4:])

    def get_weights(self) -> list[list[int]]:
        """Returns the weights in the weight map so that weights[0][0] is the top left corner"""
        return self._send_method_call(MethodHeaders.GET_WEIGHTS)

    def to_string(self) -> str:
        """Returns a string representation of the weightmap"""
        return self._send_method_call(MethodHeaders.GET_STRING)

    def set_pos(self, x: int, y: int) -> None:
        """Sets the internal position value"""
        x_new, y_new = self._coordinate_convert_to_idx(x, y)
        return self._send_method_call(MethodHeaders.SET_POS, x_new, y_new)

    def close_weight_map(self) -> None:
        """Closes the weight map"""
        return self._send_method_call(MethodHeaders.CLOSE_SERVER)

    def path_to_line(self, x1: int, y1: int, xf: int) -> list[tuple[int, int]]:
        """Calculates and returns a path from the point (x1, y1) to the line xf"""
        x_new, y_new = self._coordinate_convert_to_idx(x1, y1)
        indexes = self._send_method_call(MethodHeaders.PATH_TO_LINE, x_new, y_new, xf)
        return self.__convert_path_idx_to_coordinates(indexes)

    def get_pos(self) -> tuple[int, int]:
        """Returns the value contained within the internal position varriable"""
        x_out, y_out = self._send_method_call(MethodHeaders.GET_POS)
        return self._index_convert(x_out, y_out)
    
    def get_roll_pitch_yaw(self) -> tuple[float, float, float]:
        return self._send_method_call(MethodHeaders.GET_ROLL_PITCH_YAW)

    def set_roll_pitch_yaw(self, roll, pitch, yaw):
        self._send_method_call(MethodHeaders.SET_ROLL_PITCH_YAW, roll, pitch, yaw)

if __name__ == "__main__":
    def __main__():
        """
        A simple example demonstrating how to use the bindings
        prints the size of the map and the map itself
        """
        instance: TCPServerBinding = TCPServerBinding("localhost", 8080)
        for _ in range(0, 10):
            instance.add_boarder(1, 4, WeightMapBoarderPlace.LEFT)

            (instance.get_path(0, 0, 50, 49))
            (instance._coordinate_convert_to_idx(0, 0))
            (instance._index_convert(0, 0))

            # print((instance.get_width(), instance.get_height()), end='\n\n')
            (instance.get_width(), instance.get_height())
            (instance.set_weight(0, 0, 99))
            weights = instance.get_weights()
            print(weights)
            # print(weights, end=', ')

            (instance.set_pos(3, 4))
            (instance.get_pos())
            (instance.set_weight(0, 0, 99))
            (instance.get_weight(0, 0))
            (instance.to_string())
            print(instance.get_path(0, 0, 25, 25))

        instance.close_weight_map()

        instance.close()

        exit(0)


    __main__()
