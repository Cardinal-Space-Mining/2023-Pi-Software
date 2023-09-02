from typing import final, Union
from iserver_binding import IServerBinding
from TCPServerBinding import TCPServerBinding
import atexit

@final
class ServerBindingFactory:

    __instance: Union[IServerBinding, None] = None
    
    @staticmethod
    def get_instance() -> Union[IServerBinding, None]:
        return ServerBindingFactory.__instance

    @staticmethod
    def init(ip: str, port:int) -> None:
        if ServerBindingFactory.__instance is None:
            ServerBindingFactory.__instance = TCPServerBinding(ip, port)
        
    @staticmethod
    def _close_instance():
        if ServerBindingFactory.__instance is not None:
            ServerBindingFactory.__instance.close()


atexit.register(ServerBindingFactory._close_instance)

if __name__ == "__main__":
    def __main__():
        """
        A simple example demonstrating how to use the bindings
        prints the size of the map and the map itself
        """
        ServerBindingFactory.init("localhost", 8080)

        print(ServerBindingFactory.get_instance().get_path(0, 0, 5, 5))

        print((ServerBindingFactory.get_instance().get_width(), ServerBindingFactory.get_instance().get_height()), end='\n\n')
        weights = ServerBindingFactory.get_instance().get_weights()
        print(weights, end=', ')

        print(ServerBindingFactory.get_instance().to_string())

        ServerBindingFactory.get_instance().close()

        exit(0)

    __main__()