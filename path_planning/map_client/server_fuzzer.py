import random
from multiprocessing import Pool
from typing import Final, Any

from TCPServerBinding import TCPServerBinding
from map_util import MethodHeaders, WeightMapBoarderPlace

# Server Config Info
ip: Final[str] = "localhost"
port: Final[int] = 8080
wm_width: Final[int] = 270
wm_height: Final[int] = 100
wm_max_weight: Final[int] = 255
wm_min_weight: Final[int] = 1


def gen_rand_point() -> tuple[int, int]:
    return random.randint(0, wm_width - 1), random.randint(0, wm_height - 1)


def gen_rand_weight() -> int:
    return random.randint(wm_min_weight, wm_max_weight)


def gen_rand_call() -> MethodHeaders:
    headers = list(MethodHeaders)
    headers.remove(MethodHeaders.CLOSE_CONNECTION)
    headers.remove(MethodHeaders.CLOSE_SERVER)
    headers.remove(MethodHeaders.DEBUG_PRINT)
    return random.choice(headers)


def gen_rand_place() -> WeightMapBoarderPlace:
    l = list(WeightMapBoarderPlace)
    l.remove(WeightMapBoarderPlace.UNKNOWN)
    return random.choice(l)


def gen_random_args(header: MethodHeaders) -> tuple[Any, ...]:
    match header:
        case MethodHeaders.ADD_BORDER:  # width, weight, place
            place = gen_rand_place()
            weight = gen_rand_weight()
            rand_pt = gen_rand_point()
            if place == WeightMapBoarderPlace.LEFT or place == WeightMapBoarderPlace.RIGHT:
                return rand_pt[0], weight, place
            else:
                return rand_pt[1], weight, place
        case MethodHeaders.ADD_OBSTACLE:  # x, y, radius, weight, gradient
            rand_pt = gen_rand_point()
            radius = gen_rand_point()[0]
            weight = gen_rand_weight()
            gradient = bool(random.getrandbits(1))
            return *rand_pt, radius, weight, gradient
        case MethodHeaders.GET_PATH:
            return *gen_rand_point(), *gen_rand_point()
        case MethodHeaders.GET_WIDTH:
            return None,
        case MethodHeaders.GET_HEIGHT:
            return None,
        case MethodHeaders.GET_MAX_WEIGHT:
            return None,
        case MethodHeaders.GET_MIN_WEIGHT:
            return None,
        case MethodHeaders.GET_MAX_WEIGHT_IN_MAP:
            return None,
        case MethodHeaders.SET_WEIGHT:
            return *gen_rand_point(), gen_rand_weight()
        case MethodHeaders.GET_WEIGHT:
            return gen_rand_point()
        case MethodHeaders.RESET_MAP:
            return None,
        case MethodHeaders.GET_WEIGHTS:
            return None,
        case MethodHeaders.GET_STRING:
            return None,
        case MethodHeaders.SET_POS:
            return gen_rand_point()
        case MethodHeaders.GET_POS:
            return None,
        case MethodHeaders.DEBUG_PRINT:
            return None,
        case MethodHeaders.PATH_TO:
            return gen_rand_point()
        case MethodHeaders.PATH_TO_LINE:
            return *gen_rand_point(), gen_rand_point()[0]
        case MethodHeaders.CLOSE_CONNECTION:
            return None,
        case MethodHeaders.CLOSE_SERVER:
            return None,


def fuzz_call(conn: TCPServerBinding, call: MethodHeaders = None):
    if call is None:
        call = gen_rand_call()
    conn._send_method_call(call, *gen_random_args(call))


def fuzz_call_proc(call: MethodHeaders = None):
    conn = TCPServerBinding(ip, port)
    while True:
        fuzz_call(conn, call)

def test_open_close_conn(_)->None:
    while True:
        _ = TCPServerBinding(ip, port)


def main():
    # Process Pool Stuff
    num_procs = 8
    call: Final[MethodHeaders | None] = None

    if num_procs == 1:
        fuzz_call_proc(call)

    else:
        with Pool(num_procs) as p:
            p.map(fuzz_call_proc, (call for _ in range(0, num_procs)))


if __name__ == "__main__":
    main()
