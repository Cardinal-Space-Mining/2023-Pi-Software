#include "Server.hpp"

#include <iostream>
#include <thread>
#include <cinttypes> //PRIu64

#include "ANSIColorCodes.h"
#include "jassert.hpp"

#define UNUSED(x) (void)(x)

ConnectionError::ConnectionError(char const *const message) : std::runtime_error(message){};

char const *ConnectionError::what()
{
    return std::runtime_error::what();
}

// ----------------------------- Server Methods -----------------------------------------

struct sockaddr_in Server::generate_server_address(int PORT) noexcept
{
    struct sockaddr_in SERVER_ADDR;

    memset((void *)&SERVER_ADDR, 0, sizeof(SERVER_ADDR));

    SERVER_ADDR.sin_family = DOMAIN;
    SERVER_ADDR.sin_addr.s_addr = htonl(INADDR_ANY);
    SERVER_ADDR.sin_port = htons(PORT);

    return SERVER_ADDR;
}

Server::Server(int port, mapsize_t x_dim, mapsize_t y_dim, bool verbose, bool log_in, fweight_t allowed_ratio_in) : socket_fd(socket(DOMAIN, TYPE, 0)), wm(x_dim, y_dim), verbose(verbose), log(log_in), running(true), startup_time(time(0)), allowed_ratio(allowed_ratio_in), roll(0), pitch(0), yaw(0)
{

    if (!socket_fd)
    {
        perror("Socket Creation Failed");
        exit(-1);
    }

    struct sockaddr_in SERVER_ADDR = generate_server_address(port);

    if (bind(socket_fd, (struct sockaddr *)&SERVER_ADDR, sizeof(SERVER_ADDR)) < 0)
    {
        perror("First Bind attempt faied");

        // Asking the OS to be more permissive about us rebinding to the address
        const int enable = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
        {
            perror("setsockopt(SO_REUSEADDR) failed");
            close(socket_fd);
            exit(-1);
        }

        // Asking the OS to be more permissive about us rebinding to the port
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) != 0)
        {
            perror("setsockopt(SO_REUSEPORT) failed");
            close(socket_fd);
            exit(-1);
        }

        // The OS acknoleged our request for more permessive socket binding, so try again
        if (bind(socket_fd, (struct sockaddr *)&SERVER_ADDR, sizeof(SERVER_ADDR)) < 0)
        {
            // The OS still does not like us. Sadness
            perror("Fall Back Bind Failed");
            close(socket_fd);
            exit(-1);
        }
    }

    if (listen(socket_fd, QUEUED_CONNECTIONS) != 0)
    {
        perror("Listen Failed");
        close(socket_fd);
        exit(-1);
    }
}

Server::~Server()
{
    if (shutdown(socket_fd, SHUT_WR) != 0)
        perror("SHUT_WR Failed: ");

    if (shutdown(socket_fd, SHUT_RD) != 0)
        perror("SHUT_RD Failed: ");

    close(socket_fd);
}

void Server::recieve_thd()
{
    if (verbose)
        std::cout << "Server recieve Thread Started" << std::endl;

    while (this->running)
    {
        int conn_fd = accept(socket_fd, nullptr, nullptr);

        if (conn_fd == -1)
        { // Error

            if ((errno == EAGAIN) | (errno == EWOULDBLOCK))
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(10ms);
                continue;
            }
            else
            {
                perror("Accept failed with error: ");
            }
        }

        std::thread proc_thd(&Server::process_thd, this, conn_fd);

        if (proc_thd.joinable())
        {
            proc_thd.detach();
        }

        std::cout << "Thread dispatched for new connection: " << conn_fd << std::endl;
    }
    std::cout << "Closing Recieve Thread" << std::endl;
}

