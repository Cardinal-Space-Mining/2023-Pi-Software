from TCPServerBinding import TCPServerBinding

conn = TCPServerBinding("localhost", 8080)

conn.close()