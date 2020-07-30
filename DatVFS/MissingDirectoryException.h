#pragma once
#include <stdexcept>
class MissingDirectoryException : public std::runtime_error {
public:
	const std::string path;

	MissingDirectoryException(std::string Path) : std::runtime_error("Failed to find directory: " + Path), path(Path) {
	}
};