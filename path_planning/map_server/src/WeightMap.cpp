// Project Includes
#include "WeightMap.hpp"

#include <limits>	 //std::numeric_limits
#include <algorithm> //std::min
#include <stdexcept> //std::out_of_range, std::invalid_argument
#include <sstream>	 //std::stringstream
#include <iomanip>	 //std::setw
#include <algorithm> //std::reverse
#include <cmath>	 //std::sqrt
#include <array>
#include <functional>

#include "jassert.hpp"

// Node Constructor
Node::Node(const mapsize_t x_in,
		   const mapsize_t y_in,
		   const mapsize_t parent_x_in,
		   const mapsize_t parent_y_in,
		   const Color color_in,
		   const weight_t weight_in,
		   const fweight_t costFromSrc_in) : x(x_in), y(y_in), parent_x(parent_x_in), parent_y(parent_y_in),
											 color(color_in), weight(weight_in), costFromSrc(costFromSrc_in){};

//---------------	Boarder Place Methods	----------------------------

const BoarderPlace BoarderPlace::TOP(1);
const BoarderPlace BoarderPlace::BOTTOM(2);
const BoarderPlace BoarderPlace::RIGHT(4);
const BoarderPlace BoarderPlace::LEFT(8);
const BoarderPlace BoarderPlace::UNKNOWN(16);

uint8_t BoarderPlace::get_value() const { return value; }

bool BoarderPlace::isValid() const
{
	return value < BoarderPlace::UNKNOWN.value;
}

BoarderPlace BoarderPlace::operator+(const BoarderPlace &b) const
{

	return BoarderPlace(b.value + this->value);
}

const char *BoarderPlace::to_string() const
{
	if (this->value == TOP.value)
	{
		return "TOP";
	}
	if (this->value == BOTTOM.value)
	{
		return "BOTTOM";
	}
	if (this->value == RIGHT.value)
	{
		return "RIGHT";
	}
	if (this->value == LEFT.value)
	{
		return "LEFT";
	}

	if (this->value >= BoarderPlace::UNKNOWN.value)
	{
		return "UNKNOWN";
	}

	return "COMBO";
}

bool BoarderPlace::contains(const BoarderPlace &other)
{
	return (this->get_value() & other.get_value()) == other.get_value();
}

//---------------	WeightMap Methods	----------------------------
WeightMap::WeightMap(mapsize_t width, mapsize_t height) : width(width), height(height), arr(width, height)
{
	// Populate nodes
	for (fast_mapsize_t x = 0; x < width; x++)
	{
		for (fast_mapsize_t y = 0; y < height; y++)
		{
			new (&arr[x][y]) Node(x, y, std::numeric_limits<mapsize_t>::max(),
								  std::numeric_limits<mapsize_t>::max(),
								  Color::BLACK,
								  1,
								  std::numeric_limits<decltype(Node::costFromSrc)>::infinity());
		}
	}
}

weight_t WeightMap::getMaxWeightInMap() const
{
	weight_t curr_max = std::numeric_limits<weight_t>::min();

	for (fast_mapsize_t x = 0; x < this->width; x++)
	{
		for (fast_mapsize_t y = 0; y < this->height; y++)
		{
			curr_max = std::max(arr[x][y].weight, curr_max);
		}
	}
	return curr_max;
}

weight_t WeightMap::getWeight(mapsize_t x, mapsize_t y) const
{
	if (!isValidPoint(x, y))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Point (%u, %u) out of bounds!", x, y);
		throw std::invalid_argument(std::string(error_message));
	}

	return arr[x][y].weight;
}

void WeightMap::setWeight(mapsize_t x, mapsize_t y, weight_t weight)
{
	if (!isValidPoint(x, y))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Point (%u, %u) out of bounds!", x, y);
		throw std::invalid_argument(std::string(error_message));
	}
	if (weight > this->getMaxWeight())
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Weight %u is greater than the max weight %u!", weight, this->getMaxWeight());
		throw std::invalid_argument(std::string(error_message));
	}

	arr[x][y].weight = weight;
}

std::string WeightMap::to_string(bool extraData) const
{

	std::stringstream ss;

	if (extraData)
	{
		ss << *this;
	}
	else
	{

		static const size_t max_width = std::to_string((int)std::numeric_limits<weight_t>::max()).size();

		for (fast_mapsize_t y = 0; y < height; y++)
		{
			for (fast_mapsize_t x = 0; x < width; x++)
			{
				ss << std::setw(max_width) << std::setfill(' ') << arr[x][y].weight << ' ';
			}
			ss << '\n';
		}
	}

	return ss.str();
}

