#include <iostream> //std::cout, std::endl
#include <cstdlib>	//atoi
#include <cstring>	//strcmp

#include "Server.hpp" //Server

constexpr const char *helpMsg = "Usage: weightMap [options...]\n"
								"\t-p, --port <port>          Selects port for the server (required)\n"
								"\t-w, --width <width>        Sets the map width (required)\n"
								"\t-h, --height <height>      Sets map height (required)\n"
								"\t-v, --verbose              Prints extra information to the commandline (optional)\n"
								"\t-l, --log                  Toggles the printing of extra information to files. Files named in the form {Intst_%a_Conn_%b_started at_%c.txt} where %a denotes the time the server started, %b denotes the connection, and %c denotes the time the connection was opened. One file per connection . (use cat to view)\n"
								"\t-t, --thresh <value>       Sets the threshold for path smoothing to value\n"
								"\t/?, --help                 Show this help menu and exit\n";

int argParse(const char *arg, int argc, const char *const *argv);
bool hasArg(const char *arg, const int argc, const char *const *argv);
float argParsef(const char *arg, int argc, const char *const *argv);

bool hasArg(const char *arg, const int argc, const char *const *argv)
{
	for (size_t i = 0; i < static_cast<size_t>(argc); i++)
	{
		if (strcmp(arg, argv[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

// Parses for the value of the tag. Assuming format follows [tag] [value]
int argParse(const char *arg, int argc, const char *const *argv)
{
	for (size_t i = 0; i < (size_t)argc; i++)
	{
		if (strcmp(arg, argv[i]) == 0)
		{

			// Handle case of trailing tag
			if (i + 1 == static_cast<size_t>(argc))
			{
				return 0;
			}

			// Parse the value to an int
			return atoi(argv[i + 1]);
		}
	}

	return 0;
}

// Parses for the value of the tag. Assuming format follows [tag] [value]
float argParsef(const char *arg, int argc, const char *const *argv)
{
	for (size_t i = 0; i < (size_t)argc; i++)
	{
		if (strcmp(arg, argv[i]) == 0)
		{

			// Handle case of trailing tag
			if (i + 1 == static_cast<size_t>(argc))
			{
				return 0.0;
			}

			// Parse the value to an int
			return atof(argv[i + 1]);
		}
	}

	return 0;
}

int main(const int argc, const char *const *argv)
{
	if (argc == 1)
	{
		std::cout << helpMsg;
		std::cout << "No command line args supplied" << std::endl;
		return -1;
	}

	// Help Message
	if (hasArg("/?", argc, argv) || hasArg("--help", argc, argv))
	{
		std::cout << helpMsg;
		return 0;
	}

	// Arg Parse height
	const int heightValue = (std::max)(argParse("-h", argc, argv), argParse("--height", argc, argv));
	if (heightValue <= 0)
	{
		std::cout << "Height not supplied or invalid" << std::endl;
		return -1;
	}

	// Arg Parse width
	const int widthValue = (std::max)(argParse("-w", argc, argv), argParse("--width", argc, argv));
	if (widthValue <= 0)
	{
		std::cout << "Width not supplied or invalid" << std::endl;
		return -1;
	}

	// Arg Parse width
	const int port = (std::max)(argParse("-p", argc, argv), argParse("--port", argc, argv));
	if (port <= 0 || port > (std::numeric_limits<uint16_t>::max)() || port < 1028)
	{
		std::cout << "Port not supplied, invalid, or reserved by OS." << std::endl;
		return -1;
	}

	fweight_t allowed_threshold = (std::max)(argParsef("-t", argc, argv), argParsef("--threshold", argc, argv));
	std::cout << allowed_threshold;
	if (allowed_threshold <= 0.0f || allowed_threshold > 1.0f )
	{
		allowed_threshold = .95f;
	}

	try
	{
		// Construct the Server
		Server s(port, widthValue, heightValue, hasArg("-v", argc, argv) | hasArg("--verbose", argc, argv), hasArg("-l", argc, argv) | hasArg("--log", argc, argv), allowed_threshold);

		// Debug Info
		std::cout << "Server created on port: " << port << ". Size: (" << widthValue << ", " << heightValue << "), Threshold: " << allowed_threshold << std::endl;

		// Run the server
		s.run();
	}
	catch (std::exception &e)
	{
		std::cout << "FATAL ERROR: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