void Server::sendResponseAndBuff(const int connection_fd, RESPONSE_HEADER header, const void *buff, size_t len)
{
    char msg_buff[Server::BUFFER_SIZE];
    do
    {
        // Clear Buffer
        memset((void *)msg_buff, 0, Server::BUFFER_SIZE);

        // Determine num bytes to send
        const size_t num_send_bytes = (std::min)(len, Server::BUFFER_SIZE - sizeof(int32_t));

        // If we cannot fit all bytes in this transmission, mark as continue
        if (len > Server::BUFFER_SIZE - sizeof(int32_t))
            ((RESPONSE_HEADER *)msg_buff)[0] = RESPONSE_HEADER::CONTINUE;
        else
            ((RESPONSE_HEADER *)msg_buff)[0] = header;

        // Copy bytes into the payload section
        memcpy((void *)&msg_buff[sizeof(RESPONSE_HEADER)],
               buff,
               num_send_bytes);

        // Move window forward
        len -= num_send_bytes;
        buff = (void *)((size_t)buff + num_send_bytes);

        // Send Bytes
        sendAll(connection_fd, (void *)msg_buff, Server::BUFFER_SIZE);

        // Recieve responce
        recvAll(connection_fd, (void *)msg_buff, Server::BUFFER_SIZE);

        if (((RESPONSE_HEADER *)msg_buff)[0] != RESPONSE_HEADER::ACKNOWLEDGE)
            throw ConnectionError("Client failed to reply correctly");

    } while (len > 0 && buff != nullptr);
}

void Server::recvAll(int conn_fd, void *buff, size_t buff_size)
{

    size_t total_bytes_read = 0;

    // Read in transmission of bytes
    while (total_bytes_read < buff_size)
    {
        ssize_t num_bytes_this_recv = recv(conn_fd, (void *)((size_t)buff + total_bytes_read), buff_size - total_bytes_read, 0);

        if (num_bytes_this_recv == 0) // Handle EOF
        {
            throw ConnectionError("Connection closed by client during recvAll");
        }

        if (num_bytes_this_recv < 0) // Handle Errors
        {
            // Wait for socket to be ready
            if ((errno == EWOULDBLOCK) | (errno == EAGAIN) | (errno == EINTR))
            {
                std::this_thread::yield();
                continue;
            }

            // Terminal Errors
            else if (errno == EBADF)
            {
                throw std::runtime_error("The argument sockfd is an invalid descriptor");
            }
            else if (errno == EFAULT)
            {
                throw std::runtime_error("The receive buffer pointer(s) point outside the process's address space.");
            }
            else if (errno == EINVAL)
            {
                throw std::runtime_error("Invalid argument passed.");
            }
            else if (errno == ENOMEM)
            {
                throw std::bad_alloc();
            }
            else if (errno == ENOTCONN)
            {
                throw std::runtime_error("Socket not connected!");
            }
            else if (errno == ENOTSOCK)
            {
                throw std::runtime_error("Socket argument does not refer to a socket.");
            }
            else if (errno == ECONNREFUSED)
            { // Connection Errors
                throw ConnectionError("Remote host refused to allow the network connection");
            }
            else
            {
                throw std::runtime_error("Nonstandard error in recv");
            }
        }

        total_bytes_read += (size_t)num_bytes_this_recv;
    }
}

void Server::sendAll(int connection_fd, const void *buff, size_t length)
{
    // Send the data
    size_t total_bytes_sent = 0;

    while (total_bytes_sent < length)
    {
        const ssize_t num_bytes_sent = send(connection_fd, (void *)((size_t)buff + total_bytes_sent), length - total_bytes_sent, MSG_NOSIGNAL);
        if (num_bytes_sent < 0)
        { // Error
            if ((errno == EWOULDBLOCK) | (errno == EAGAIN) | (errno == EINTR))
            {
                std::this_thread::yield();
                continue;
            }
            else if (errno == EBADF)
                throw std::invalid_argument("The socket argument is not a valid file descriptor.");

            else if (errno == ECONNRESET)
                throw ConnectionError("A connection was forcibly closed by a peer.");

            else if (errno == EDESTADDRREQ)
                throw ConnectionError("The socket is not connection-mode and no peer address is set.");

            else if (errno == EMSGSIZE)
                throw std::runtime_error("The message is too large to be sent all at once, as the socket requires.");

            else if (errno == ENOTCONN)
                throw ConnectionError("The client is not connected");

            else if (errno == ENOTSOCK)
                throw std::invalid_argument("The socket argument does not refer to a socket.");

            else if (errno == EOPNOTSUPP)
                throw std::invalid_argument("The socket argument is associated with a socket that does not support one or more of the values set in flags.");

            else if (errno == EPIPE)
                throw ConnectionError("The socket is shut down for writing, or the socket is connection-mode and is no longer connected. In the latter case, and if the socket is of type SOCK_STREAM, the SIGPIPE signal is generated to the calling thread.");

            else if (errno == EACCES)
                throw std::runtime_error("The calling process does not have the appropriate privileges.");

            else if (errno == EIO)
                throw std::runtime_error(" An I/O error occurred while reading from or writing to the file system.");

            else if (errno == ENETDOWN)
                throw ConnectionError("The local network interface used to reach the destination is down.");

            else if (errno == ENETUNREACH)
                throw ConnectionError("No route to the network is present.");

            else if (errno == ENOBUFS)
                throw std::bad_alloc();

            else
                throw std::runtime_error("Misc Socket Error");
        }
        else if (num_bytes_sent == 0)
        {
            // Connection closed by client
            throw ConnectionError("Connection closed by client during send");
        }
        else
        {
            total_bytes_sent += (size_t)num_bytes_sent;
        }
    }
}