namespace WeightMapHelper
{										// Internal Linkage please
	constexpr fweight_t SQRT_2 = 1.42f; // Slightly more than actual sqrt2
	constexpr DijkstrasMove moves[] = {
		{0, 1, 1},
		{-1, 0, 1},
		{0, -1, 1},
		{1, 0, 1},
		{1, 1, SQRT_2},
		{-1, 1, SQRT_2},
		{-1, -1, SQRT_2},
		{1, -1, SQRT_2}};
	constexpr size_t numMoves = sizeof(moves) / sizeof(moves[0]);
}

WeightMap::path_t WeightMap::pathToXVal(mapsize_t srcX, mapsize_t srcY, mapsize_t dstX)
{

	if (!isValidPoint(srcX, srcY))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Source Point (%u, %u) out of bounds!", srcX, srcY);
		throw std::invalid_argument(std::string(error_message));
	}

	if (dstX >= this->width)
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "X value to travel to {%u} out of bounds!", dstX);
		throw std::invalid_argument(std::string(error_message));
	}

	Node &src = this->arr[srcX][srcY];

	this->resetMap();

	src.parent_x = std::numeric_limits<mapsize_t>::max();
	src.parent_y = std::numeric_limits<mapsize_t>::max();
	src.color = Color::RED;
	src.costFromSrc = 0;

	std::vector<Node *> queue_backer;
	queue_backer.reserve(this->width * this->height);
	WeightMap::queue_t q(NodeCmp(), std::move(queue_backer));
	q.push(&src);
	while (!q.empty())
	{
		// Taking a reference from a queue and destroying it is ok because the Node does not live in the queue,
		//  Rather, the queue holds references to a Node held by the weightmap, and we are copying out that reference
		//  And then removing the reference object from the queue
		Node &currentNode = *q.top();
		q.pop();

		using namespace WeightMapHelper;
		dijkstrasMakeMoves(moves, numMoves, currentNode, q);

		if (dstX <= currentNode.x)
		{
			return generate_path(src, currentNode);
		}

		currentNode.color = Color::GREEN;
	}

	// We should not reach here. The if statement in the while loop should have triggered on the first node with the propper x-value
	jassert(false);
}

WeightMap::path_t WeightMap::generate_path(const Node &src, const Node &dst) const
{
	path_t path;

	const Node *currentNode = &dst;
	while (isValidPoint(currentNode->parent_x, currentNode->parent_y))
	{
		path.emplace_back(currentNode->x, currentNode->y);
		currentNode = &this->arr[currentNode->parent_x][currentNode->parent_y];
	}
	jassert(path.size() <= (this->height * this->width));
	path.emplace_back(currentNode->x, currentNode->y);
	std::reverse(path.begin(), path.end());
	jassert(path[0].first == src.x && path[0].second == src.y);
	return path;
}

WeightMap::path_t WeightMap::getPath(mapsize_t srcX, mapsize_t srcY, mapsize_t dstX, mapsize_t dstY)
{
	if (!isValidPoint(srcX, srcY))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Source Point (%u, %u) out of bounds!", srcX, srcY);
		throw std::invalid_argument(std::string(error_message));
	}
	if (!isValidPoint(dstX, dstY))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Destination Point (%u, %u) out of bounds!", dstX, dstY);
		throw std::invalid_argument(std::string(error_message));
	}

	if (srcX == dstX && srcY == dstY)
	{
		return {{srcX, srcY}};
	}

	Node &src = this->arr[srcX][srcY];
	Node &dst = this->arr[dstX][dstY];

	this->resetMap();

	src.parent_x = std::numeric_limits<mapsize_t>::max();
	src.parent_y = std::numeric_limits<mapsize_t>::max();
	src.color = Color::RED;
	src.costFromSrc = 0;

	std::vector<Node *> queue_backer;
	queue_backer.reserve(this->width * this->height);
	WeightMap::queue_t q(NodeCmp(), std::move(queue_backer));
	q.push(&src);
	while (!q.empty())
	{
		Node &currentNode = *q.top();
		q.pop();

		if (currentNode.x == dst.x && currentNode.y == dst.y)
		{
			break;
		}

		using namespace WeightMapHelper;
		dijkstrasMakeMoves(moves, numMoves, currentNode, q);

		currentNode.color = Color::GREEN;
	}

	// Generate Path

	return generate_path(src, dst);
}

