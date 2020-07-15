#pragma once
#include <stdexcept>
class MissingFileException : public std::runtime_error {
public:
	const std::string path;

	MissingFileException(std::string Path) : std::runtime_error("Failed to find file at path: " + Path) {
		path = Path;
	}
};