void Server::sendPath(std::vector<std::pair<mapsize_t, mapsize_t>> &path, int conn_fd)
{
    // Send the points
    //					Num Points	2 ints / pt   size of an mapsize_t (2)	plus a 32 bit int
    size_t buffsize = (path.size() * 2 * sizeof(mapsize_t)) + sizeof(int32_t);
    mapsize_t *buff = (mapsize_t *)new char[buffsize];
    ((int32_t *)buff)[0] = path.size();

    size_t index = 2;
    for (auto &it : path)
    {
        buff[index] = it.first;
        buff[index + 1] = it.second;
        index += 2;
    }

    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, (void *)buff, buffsize);
    delete[] buff;
}

void Server::process_thd(const int conn_fd) noexcept
{
    std::ofstream fout;
    if (this->log)
    {
        char file_name[1028];
        snprintf(file_name, sizeof(file_name), "Intst_%" PRIu64 "_Conn_%d_started at_%" PRIu64 ".txt", (uint64_t)this->startup_time, conn_fd, (uint64_t)time(0));
        fout.open(file_name);
        fout << file_name << std::endl;
    }
    std::array<char, Server::BUFFER_SIZE> arr;

    try
    {
        do
        {
            recvAll(conn_fd, (void *)&arr[0], Server::BUFFER_SIZE);
        } while (this->running && process(arr, conn_fd, fout));
    }
    catch (ConnectionError &e)
    {
        std::cout << ANSIColorCodes::RED << e.what() << '\n'
                  << ANSIColorCodes::RESET;
        if (this->log)
            fout << ANSIColorCodes::RED << e.what() << '\n'
                 << ANSIColorCodes::RESET;
    }
    catch (AssertError &e)
    {
        std::cout << ANSIColorCodes::RED << "In thread servicing connection: " << conn_fd << ' ' << e.what() << '\n'
                  << ANSIColorCodes::RESET;
        if (this->log)
        {
            fout << ANSIColorCodes::RED << "In thread servicing connection: " << conn_fd << ' ' << e.what() << '\n'
                 << ANSIColorCodes::RESET;
            fout.close(); // Save that lovely logged failure :)
        }

        exit(-1);
    }

    close(conn_fd);

    std::cout << "Thread servicing connection " << conn_fd << " terminiating" << std::endl;
    if (this->log)
        fout << "Thread servicing connection " << conn_fd << " terminiating" << std::endl;
    return;
}