/*
O(N) runtime, O(1) memory useage
*/
void WeightMap::compressPath(path_t &path)
{
	if (path.size() < 3)
	{
		return;
	}

	size_t book_mark = 1;

	for (fast_mapsize_t idx = 1; idx < path.size()-1; idx++)
	{
		const int8_t b_dx = path[idx].first - path[idx - 1].first;
		const int8_t b_dy = path[idx].second - path[idx - 1].second;
		const int8_t f_dx = path[idx+1].first - path[idx].first;
		const int8_t f_dy = path[idx+1].second - path[idx].second;
		if (b_dx != f_dx || f_dy != b_dy)
		{
			path[book_mark] = path[idx];
			book_mark++;
		}
	}
	path[book_mark] = path[path.size() - 1];

	path.resize(book_mark + 1);
}

void WeightMap::smoothPath(WeightMap::path_t &path, fweight_t allowed_ratio)
{
	size_t delta_size = 99999;
	size_t start_size;
	while (delta_size != 0)
	{
		start_size = path.size();
		for (size_t i = 1; i < path.size() - 1; i++)
		{
			const fweight_t ratio = WeightMap::length_ratio(path[i - 1], path[i], path[i + 1]);
			if (ratio > allowed_ratio)
			{
				path.erase(path.begin() + (path_t::difference_type)i);
			}
		}
		delta_size = start_size - path.size();
	}
}


void WeightMap::theta_smooth_path(WeightMap::path_t &path)
{
	size_t delta_size = 99999;
	
	while (delta_size != 0)
	{
		const size_t start_size = path.size();
		for (size_t i = 1; i < path.size() - 1; i++)
		{
			Node &a = arr[path[i - 1].first][path[i - 1].second];
			Node &c = arr[path[i + 1].first][path[i + 1].second];
			const float direct_cost = this->get_linear_cost(a, c);
			const float new_cost = direct_cost + a.costFromSrc;
			if (new_cost < c.costFromSrc)
			{
				//Remove Node B from path
				path.erase(path.begin() + i);

				//Update costs from source to reflect change
				const float cost_difference = c.costFromSrc - new_cost;
				for (size_t a = i; a < path.size(); a++)
				{
					arr[path[a].first][path[a].second].costFromSrc -= cost_difference;
				}
				break;
			}
		}
		delta_size = start_size - path.size();
	}
}

void WeightMap::addBoarder(mapsize_t boarder_width, weight_t boarder_weight, BoarderPlace place)
{

	if (!WeightMap::isValidWeight(boarder_weight))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Weight {%u} out of bounds!", boarder_weight);
		throw std::invalid_argument(std::string(error_message));
	}

	if (!place.isValid())
	{

		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Place {0x%x} is invalid!", place.get_value());
		throw std::invalid_argument(std::string(error_message));
	}

	if ((place.contains(BoarderPlace::TOP) || place.contains(BoarderPlace::BOTTOM)) && boarder_width > height)
	{
		throw std::invalid_argument("Boarder Width is greater than the height of the board");
	}

	if ((place.contains(BoarderPlace::RIGHT) || place.contains(BoarderPlace::LEFT)) && boarder_width > width)
	{
		throw std::invalid_argument("Boarder Width is greater than the widtg of the board");
	}

	if (place.contains(BoarderPlace::TOP))
	{
		for (fast_mapsize_t y = 0; y < boarder_width; y++)
		{
			for (fast_mapsize_t x = 0; x < width; x++)
			{
				if (isValidPoint(x, y))
				{
					arr[x][y].weight = boarder_weight;
				}
			}
		}
	}

	if (place.contains(BoarderPlace::BOTTOM))
	{
		for (fast_mapsize_t y = height - boarder_width; y < height; y++)
		{
			for (fast_mapsize_t x = 0; x < width; x++)
			{
				arr[x][y].weight = boarder_weight;
			}
		}
	}

	if (place.contains(BoarderPlace::RIGHT))
	{
		for (fast_mapsize_t x = width - boarder_width; x < width; x++)
		{
			for (fast_mapsize_t y = 0; y < height; y++)
			{
				arr[x][y].weight = boarder_weight;
			}
		}
	}

	if (place.contains(BoarderPlace::LEFT))
	{
		for (fast_mapsize_t x = 0; x < boarder_width; x++)
		{
			for (fast_mapsize_t y = 0; y < height; y++)
			{
				arr[x][y].weight = boarder_weight;
			}
		}
	}
}

