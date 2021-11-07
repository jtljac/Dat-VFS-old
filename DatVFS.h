#pragma once

#include <cstdint>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <cassert>
#include <regex>
#include "DatVFS/DatVFSCommon.h"

class DatVFS {
    std::unordered_map<std::string, DatVFS*> folders;
    std::unordered_map<std::string, IDVFSFile*> files;

public:
    /**
     * Counts all the files inside and below this directory in the VFS
     * @return The amount of files inside and below this directory in the VFS
     */
    size_t countFiles() {
        size_t count = files.size();
        for (auto& folder: folders) {
            count += folder.second->countFiles();
        }
        return count;
    }

    /**
     * Counts all the files inside and below this directory in the VFS that match the given regex
     * @param regex The regex string the title must match
     * @return The amount of files inside and below this directory in the VFS
     */
    inline int countFilesMatchingRegex(const std::string& regex) {
        return countFilesMatchingRegex(std::regex(regex));
    }

    /**
     * Counts all the files inside and below this directory in the VFS that match the given regex
     * @param regex The regex the title must match
     * @return The amount of files inside and below this directory in the VFS
     */
    int countFilesMatchingRegex(const std::regex& regex) {
        int count = 0;

        // Count files that match
        for (auto& file: files) {
            if (std::regex_match(file.first, regex)) {
                ++count;
            }
        }

        // add count of each subdirectory
        for (auto& folder: folders) {
            count += folder.second->countFilesMatchingRegex(regex);
        }

        return count;
    }

    /**
     * Retrieves the file at the given path
     * @param filePath The path to the file
     * @param index (Optional) The index of the path to start from
     * @return The file at the given location, null if no file is found
     */
    IDVFSFile* getFile(const std::vector<std::string>& filePath, size_t index = 0) {
        if (index == filePath.size() - 1) {
            return files[filePath[index]];
        } else if (index < filePath.size()) {
            return folders[filePath[index]]->getFile(filePath, ++index);
        } else {
            return nullptr;
        }
    }

    /**
     * Retrieves the file at the given path
     * @param filePath The path to the file
     * @return The file at the given location, null if no file is found
     */
    IDVFSFile* getFile(const std::string& filePath) {
        std::vector<std::string> filePathList = stringPathToVectorPath(filePath);

        return getFile(filePathList, 0);
    }

    /**
     * Retrieves the folder at the given path
     * @param folderPath The path of the folder
     * @param index (Optional) The index of the path to start from
     * @return The folder at the given location
     */
    DatVFS* getFolder(const std::vector<std::string>& folderPath, size_t index = 0) {
        if (index == folderPath.size() - 1) {
            return folders[folderPath[index]];
        } else if (index < folderPath.size()) {
            return folders[folderPath[index]]->getFolder(folderPath, ++index);
        } else {
            return nullptr;
        }
    }

    /**
     * Retrieves the folder at the given path
     * @param folderPath The path of the folder
     * @param index (Optional) The index of the path to start from
     * @return The folder at the given location
     */
    DatVFS* getFolder(const std::string& folderPath) {
        std::vector<std::string> folderPathList = stringPathToVectorPath(folderPath);

        return getFolder(folderPathList, 0);
    }

    /**
     * Inserts the IDVFSFile into VFS
     * @param filePath The path to the file
     * @param dvfsFile The file to insert
     * @param pathIndex (Optional) The index of the path to start from
     * @return If the file was successfully inserted
     */
    bool insertFile(const std::vector<std::string>& filePath, IDVFSFile* dvfsFile, size_t pathIndex = 0) {
        if (pathIndex == filePath.size() - 1) {
            if (files.count(filePath[pathIndex]) > 0) {
                delete files[filePath[pathIndex]];
            }
            files[filePath[pathIndex]] = dvfsFile;
            return true;
        } else if (pathIndex < filePath.size()) {
            return folders[filePath[pathIndex]]->getFile(filePath, ++pathIndex);
        } else {
            return false;
        }
    }

    /**
     * Inserts the IDVFSFile into VFS
     * @param filePath The path to the file
     * @param dvfsFile The file to insert
     * @return If the file was successfully inserted
     */
    bool insertFile(const std::string& filePath, IDVFSFile* dvfsFile) {
        std::vector<std::string> filePathList = stringPathToVectorPath(filePath);

        return insertFile(filePathList, dvfsFile, 0);
    }

    /**
     * Inserts the files defined by the inserter into the VFS
     * @param inserter The inserter defining the files to insert
     * @param mountIndex (Optional) The index of the mount path to start from
     * @return If the file was successfully inserted
     */
    bool insertFiles(const IDVFSInserter& inserter, size_t mountIndex = 0) {
        if (mountIndex < inserter.mountPoint.size() - 1) {
            DatVFS* folder = folders[inserter.mountPoint[mountIndex]];

            if (folder) return folder->insertFiles(inserter, ++mountIndex);
            else return false;
        }

        for (const auto& item : inserter.getAllFiles()) {
            insertFile(item.first, item.second.get());
        }
    }

//    /**
//     * Gets a vector of all the files that match the given regex string inside this directory
//     * @param Regex The regex string to match the file title to
//     * @return A vector containing all the files that match the given regex string
//     */
//    std::vector<IDVFSFile*> getAllFilesThatMatchRegex(const std::string& Regex) {
//        // TODO: maybe use a linked list and only iterate through the vfs once
//        int count = countFilesMatchingRegex(Regex);
//        std::vector<IDVFSFile*> Files(count);
//
//        getAllFilesThatMatchRegexInternal(Regex, Files);
//        return Files;
//    }

    /**
     * Removes all empty directories below this directory in the VFS
     */
    void prune() {
        for (auto it = folders.begin(); it != folders.end();) {
            it->second->prune();
            if (it->second->countFiles() == 0) {
                delete it->second;
                folders.erase(it++);
            } else {
                ++it;
            }
        }
    }

    /**
     * Prints to cout all of the directories in the tree and all of the files in those directories in a formatted way
     * @param Prefix The prefix of the file/folder when printing it (To add formatting)
     * @param Depth The depth of the file/folder (To add formatting)
     */
    void tree(const std::string& Prefix = "", int Depth = 0) {
        // Print folders and their subdirectories & files
        for (auto& folder: folders) {
            std::cout << Prefix << (Depth != 0 ? "-" : "") << folder.first << std::endl;
            folder.second->tree(Prefix + " |", Depth + 1);
        }

        // Print Files
        for (auto& file: files) {
            std::cout << Prefix << (Depth != 0 ? "-" : "") << file.first << std::endl;
        }
    }
};