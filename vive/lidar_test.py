from rplidar import RPLidar

def main(): 
    try:
        lidar = RPLidar('/dev/ttyUSB0', baudrate=115200, timeout=3)
        info = lidar.get_info()
        print(info)

        health = lidar.get_health()
        print(health)
        for i, scan in enumerate(lidar.iter_scans()):
            print('%d: Got %d measurments' % (i, len(scan)))
            if i > 10:
                break

    # except RPLidarException as e:
    #     lidar.stop()
    #     lidar.disconnect()
    #     print("RPLidarException: " + str(e))
    #     main()
    except KeyboardInterrupt:
        lidar.stop()
        lidar.stop_motor()
        lidar.disconnect()
    except Exception as e:
        lidar.stop()
        lidar.stop_motor()
        lidar.disconnect()
        print("Exception: " + str(e))
        main()  

if __name__ == "__main__":
    main()

# lidar.stop()
# lidar.stop_motor()
# lidar.disconnect()