import os
import sys
import socket
import pygame
from TCPServerBinding import TCPServerBinding
from math import ceil


from pygame.locals import (
    K_ESCAPE,
    KEYDOWN,
)



black = (0, 0, 0)
red = (255, 0, 0)
green = (0, 255, 0)
purple = (255, 0, 255)
cyan = (0, 255, 255)
blue = (0, 0, 255)
dark_green = (0, 125, 0)
white = (255, 255, 255)
ALPHA = (71, 112, 76)

blockSize = 4

SCREEN_WIDTH = 291 * blockSize
SCREEN_HEIGHT = 149 * blockSize

ROBOT_DIAMETER = 33

END_X = 210

conn = TCPServerBinding("10.11.11.130", 8080)

screen = pygame.display.set_mode([SCREEN_WIDTH, SCREEN_HEIGHT])
DEFAULT_IMAGE_SIZE = (ROBOT_DIAMETER, ROBOT_DIAMETER)

pygame.init()

clock = pygame.time.Clock()



# def unpack_path(compressed):
#     path = []
# def draw_path_cheap():
#     pos = ServerBindingFactory.get_instance().get_pos()
#     path = ServerBindingFactory.get_instance().path_to_line(pos[0], pos[1], 269)
#     for x in range(1, len(path)):
#         curr_point = path[x]
#         prev_point = path[x-1]
#         curr_pygame_point = curr_point[0] * blockSize,  ceil(curr_point[1] * blockSize + (SCREEN_HEIGHT / 2))
#         prev_pygame_point = prev_point[0] * blockSize,  ceil(prev_point[1] * blockSize + (SCREEN_HEIGHT / 2))
        
#         pygame.draw.line(screen, green, curr_pygame_point, prev_pygame_point,blockSize)


def unpack_path(compressed):
    path = []

#     for i in range(len(compressed) - 1):
#         if compressed[i][0] == compressed[i+1][0]:
#             pass
#         elif compressed[i][1] == compressed[i+1][1]:
#             pass

PERMANENT_PATH = []
PERMANENT_PATH_COMPLETE = False

REVERSE = True

path = conn.path_to_line(0, 0, END_X)
# print(path)
# Draws the path to destination
def draw_path():
    pos = conn.get_pos()
    if len(PERMANENT_PATH) > 0 and not (PERMANENT_PATH[len(PERMANENT_PATH) - 1][0] == pos[0] and PERMANENT_PATH[len(PERMANENT_PATH) - 1][1] == pos[1]):
        PERMANENT_PATH.append(pos)
    elif len(PERMANENT_PATH) == 0:
        PERMANENT_PATH.append(pos)

    # path = conn.path_to_line(pos[0], pos[1], 269)
    path = conn.path_to_line(pos[0], pos[1], END_X)
    # print(path, pos)

    # if(len(path) > 0):
    #     path.pop(0)

    # print(path)
    
    # path = PERMANENT_PATH + path
    
    for i in range(0, len(path) - 1):
        # rect = pygame.Rect(point[0] * blockSize,
        #                    (point[1] * blockSize + (SCREEN_HEIGHT / 2)), blockSize, blockSize)
        # pygame.draw.rect(screen, green, rect)
        color = green

        # if(len(path[i]) > 2):
        #     color = path[i][2]
        # print(path)
        if REVERSE:
            pygame.draw.line(screen, color, (SCREEN_WIDTH - path[i][0] * blockSize, SCREEN_HEIGHT - ((path[i][1] * blockSize) + (SCREEN_HEIGHT / 2))), 
                             (SCREEN_WIDTH - path[i+1][0] * blockSize, SCREEN_HEIGHT - ((path[i+1][1] * blockSize) + (SCREEN_HEIGHT / 2))), blockSize)
        else:
            pygame.draw.line(screen, color, (path[i][0] * blockSize, (path[i][1] * blockSize) + (SCREEN_HEIGHT / 2)), (path[i+1][0] * blockSize, (path[i+1][1] * blockSize) + (SCREEN_HEIGHT / 2)), blockSize)

    for i in range(0, len(PERMANENT_PATH) - 1):
        # rect = pygame.Rect(point[0] * blockSize,
        #                    (point[1] * blockSize + (SCREEN_HEIGHT / 2)), blockSize, blockSize)
        # pygame.draw.rect(screen, green, rect)
        color = dark_green

        # if(len(path[i]) > 2):
        #     color = path[i][2]
        # print(path)
        if REVERSE:
            pygame.draw.line(screen, color, (SCREEN_WIDTH - PERMANENT_PATH[i][0] * blockSize, SCREEN_HEIGHT - ((PERMANENT_PATH[i][1] * blockSize) + (SCREEN_HEIGHT / 2))), 
                             (SCREEN_WIDTH - PERMANENT_PATH[i+1][0] * blockSize, SCREEN_HEIGHT - ((PERMANENT_PATH[i+1][1] * blockSize) + (SCREEN_HEIGHT / 2))), blockSize)
        else:
            pygame.draw.line(screen, color, (PERMANENT_PATH[i][0] * blockSize, (PERMANENT_PATH[i][1] * blockSize) + (SCREEN_HEIGHT / 2)), (PERMANENT_PATH[i+1][0] * blockSize, (PERMANENT_PATH[i+1][1] * blockSize) + (SCREEN_HEIGHT / 2)), blockSize)




def draw_grid():
    for x in range(0, SCREEN_WIDTH, blockSize):
        for y in range(0, SCREEN_HEIGHT, blockSize):
            rect = pygame.Rect(x, y, blockSize, blockSize)
            pygame.draw.rect(screen, white, rect, 1)


# Draws all weight_map weights from the server
def draw_weights():
    weights = conn.get_weights()
    for x in range(len(weights)):
        for y in range(len(weights[0])):
            if REVERSE:
                rect = pygame.Rect(SCREEN_WIDTH - x * blockSize, SCREEN_HEIGHT - y * blockSize,
                               blockSize, blockSize)
            else:
                rect = pygame.Rect(x * blockSize, y * blockSize,
                               blockSize, blockSize)
            # print(weights[x][y])
            pygame.draw.rect(screen, (weights[x][y], 0, 0), rect)

# Draws the robot position on the GUI
def draw_robot_pos():
    pos = conn.get_pos()
    if REVERSE:
        rect = pygame.Rect(SCREEN_WIDTH - pos[0] * blockSize,
                SCREEN_HEIGHT - (pos[1] * blockSize + (SCREEN_HEIGHT / 2)), blockSize, blockSize)
    else:
        rect = pygame.Rect(pos[0] * blockSize,
                (pos[1] * blockSize + (SCREEN_HEIGHT / 2)), blockSize, blockSize)
    pygame.draw.rect(screen, purple, rect)




def main():
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit(0)
            elif event.type == KEYDOWN:
                if event.key == K_ESCAPE:
                    pygame.quit()
                    sys.exit(0)
        screen.fill(black)
        # drawGrid()
        # drawObjects()
        draw_weights()
        draw_path()
        draw_robot_pos()
        pygame.display.update()


# if __name__ == '__main__':
main()