/*
This is a specialization for the most common use case, an obstical fully in the field with a gradient being applied.
We will do the gradient computation once, and take advantage of the lines of symmetry {x = 0, y = 0, x=y, y=-x}
which should cut the number of gradient computations by 1/8
*/
void WeightMap::fast_add_obstical(mapsize_t x_in, mapsize_t y_in, mapsize_t radius, weight_t weight)
{
	const fweight_t radius_f = radius; // Don't want to convert it to a float each time
	for (size_t dy = radius; dy > 0; dy--)
	{
		for (size_t dx = 0; dx <= radius; dx++)
		{
			if (dx > dy)
			{
				// We have reached the x-y symmetry line
				// The values on the other side of that line are already being
				// filled by symmetry rules.
				// We can break early
				break;
			}

			const fweight_t dist = std::sqrt((dx * dx) + (dy * dy));
			if (dist > radius_f)
			{
				// We have reached the edge of this line, and all values after should remain unchanged
				// Cancel x-iteration early
				break;
			}

			const fweight_t frac = 1.0 - (dist / radius_f);
			const weight_t calculated_weight = frac * weight;

			arr[x_in + dx][y_in + dy].weight = std::max(calculated_weight, arr[x_in + dx][y_in + dy].weight); // Standard Location
			arr[x_in - dx][y_in + dy].weight = std::max(calculated_weight, arr[x_in - dx][y_in + dy].weight); // Symmetry accross x-axis
			arr[x_in + dx][y_in - dy].weight = std::max(calculated_weight, arr[x_in + dx][y_in - dy].weight); // Symmetry accross y-axis
			arr[x_in - dx][y_in - dy].weight = std::max(calculated_weight, arr[x_in - dx][y_in - dy].weight); // Symmetry accross x = -y

			// Symmetry Accross X=Y
			arr[x_in + dy][y_in + dx].weight = std::max(calculated_weight, arr[x_in + dy][y_in + dx].weight); // Standard Location
			arr[x_in - dy][y_in + dx].weight = std::max(calculated_weight, arr[x_in - dy][y_in + dx].weight); // Symmetry accross x-axis
			arr[x_in + dy][y_in - dx].weight = std::max(calculated_weight, arr[x_in + dy][y_in - dx].weight); // Symmetry accross y-axis
			arr[x_in - dy][y_in - dx].weight = std::max(calculated_weight, arr[x_in - dy][y_in - dx].weight); // Symmetry accross x = -y
		}
	}
	arr[x_in][y_in].weight = weight; // Standard Location
}

void WeightMap::slow_add_obstical(mapsize_t x_in, mapsize_t y_in, mapsize_t radius, weight_t weight, bool gradiant)
{

	const fast_mapsize_t left_x_value((mapsize_t)std::max<int32_t>(x_in - radius, 0));
	const fast_mapsize_t right_x_value((mapsize_t)std::min<int32_t>(x_in + radius, this->width));

	const fast_mapsize_t top_y_value((mapsize_t)std::max<int32_t>(y_in - radius, 0));
	const fast_mapsize_t bottom_y_value((mapsize_t)std::min<int32_t>(y_in + radius, this->height));

	for (fast_mapsize_t x = left_x_value; x < right_x_value; x++)
	{
		for (fast_mapsize_t y = top_y_value; y < bottom_y_value; y++)
		{
			const fweight_t dist = distance(x, y, x_in, y_in);
			if (!isValidPoint(x, y) || dist > radius)
			{
				continue;
			}

			Node &curNode = arr[x][y];

			if (gradiant)
			{
				const fweight_t frac = 1 - (dist / radius);
				const weight_t calculated_weight(frac * weight);
				curNode.weight = std::max(calculated_weight, curNode.weight);
			}
			else
			{
				curNode.weight = weight;
			}
		}
	}
}

