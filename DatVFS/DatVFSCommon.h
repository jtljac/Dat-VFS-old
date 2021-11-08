#pragma once
#include <utility>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

/**
 * Splits a string by forward or back slashes, ignoring the last one
 * @param path The path to split
 * @return A vector containing the individual components of the path
 */
inline std::vector<std::string> stringPathToVectorPath(const std::string& path) {
    if (path.empty()) return {};

    bool ignoreLast = path[path.size() - 1] == '\\' || path[path.size() - 1] == '/';

    std::vector<std::string> pathList(std::count_if(path.begin(), path.end(), [&](const char& item){
        return item == '/' || item == '\\';
    }) + (ignoreLast ? 0 : 1));

    size_t start = 0;
    for (int i = 0; i < pathList.size(); ++i) {
        size_t nextPos = i == pathList.size() - 1 ? path.size() - (ignoreLast ? 1 : 0) : path.find_first_of("\\/", start);

        size_t length = nextPos - start;

        pathList[i] = path.substr(start, length);

        start = ++nextPos;
    }

    return pathList;
}

/**
 * An interface for classes that can be added to the VFS
 * Also contains logic for handling multiple entries in the VFS of the same IDVFSFile
 */
class IDVFSFile {
    uint8_t references = 0;
protected:
    size_t fileSize = 0;
public:
    virtual ~IDVFSFile()=default;

    [[nodiscard]] size_t getFileSize() const {
        return fileSize;
    }

    /**
     * Get a vector containing all the bytes of the DVFS File
     * @return A vector containing all the bytes of the DVFS File
     */
    [[maybe_unused]] [[nodiscard]] std::vector<char> getContent() const{
        std::vector<char> buffer(fileSize);
        if(getContent(buffer.data())) return buffer;
        else return {};
    }

    /**
     * Gets a count of the references to this DVFSFile in the VFS
     * @return The number of references to this DVFSFile in the VFS
     */
    [[maybe_unused]] [[nodiscard]] uint8_t getReferenceCount() const {
        return references;
    }

    /**
     * Increments the number of references
     * @return the new number of references
     */
    uint8_t& operator++() {
        return ++references;
    }

    /**
     * Decrements the number of references
     * @return the new number of references
     */
    uint8_t& operator--() {
        return --references;
    }

    // Virtual Functions
    /**
     * Check the file the DVFS File points to is valid
     * @return If the file the DVFS File points to is valid
     */
    [[nodiscard]] virtual bool isValidFile() const = 0;
    /**
     * Loads the content of the file into the provided buffer pointer
     * Warning, does not check the buffer is the correct size, you can get the size with <<DVFSFile>><getSize>
     * @param buffer A pointer to the buffer to deposit the file data into
     */
    virtual bool getContent(char* buffer) const = 0;
};

/**
 * An entry for the DVFS that represents loose files on the disk
 */
struct DVFSLooseFile : IDVFSFile {
    const std::filesystem::path path;
    explicit DVFSLooseFile(std::filesystem::path filePath) : path(std::move(filePath)) {
        if (!is_directory(path) && std::filesystem::exists(path)) {
            fileSize = (size_t) std::filesystem::file_size(path);
        }
    }

    [[nodiscard]] bool isValidFile() const override {
        return !is_directory(path) && std::filesystem::exists(path);
    }

    bool getContent(char* buffer) const override {
        if (!isValidFile()) return false;

        std::ifstream fileStream(path, std::ios::in | std::ios::binary);

        return fileStream.read(buffer, fileSize).good();
    }
};


/**
 * An interface containing methods for adding files to the DVFS
 */
struct IDVFSInserter {
    using pair = std::pair<std::string, std::unique_ptr<IDVFSFile>>;

    const std::vector<std::string> mountPoint;

    /**
     * @param mountPoint The location in DVFS To mount the files onto
     */
    explicit IDVFSInserter(const std::string& mountPoint = "") : mountPoint(stringPathToVectorPath(mountPoint)) {}

    /**
     * Gets a vector of all the files paired with their relative path in the DVFS
     * The pair is laid out as such:
     * first: string relative path
     * second: DVFSFile pointer
     * @return The files paired with their relative path in the DVFS
     */
    [[nodiscard]] virtual std::vector<pair> getAllFiles() const = 0;
};

/**
 * A Inserter for the DVS for adding loose files from the disk
 */
class DVFSLooseFilesInserter : public IDVFSInserter {
protected:
    const std::filesystem::path looseFilesPath;
    const bool recursive;

    explicit DVFSLooseFilesInserter(std::filesystem::path directory, const std::string& mountPoint = "", bool recursive = true) : looseFilesPath(std::move(directory)), IDVFSInserter(mountPoint), recursive(recursive) {}

    virtual void addFile(std::vector<pair>& pairList, const std::filesystem::path &path) const {
        std::string dest = relative(path, looseFilesPath).string();
        pairList.emplace_back(pair(dest, std::unique_ptr<IDVFSFile>(new DVFSLooseFile(path))));
    }

    void addFiles(std::vector<pair>& pairList, const std::filesystem::path& directory) const {
        std::filesystem::directory_iterator directories(directory);

        // Iterate through all files and folders in this directory
        for (const auto& entry : directories) {

            // If it's a directory, enter and add all the sub files
            if (entry.is_directory() && recursive) {
                addFiles(pairList, entry.path());
            }
            else {
                addFile(pairList, entry.path());
            }
        }
    }

    [[nodiscard]] std::vector<pair> getAllFiles() const override {
        std::vector<pair> pairList;

        addFiles(pairList, looseFilesPath);

        return pairList;
    }
};

/**
 * An inserter for the DVFS that adds files from the disk
 * Also allows a regex string to be passed to allow for name filtering
 */
class DVFSLooseFilesInserterFiltered : public DVFSLooseFilesInserter {
    std::string regexString;
public:
    explicit DVFSLooseFilesInserterFiltered(std::filesystem::path directory, std::string regexString, const std::string& mountPoint = "", bool recursive = true) : regexString(std::move(regexString)), DVFSLooseFilesInserter(std::move(directory), mountPoint, recursive) {}

    void addFile(std::vector<pair>& pairList, const std::filesystem::path& path) const override {
        if (std::regex_match(path.filename().string(), std::regex(regexString)))
            DVFSLooseFilesInserter::addFile(pairList, path);
    }
};