bool Server::process(const std::array<char, Server::BUFFER_SIZE> &arr, const int conn_fd, std::ostream &fout)
{
    const CALL_HEADER header = ((const CALL_HEADER *)arr.cbegin())[0];

    const void *const args = (const void *)&((const CALL_HEADER *)arr.cbegin())[1];

    constexpr size_t num_bytes = sizeof(std::array<char, Server::BUFFER_SIZE>) - sizeof(CALL_HEADER);

    if (this->verbose)
        std::cout << ANSIColorCodes::GREEN << "Conn " << conn_fd << ": ";

    if (this->log)
        fout << ANSIColorCodes::GREEN << "Conn " << conn_fd << ": ";

    try
    {
        switch (header)
        {
        case CALL_HEADER::ADD_BORDER:
        {
            Add_Boarder(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::ADD_OBSTACLE:
        {
            Add_Obstacle(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_PATH:
        {
            Get_Path(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::PATH_TO_LINE:
        {
            Path_To_Line(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::PATH_TO:
        {
            Path_To(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_WIDTH:
        {
            Get_Width(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_HEIGHT:
        {

            Get_Height(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_MAX_WEIGHT:
        {
            Get_Max_Weight(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_MIN_WEIGHT:
        {
            Get_Min_Weight(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_MAX_WEIGHT_IN_MAP:
        {
            Get_Max_Weight_In_Map(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::SET_WEIGHT:
        {
            Set_Weight(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_WEIGHT:
        {
            Get_Weight(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::RESET_MAP:
        {
            Reset_Map(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::SET_POS:
        {
            Set_Pos(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_POS:
        {
            Get_Pos(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::CLOSE_SERVER:
        {
            Close_Server(this, args, num_bytes, conn_fd, fout);
            return false;
        }
        case CALL_HEADER::GET_WEIGHTS:
        {
            Get_Weights(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::GET_STRING:
        {
            Get_String(this, args, num_bytes, conn_fd, fout);
            break;
        }
        case CALL_HEADER::DEBUG_PRINT:
        {
            Debug_Print(this, args, num_bytes, conn_fd, fout);
            break;
        }

        default:
        {
            constexpr const char *invalidHeader = "Invalid Call Header";
            sendResponseAndBuff(conn_fd, RESPONSE_HEADER::FAILURE, invalidHeader, strlen(invalidHeader));
            if (this->verbose)
                std::cout << ANSIColorCodes::RED << "Server::InvalidFunction\n"
                          << std::flush;

            if (this->log)
                fout << ANSIColorCodes::RED << "Server::InvalidFunction\n"
                     << std::flush;
        }
        }
    }
    catch (ConnectionError &e)
    {
        throw e; // Client closed early. Shut down this thread
    }
    catch (AssertError &e)
    {
        sendResponseAndBuff(conn_fd, RESPONSE_HEADER::FAILURE, (const void *)e.what(), strlen(e.what()));
        throw e;
    }
    catch (std::exception &e)
    {
        sendResponseAndBuff(conn_fd, RESPONSE_HEADER::FAILURE, (const void *)e.what(), strlen(e.what()));
        std::cout << "Connection: " << conn_fd << " threw an exception with message: " << e.what();
        if (this->log)
            fout << "Connection: " << conn_fd << " threw an exception with message: " << e.what();
    }

    if (this->verbose)
        std::cout << ANSIColorCodes::RESET << std::endl;

    if (this->log)
        fout << ANSIColorCodes::RESET << std::endl;

    return true;
}
// ---------------------------Server Helper Methods --------------------------
std::vector<weight_t> Server::getWeights() const
{

    const std::size_t num_elms = ((size_t)(this->wm.getWidth() * this->wm.getHeight())) + 2;

    std::vector<weight_t> weights(num_elms);

    weights[0] = wm.getWidth();
    weights[1] = wm.getHeight();

    size_t index = 2;

    for (size_t y = 0; y < this->wm.getHeight(); y++)
    {
        for (size_t x = 0; x < this->wm.getWidth(); x++)
        {
            weights[index] = wm.getWeight(x, y);
            index++;
        }
    }
    return weights;
}

std::vector<unsigned char> Server::compressWeights(std::vector<weight_t> &weights)
{
    const size_t input_size = sizeof(weight_t) * weights.size();

    size_t compressed_buff_size = compressBound(input_size);
    std::vector<unsigned char> compressed_buffer(compressed_buff_size);

    int result = compress2(static_cast<Bytef *>(&compressed_buffer[0]), // Destination Buffer
                           &compressed_buff_size,                       // Out variable that holds size of the resulting compressed buffer
                           (Bytef *)&weights[0],                        // Address of buffer to compress
                           input_size,                                  // Size of buffer to compress
                           Z_BEST_COMPRESSION                           // Compression Level
    );
    switch (result)
    {
    case Z_OK:
        // Compression was successful
        break;
    case Z_MEM_ERROR:
        // Not enough memory to allocate for the compression state
        throw std::runtime_error("Z_MEM_ERROR");
    case Z_BUF_ERROR:
        // Output buffer was not large enough to hold the compressed data
        throw std::runtime_error("Z_BUF_ERROR");
    case Z_STREAM_ERROR:
        // Invalid compression level or other error with the compression stream
        throw std::runtime_error("Z_STREAM_ERROR");
    default:
        throw std::runtime_error("InvalidError");
    }

    compressed_buffer.resize(compressed_buff_size);
    return compressed_buffer;
}

// ---------------------------Server Disbatch Methods -----------------------------------
void Server::Add_Boarder(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // assert argument Buffer is big enough
    const int32_t *const arr = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 3;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t boarder_width = arr[0];
    const weight_t weight = arr[1];
    const BoarderPlace place(arr[2]);

    // Print Debug Info
    if (self->verbose)
        std::cout
            << ANSIColorCodes::YELLOW << "Server::Add_Boarder\n"
            << ANSIColorCodes::CYAN << "\tboarder_width: " << ANSIColorCodes::YELLOW << boarder_width << "\n"
            << ANSIColorCodes::CYAN << "\tweight: " << ANSIColorCodes::YELLOW << weight << "\n"
            << ANSIColorCodes::CYAN << "\tplace: " << ANSIColorCodes::YELLOW << place.to_string() << "\n"
            << std::flush;
    if (self->log)
        fout
            << ANSIColorCodes::YELLOW << "Server::Add_Boarder\n"
            << ANSIColorCodes::CYAN << "\tboarder_width: " << ANSIColorCodes::YELLOW << boarder_width << "\n"
            << ANSIColorCodes::CYAN << "\tweight: " << ANSIColorCodes::YELLOW << weight << "\n"
            << ANSIColorCodes::CYAN << "\tplace: " << ANSIColorCodes::YELLOW << place.to_string() << "\n"
            << std::flush;

    // Execute Function Call
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        self->wm.addBoarder(boarder_width, weight, place);
    }

    // Send Server Response
    returnSuccess(conn_fd);
}

void Server::Add_Obstacle(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const args = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 5;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t x = args[0];
    const mapsize_t y = args[1];
    const int radius = args[2];
    const weight_t weight = args[3];
    const bool gradiant = args[4];

    // Print Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Add_Obstacle\n"
                  << ANSIColorCodes::CYAN << "\tX: " << ANSIColorCodes::YELLOW << x << "\n"
                  << ANSIColorCodes::CYAN << "\tY: " << ANSIColorCodes::YELLOW << y << "\n"
                  << ANSIColorCodes::CYAN << "\tRadius: " << ANSIColorCodes::YELLOW << radius << "\n"
                  << ANSIColorCodes::CYAN << "\tWeight: " << ANSIColorCodes::YELLOW << weight << "\n"
                  << ANSIColorCodes::CYAN << "\tGradiant: " << ANSIColorCodes::YELLOW << gradiant << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Add_Obstacle\n"
             << ANSIColorCodes::CYAN << "\tX: " << ANSIColorCodes::YELLOW << x << "\n"
             << ANSIColorCodes::CYAN << "\tY: " << ANSIColorCodes::YELLOW << y << "\n"
             << ANSIColorCodes::CYAN << "\tRadius: " << ANSIColorCodes::YELLOW << radius << "\n"
             << ANSIColorCodes::CYAN << "\tWeight: " << ANSIColorCodes::YELLOW << weight << "\n"
             << ANSIColorCodes::CYAN << "\tGradiant: " << ANSIColorCodes::YELLOW << gradiant << "\n"
             << std::flush;

    // Execute Function Call
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        self->wm.addObstical(x, y, radius, weight, gradiant);
    }

    // Send Server Response
    returnSuccess(conn_fd);
}

void Server::Get_Path(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const args = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 4;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t x1 = args[0];
    const mapsize_t y1 = args[1];
    const mapsize_t xf = args[2];
    const mapsize_t yf = args[3];

    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Path\n"
                  << ANSIColorCodes::RESET
                  << ANSIColorCodes::CYAN << "\tx1: " << ANSIColorCodes::YELLOW << x1 << "\n"
                  << ANSIColorCodes::CYAN << "\ty1: " << ANSIColorCodes::YELLOW << y1 << "\n"
                  << ANSIColorCodes::CYAN << "\txf: " << ANSIColorCodes::YELLOW << xf << "\n"
                  << ANSIColorCodes::CYAN << "\tyf: " << ANSIColorCodes::YELLOW << yf << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Path\n"
             << ANSIColorCodes::RESET
             << ANSIColorCodes::CYAN << "\tx1: " << ANSIColorCodes::YELLOW << x1 << "\n"
             << ANSIColorCodes::CYAN << "\ty1: " << ANSIColorCodes::YELLOW << y1 << "\n"
             << ANSIColorCodes::CYAN << "\txf: " << ANSIColorCodes::YELLOW << xf << "\n"
             << ANSIColorCodes::CYAN << "\tyf: " << ANSIColorCodes::YELLOW << yf << "\n"
             << std::flush;

    std::vector<std::pair<mapsize_t, mapsize_t>> path;
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        path = self->wm.getPath(x1, y1, xf, yf);
    }

    WeightMap::compressPath(path);

    WeightMap::smoothPath(path, self->allowed_ratio);

    sendPath(path, conn_fd);
}

void Server::Path_To_Line(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const args = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 3;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t x1 = args[0];
    const mapsize_t y1 = args[1];
    const mapsize_t xf = args[2];

    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Path_To_Line\n"
                  << ANSIColorCodes::CYAN << "\tx1: " << ANSIColorCodes::YELLOW << x1 << "\n"
                  << ANSIColorCodes::CYAN << "\ty1: " << ANSIColorCodes::YELLOW << y1 << "\n"
                  << ANSIColorCodes::CYAN << "\txf: " << ANSIColorCodes::YELLOW << xf << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Path_To_Line\n"
             << ANSIColorCodes::CYAN << "\tx1: " << ANSIColorCodes::YELLOW << x1 << "\n"
             << ANSIColorCodes::CYAN << "\ty1: " << ANSIColorCodes::YELLOW << y1 << "\n"
             << ANSIColorCodes::CYAN << "\txf: " << ANSIColorCodes::YELLOW << xf << "\n"
             << std::flush;

    // Execute Command
    std::vector<std::pair<mapsize_t, mapsize_t>> path;
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        path = self->wm.pathToXVal(x1, y1, xf);
    }

    WeightMap::compressPath(path);
    WeightMap::smoothPath(path, self->allowed_ratio);

    // Send Response
    sendPath(path, conn_fd);
}

void Server::Path_To(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const args = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 2;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t xf = args[0];
    const mapsize_t yf = args[1];

    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Path_To\n"
                  << ANSIColorCodes::CYAN << "\txf: " << ANSIColorCodes::YELLOW << xf << "\n"
                  << ANSIColorCodes::CYAN << "\tyf: " << ANSIColorCodes::YELLOW << yf << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Path_To\n"
             << ANSIColorCodes::CYAN << "\txf: " << ANSIColorCodes::YELLOW << xf << "\n"
             << ANSIColorCodes::CYAN << "\tyf: " << ANSIColorCodes::YELLOW << yf << "\n"
             << std::flush;

    // Execute Command
    std::vector<std::pair<mapsize_t, mapsize_t>> path;
    {
        // Load Cached Location into local non-volitile memory
        self->pos_mtx.lock();
        mapsize_t x1 = self->pos.first;
        mapsize_t y1 = self->pos.second;
        self->pos_mtx.unlock();
        // Lock Mutex
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        // Compute path
        path = self->wm.getPath(x1, y1, xf, yf);
    }

    WeightMap::compressPath(path);
    WeightMap::smoothPath(path, self->allowed_ratio);

    // Send Response
    sendPath(path, conn_fd);
}

void Server::Get_Width(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Width\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Width\n"
             << std::flush;

    // Execute Command
    int32_t width = self->wm.getWidth();

    // Send Response
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, &width, sizeof(width));
}

void Server::Get_Height(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Height\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Height\n"
             << std::flush;

    // Execute Command
    int32_t height = self->wm.getHeight();

    // Send Response
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, &height, sizeof(height));
}

void Server::Get_Max_Weight(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Max_Weight\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Max_Weight\n"
             << std::flush;

    // Execute Command
    int32_t max_weight = self->wm.getMaxWeight();

    // Send Response
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, &max_weight, sizeof(max_weight));
}

void Server::Get_Min_Weight(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Min_Weight\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Min_Weight\n"
             << std::flush;
    // Execute Command
    int32_t min_weight = self->wm.getMinWeight();

    // Send Response
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, &min_weight, sizeof(min_weight));
}

void Server::Get_Max_Weight_In_Map(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Max_Weight_In_Map\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Max_Weight_In_Map\n"
             << std::flush;
    // Execute Command
    int32_t max_weight;
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        max_weight = self->wm.getMaxWeightInMap();
    }

    // Send Response
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, &max_weight, sizeof(max_weight));
}

void Server::Set_Weight(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const arr = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 3;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t x = arr[0];
    const mapsize_t y = arr[1];
    const weight_t weight = arr[2];

    // Print Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Set_Weight\n"
                  << ANSIColorCodes::CYAN << "\tx: " << ANSIColorCodes::YELLOW << x << "\n"
                  << ANSIColorCodes::CYAN << "\ty: " << ANSIColorCodes::YELLOW << y << "\n"
                  << ANSIColorCodes::CYAN << "\tweight: " << ANSIColorCodes::YELLOW << weight << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Set_Weight\n"
             << ANSIColorCodes::CYAN << "\tx: " << ANSIColorCodes::YELLOW << x << "\n"
             << ANSIColorCodes::CYAN << "\ty: " << ANSIColorCodes::YELLOW << y << "\n"
             << ANSIColorCodes::CYAN << "\tweight: " << ANSIColorCodes::YELLOW << weight << "\n"
             << std::flush;
    // Execute Command
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        self->wm.setWeight(x, y, weight);
    }

    // Respond
    returnSuccess(conn_fd);
}

void Server::Get_Weight(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const arr = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 2;
    jassert(buff_len >= args_size);

    // Parse Out Arguments
    const mapsize_t x = arr[0];
    const mapsize_t y = arr[1];

    // Print Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Weight\n"
                  << ANSIColorCodes::CYAN << "\tx: " << ANSIColorCodes::YELLOW << x << "\n"
                  << ANSIColorCodes::CYAN << "\ty: " << ANSIColorCodes::YELLOW << y << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Weight\n"
             << ANSIColorCodes::CYAN << "\tx: " << ANSIColorCodes::YELLOW << x << "\n"
             << ANSIColorCodes::CYAN << "\ty: " << ANSIColorCodes::YELLOW << y << "\n"
             << std::flush;
    // Execute Command
    int32_t weight = self->wm.getWeight(x, y);

    // Respond
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, (void *)&weight, sizeof(weight));
}

void Server::Reset_Map(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Reset_Map\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Reset_Map\n"
             << std::flush;
    // Execute Command
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);

        // Cache results of fn calls
        const weight_t min_weight = WeightMap::getMinWeight();
        const mapsize_t width = self->wm.getWidth();
        const mapsize_t height = self->wm.getHeight();

        for (fast_mapsize_t x = 0; x < width; x++)
        {
            for (fast_mapsize_t y = 0; y < height; y++)
            {
                self->wm.setWeight(x, y, min_weight);
            }
        }
    }

    // Send Response
    returnSuccess(conn_fd);
}

void Server::Set_Pos(Server *self, const void *buff, size_t buff_len, const int conn_fd, std::ostream &fout)
{
    // jassert argument Buffer is big enough
    const int32_t *const arr = (const int32_t *)buff;
    constexpr size_t args_size = sizeof(int32_t) * 2;
    jassert(buff_len >= args_size);

    // Parse Args
    const int32_t x = arr[0];
    const int32_t y = arr[1];

    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Set_Pos\n"
                  << ANSIColorCodes::CYAN << "\tx: " << ANSIColorCodes::YELLOW << x << "\n"
                  << ANSIColorCodes::CYAN << "\ty: " << ANSIColorCodes::YELLOW << y << "\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Set_Pos\n"
             << ANSIColorCodes::CYAN << "\tx: " << ANSIColorCodes::YELLOW << x << "\n"
             << ANSIColorCodes::CYAN << "\ty: " << ANSIColorCodes::YELLOW << y << "\n"
             << std::flush;
    // Execute Command
    self->pos_mtx.lock();
    self->pos.first = x;
    self->pos.second = y;
    self->pos_mtx.unlock();

    returnSuccess(conn_fd);
}

void Server::Get_Pos(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Pos\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Pos\n"
             << std::flush;
    // Execute Command
    int32_t res[2];
    self->pos_mtx.lock();
    res[0] = self->pos.first;
    res[1] = self->pos.second;
    self->pos_mtx.unlock();

    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, res, sizeof(res));
}

void Server::Close_Server(Server *self, const void *, size_t, const int, std::ostream &fout)
{
    std::cout << ANSIColorCodes::YELLOW << "Server::Close_Server\n"
              << ANSIColorCodes::RESET << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Close_Server\n"
             << ANSIColorCodes::RESET << std::flush;
    self->running = false;
}

void Server::Get_Weights(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Weights\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Weights\n"
             << std::flush;

    // Get the weights
    std::vector<weight_t> weights(self->getWeights());

    // Attempt to compress the weights
    std::vector<unsigned char> compressed_buff = self->compressWeights(weights);

    if (compressed_buff.size() != 0) // If compression was successful
    {
        // Send compressed data
        if (self->verbose)
            std::cout << ANSIColorCodes::CYAN << "\tUncompressed Size: " << weights.size() * sizeof(weight_t) << "\n\tCompressed Size: " << compressed_buff.size() << "\n";
        sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, &compressed_buff[0], compressed_buff.size());
    }
    else // If compression unsuccessful
    {
        constexpr const char *compressionFailureMSG = "Compression Returned Zero Bytes";
        sendResponseAndBuff(conn_fd, RESPONSE_HEADER::FAILURE, compressionFailureMSG, strlen(compressionFailureMSG) + 1);
    }
}

void Server::Get_String(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_String\n"
                  << std::flush;
    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_String\n"
             << std::flush;
    std::string s;
    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        s = self->wm.to_string();
    }
    sendResponseAndBuff(conn_fd, RESPONSE_HEADER::SUCCESS, (const void *)s.c_str(), s.size());
}

void Server::Debug_Print(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Debug_Print\n"
                  << std::flush;

    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Debug_Print\n"
             << std::flush;

    {
        std::lock_guard<decltype(Server::wm_mtx)> lck(self->wm_mtx);
        std::cout << self->wm;
    }
    returnSuccess(conn_fd);
}

void Server::Get_Roll_Pitch_Yaw(Server *self, const void *, size_t, const int conn_fd, std::ostream &fout)
{

    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Get_Roll_Pitch_Yaw\n"
                  << std::flush;

    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Get_Roll_Pitch_Yaw\n"
             << std::flush;
    const double rpy[3] = {self->roll, self->pitch, self->yaw};
    sendResponseAndBuff(conn_fd,RESPONSE_HEADER::SUCCESS, rpy, sizeof(rpy));
}

void Server::Set_Roll_Pitch_Yaw(Server *self , const void * buff, size_t, const int conn_fd, std::ostream &fout)
{
    // Debug Info
    if (self->verbose)
        std::cout << ANSIColorCodes::YELLOW << "Server::Set_Roll_Pitch_Yaw\n"
                  << std::flush;

    if (self->log)
        fout << ANSIColorCodes::YELLOW << "Server::Set_Roll_Pitch_Yaw\n"
             << std::flush;
    const double* rpy = (const double* ) buff;
    self->roll = rpy[0];
    self->pitch = rpy[1];
    self->yaw = rpy[2];
    returnSuccess(conn_fd);

}
