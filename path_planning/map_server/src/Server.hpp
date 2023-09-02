#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>

#include <mutex>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <cmath>
#include <fstream>
#include <ctime>

#include <vector>

#include "WeightMap.hpp"

class ConnectionError : public std::runtime_error
{
public:
	ConnectionError(char const *const message);
	virtual char const *what();
};

enum CALL_HEADER : int32_t
{
	ADD_BORDER = 1,
	ADD_OBSTACLE = 2,
	GET_PATH = 3,
	GET_WIDTH = 4,
	GET_HEIGHT = 5,
	GET_MAX_WEIGHT = 6,
	GET_MIN_WEIGHT = 7,
	GET_MAX_WEIGHT_IN_MAP = 8,
	SET_WEIGHT = 9,
	GET_WEIGHT = 10,
	RESET_MAP = 11,
	GET_WEIGHTS = 12,
	GET_STRING = 13,
	SET_POS = 14,
	GET_POS = 15,
	DEBUG_PRINT = 16,
	PATH_TO = 17,
	PATH_TO_LINE = 18,
	GET_ROLL_PITCH_YAW = 19,
	SET_ROLL_PITCH_YAW = 20,
	CLOSE_CONNECTION = 999,
	CLOSE_SERVER = 1000
};

constexpr const char *to_string(CALL_HEADER h)
{
	switch (h)
	{
	case CALL_HEADER::ADD_BORDER:
		return "ADD_BORDER";
	case CALL_HEADER::ADD_OBSTACLE:
		return "ADD_OBSTACLE";
	case CALL_HEADER::GET_PATH:
		return "GET_PATH";
	case CALL_HEADER::GET_WIDTH:
		return "GET_WIDTH";
	case CALL_HEADER::GET_HEIGHT:
		return "GET_HEIGHT";
	case CALL_HEADER::GET_MAX_WEIGHT:
		return "GET_MAX_WEIGHT";
	case CALL_HEADER::GET_MIN_WEIGHT:
		return "GET_MIN_WEIGHT";
	case CALL_HEADER::GET_MAX_WEIGHT_IN_MAP:
		return "GET_MAX_WEIGHT_IN_MAP";
	case CALL_HEADER::SET_WEIGHT:
		return "SET_WEIGHT";
	case CALL_HEADER::GET_WEIGHT:
		return "GET_WEIGHT";
	case CALL_HEADER::RESET_MAP:
		return "RESET_MAP";
	case CALL_HEADER::GET_WEIGHTS:
		return "GET_WEIGHTS";
	case CALL_HEADER::SET_POS:
		return "SET_POS";
	case CALL_HEADER::GET_POS:
		return "GET_POS";
	case CALL_HEADER::CLOSE_CONNECTION:
		return "CLOSE_CONNECTION";
	case CALL_HEADER::CLOSE_SERVER:
		return "CLOSE_SERVER";
	case CALL_HEADER::GET_STRING:
		return "GET_STRING";
	case CALL_HEADER::DEBUG_PRINT:
		return "DEBUG_PRINT";
	case CALL_HEADER::PATH_TO:
		return "PATH_TO";
	case CALL_HEADER::PATH_TO_LINE:
		return "PATH_TO_LINE";
	default:
		return "INVALID";
	}
}

enum class RESPONSE_HEADER : int32_t
{
	SUCCESS = 0,
	FAILURE = 1,
	CONTINUE = 3,
	ACKNOWLEDGE = 4,
};

class Server
{
public: // Public Constructors/destructors
	Server(int port, mapsize_t x_dim, mapsize_t y_dim, bool verbose = false, bool log = false, const fweight_t allowed_ratio = .95);

	~Server();

public: // public methods

	inline void run()
	{
		this->recieve_thd();
	};

private:													 // private members
	static constexpr int QUEUED_CONNECTIONS = 255;			 // How many connections the linux kernal will queue for us
	static constexpr int DOMAIN = AF_INET;					 // IPV4
	static constexpr int TYPE = SOCK_STREAM | SOCK_NONBLOCK; // TCP and non-blocking socket calls
	static constexpr int PROTOCOL = 0;						 // Nothing Special
	static constexpr size_t BUFFER_SIZE = 1024;				 // The number of bytes the server will send and recieve

	// Member Variables
	const int socket_fd; // Socket file descriptor

	WeightMap wm;	   // Weight Map
	std::mutex wm_mtx; // Mutex for the weight map

	volatile std::pair<mapsize_t, mapsize_t> pos; // Robot's position in inches
	std::mutex pos_mtx;							  // Mutex for position

	const bool verbose;				   // Toggles verbose debug messages
	const bool log;					   // Toggles per thread log files
	volatile std::atomic_bool running; // Condition variable for if the server is running

	const time_t startup_time;

	const fweight_t allowed_ratio;

	double roll;
	double pitch;
	double yaw;

private: // Private methods in responce to various CALL_HEADER's
	std::vector<weight_t> getWeights() const;

	std::vector<unsigned char> compressWeights(std::vector<weight_t> &weights);

private:
	static void Add_Boarder(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Add_Obstacle(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Path(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Path_To_Line(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Path_To(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Width(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Height(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Max_Weight(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Min_Weight(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Max_Weight_In_Map(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Weight(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Set_Weight(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Reset_Map(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Set_Pos(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Pos(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Close_Server(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Weights(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_String(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Debug_Print(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Get_Roll_Pitch_Yaw(Server *, const void *, size_t, const int, std::ostream &fout);

	static void Set_Roll_Pitch_Yaw(Server *, const void *, size_t, const int, std::ostream &fout);

private: // Private utility functions
	static void sendPath(std::vector<std::pair<mapsize_t, mapsize_t>> &path, int conn_fd);

	static void recvAll(int conn_fd, void *buff, size_t buff_size);

	static void sendAll(int connection_fd, const void *buff, size_t length);

	static void sendResponseAndBuff(const int connection_fd, RESPONSE_HEADER header, const void *buff, size_t len);

	static inline void returnSuccess(const int connection_fd)
	{
		sendResponseAndBuff(connection_fd, RESPONSE_HEADER::SUCCESS, nullptr, 0);
	};

	static inline void returnFailure(const int connection_fd)
	{
		sendResponseAndBuff(connection_fd, RESPONSE_HEADER::FAILURE, nullptr, 0);
	};

private: // private methods
	void process_thd(const int conn_fd) noexcept;

	bool process(const std::array<char, Server::BUFFER_SIZE> &arr, const int conn_fd, std::ostream &fout);

	void recieve_thd();

	static struct sockaddr_in generate_server_address(int PORT) noexcept;
};