void WeightMap::addObstical(mapsize_t x_in, mapsize_t y_in, mapsize_t radius, weight_t weight, bool gradiant)
{
	if (!isValidPoint(x_in, y_in))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Point (%u, %u) out of bounds!", x_in, y_in);
		throw std::invalid_argument(std::string(error_message));
	}

	if (!WeightMap::isValidWeight(weight))
	{
		char error_message[60];
		std::snprintf(error_message, sizeof(error_message), "Weight {%u} out of bounds!", weight);
		throw std::invalid_argument(std::string(error_message));
	}

	const int_fast32_t left_x_value(x_in - radius);
	const int_fast32_t right_x_value(x_in + radius);

	const int_fast32_t top_y_value(y_in - radius);
	const int_fast32_t bottom_y_value(y_in + radius);

	if ((left_x_value >= 0) && (right_x_value < this->width) && (top_y_value >= 0) && (bottom_y_value < this->height) && gradiant)
	{
		fast_add_obstical(x_in, y_in, radius, weight);
	}
	else
	{
		slow_add_obstical(x_in, y_in, radius, weight, gradiant);
	}
}

bool WeightMap::isValidPoint(int32_t x, int32_t y) const
{
	return (x < this->width && y < this->height) && (x >= 0 && y >= 0);
}

std::string WeightMap::path_to_str(path_t &path)
{
	std::stringstream ss;
	for (auto &point : path)
	{
		ss << '(' << point.first << ", " << point.second << ")\n";
	}
	return ss.str();
}

bool WeightMap::is_path_continuious(path_t &path)
{
	const fweight_t max_dist = std::sqrt(2);
	for (size_t i = 1; i < path.size(); i++)
	{
		point_t &pt1 = path[i - 1];
		point_t &pt2 = path[i];
		const fweight_t dist = WeightMap::distance(pt1.first, pt1.second, pt2.first, pt2.second);
		if (dist > max_dist)
		{
			return false;
		}
	}
	return true;
}

std::string WeightMap::point_to_string(const point_t &pt)
{
	std::stringstream ss;
	ss << '(' << pt.first << ", " << pt.second << ')';
	return ss.str();
}

// Helper Functions
void WeightMap::resetMap()
{
	for (fast_mapsize_t x = 0; x < width; x++)
	{
		for (fast_mapsize_t y = 0; y < height; y++)
		{
			// Set up default values
			Node &current_node = arr[x][y];
			current_node.parent_x = std::numeric_limits<mapsize_t>::max();
			current_node.parent_y = std::numeric_limits<mapsize_t>::max();
			current_node.color = Color::BLACK;
			current_node.costFromSrc = std::numeric_limits<decltype(current_node.costFromSrc)>::infinity();
		}
	}
}

void WeightMap::dijkstrasMakeMoves(const DijkstrasMove *moves, const size_t numMoves, const Node &currentNode, WeightMap::queue_t &q)
{
	for (size_t i = 0; i < numMoves; i++)
	{

		const int32_t xidx = currentNode.x + moves[i].dx;
		const int32_t yidx = currentNode.y + moves[i].dy;

		// Bounds Check
		if (isValidPoint(xidx, yidx))
		{
			Node &neighborNode = arr[(size_t)xidx][(size_t)yidx];

			// Basic Turning
			fweight_t nn_new_cost_from_src = (moves[i].weight_multiplier * (((fweight_t)(neighborNode.weight + currentNode.weight)) / 2)) + currentNode.costFromSrc;

			// Add cost to turning
			if (isValidPoint(currentNode.parent_x, currentNode.parent_y))
			{
				int32_t parrent_current_dx = currentNode.parent_x - currentNode.x;
				int32_t parrent_current_dy = currentNode.parent_y - currentNode.y;

				if (parrent_current_dx != moves[i].dx || parrent_current_dy != moves[i].dy)
				{
					nn_new_cost_from_src += 5;
				}
			}

			if ((nn_new_cost_from_src < neighborNode.costFromSrc))
			{

				neighborNode.color = Color::RED;
				neighborNode.costFromSrc = nn_new_cost_from_src;
				neighborNode.parent_x = currentNode.x;
				neighborNode.parent_y = currentNode.y;
				q.push(&neighborNode);
			}
		}
	}
}

#include "map_util.hpp"
fweight_t WeightMap::get_linear_cost(Node &a, Node &b)
{
	const int32_t dx = a.x - b.x;
	const int32_t dy = a.y - b.y;

	const mapsize_t top = std::min(a.y, b.y);
	const mapsize_t bottom = std::max(a.y, b.y);

	const mapsize_t left = std::min(a.x, b.x);
	const mapsize_t right = std::max(a.x, b.x);

	if (dx == 0)
	{
		fweight_t weight = (0.5 * a.weight) + (0.5 * b.weight);
		for (mapsize_t i = top; i < bottom; i++)
		{
			weight += arr[a.x][i].weight;
		}
		return weight;
	}
	else if (dy == 0)
	{
		fweight_t weight = (0.5 * a.weight) + (0.5 * b.weight);
		for (mapsize_t i = left; i < right; i++)
		{
			weight += arr[i][a.y].weight;
		}
		return weight;
	}
	else
	{
		// Set up the line to run through the center of a
		const float m = (float)dy / (float)dx;
		const float h = (a.y + 0.5f) - (m * (a.x + 0.5f));
		float weight = 0;

		// Reset colors
		for (size_t x = left; x <= right; x++)
		{
			for (size_t y = top; y <= bottom; y++)
			{
				arr[x][y].color = Color::BLACK;
			}
		}

		float linear_length = 0;

		std::queue<std::reference_wrapper<Node>> nodes;
		nodes.emplace(a);
		while (!nodes.empty())
		{
			Node &n = nodes.front();
			nodes.pop();

			// If node is in box
			if (n.x >= left && n.x <= right && n.y >= top && n.y <= bottom)
			{
				if (n.color != Color::BLACK)
				{
					continue;
				}

				const float sq_x = n.x + 0.5;
				const float sq_y = n.y + 0.5;
				const float line_length = get_length_line_in_square(sq_x, sq_y, m, h);
				linear_length += line_length;
				weight += n.weight * line_length;
				n.color = Color::GREEN;

				// Only push neighbors if the line goes through this box
				if (line_length > 0)
				{
					for (auto &move : WeightMapHelper::moves)
					{
						const mapsize_t x = n.x + move.dx;
						const mapsize_t y = n.y + move.dy;
						if (isValidPoint(x, y))
						{
							nodes.emplace(arr[x][y]);
						}
					}
				}
			}
		}

		linear_length -= (get_length_line_in_square(a.x + 0.5, a.y + 0.5, m, h) + get_length_line_in_square(b.x + 0.5, b.y + 0.5, m, h)) / 2;

		weight -= (get_length_line_in_square(a.x + 0.5, a.y + 0.5, m, h) * a.weight) / 2;
		weight -= (get_length_line_in_square(b.x + 0.5, b.y + 0.5, m, h) * b.weight) / 2;

		return weight;
	}
}

fweight_t WeightMap::distance(fweight_t x1, fweight_t y1, fweight_t x2, fweight_t y2)
{
	return (fweight_t)std::sqrt(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)));
}

/*
Returns the ratio of the path through these three points compared to the path between the end points
*/
fweight_t WeightMap::length_ratio(WeightMap::point_t &l, WeightMap::point_t &m, WeightMap::point_t &r)
{
	return distance(l.first, l.second, r.first, r.second) / (distance(l.first, l.second, m.first, m.second) + distance(m.first, m.second, r.first, r.second));
}

//---------------	Node Methods	----------------------------
bool operator==(const Node &a, const Node &b)
{
	return a.color == b.color && a.costFromSrc == b.costFromSrc && a.parent_x == b.parent_x && a.parent_y == b.parent_y && a.weight == b.weight && a.x == b.x && a.y == b.y;
}

std::ostream &operator<<(std::ostream &os, const Color &c)
{
	switch (c)
	{
	case Color::BLACK:
		os << "BLACK";
		break;
	case Color::RED:
		os << "RED";
		break;
	case Color::GREEN:
		os << "GREEN";
		break;
	default:
		os << "UNKNOWN";
		break;
	}

	return os;
}

std::ostream &operator<<(std::ostream &os, const Node &n)
{
	os << "{ X: " << n.x << "; Y: " << n.y << "; Weight: " << n.weight << "; Parent: (" << n.parent_x << ',' << n.parent_y << "); Color: " << n.color << "; CostFromSrc: " << n.costFromSrc << "}";
	return os;
}

std::ostream &operator<<(std::ostream &os, const WeightMap &wm)
{
	for (size_t y = 0; y < wm.height; y++)
	{
		for (size_t x = 0; x < wm.width; x++)
		{
			os << wm.arr[x][y] << ' ';
		}
		os << '\n';
	}

	return os;
}

bool NodeCmp::operator()(const Node &a, const Node &b)
{
	return a.costFromSrc > b.costFromSrc;
}

bool NodeCmp::operator()(const Node *a, const Node *b)
{
	return a->costFromSrc > b->costFromSrc;
}
