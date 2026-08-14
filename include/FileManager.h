#pragma once

#include <fstream>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>

/**
 * @brief Utilities for basic file I/O operations.
 * @details Provides static-like inline functions to read, write, append, and analyze delimited text files. 
 * Designed to handle 2D string vectors seamlessly, enabling easy CSV or plain-text manipulation.
 */
namespace FileManager {

    /**
     * @brief Reads a delimited text file into a 2D vector of strings.
     * @details This function opens and parses a file line by line. Each line is split
     * into columns based on the specified delimiter. It correctly handles a space
     * delimiter by treating consecutive whitespace as a single separator and skips
     * any empty lines that might result from the parsing process. If the file cannot
     * be opened, an error is logged to std::cerr.
     *
     * @param filePath The full path to the file to be read.
     * @param delimiter The character that separates columns in the file.
     * @return An `std::optional` containing a `std::vector<std::vector<std::string>>`
     * with the file data upon success. Returns `std::nullopt` if the file cannot be opened.
     */
    inline std::optional<std::vector<std::vector<std::string>>> read(const std::string& filePath, char delimiter) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filePath << std::endl;
            return std::nullopt;
        }

        std::vector<std::vector<std::string>> fileData;
        std::string line;

        while (std::getline(file, line)) {
            std::vector<std::string> row;
            std::stringstream ss(line);
            std::string cell;

            if (delimiter == ' ') {
                while (ss >> cell) {
                    row.push_back(cell);
                }
            } else {
                while (getline(ss, cell, delimiter)) {
                    row.push_back(cell);
                }
            }
            
            if (!row.empty()) {
                fileData.push_back(row);
            }
        }
        return fileData;
    }

    /**
     * @brief Writes a 2D vector of strings to a file, overwriting any existing content.
     * @details This function ensures that the parent directory for the specified filePath
     * exists, creating it if necessary. It then opens the file in truncation mode,
     * effectively clearing it before writing the new data. Each inner vector is
     * written as a line, with elements separated by the given delimiter.
     *
     * @param filePath The full path to the output file.
     * @param data The 2D vector of strings to write to the file.
     * @param delimiter The character to use for separating columns.
     * @return Returns `true` if the file was successfully written, `false` otherwise.
     * An error is logged to std::cerr on failure.
     */
    inline bool write(const std::string& filePath, const std::vector<std::vector<std::string>>& data, char delimiter) {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not create or open file " << filePath << std::endl;
            return false;
        }

        for (size_t i = 0; i < data.size(); ++i) {
            for (size_t j = 0; j < data[i].size(); ++j) {
                file << data[i][j];
                if (j < data[i].size() - 1) {
                    file << delimiter;
                }
            }
            if (i < data.size() - 1) {
                file << "\n";
            }
        }
        return true;
    }

    /**
     * @brief Appends a 2D vector of strings to the end of a file.
     * @details The function first ensures the parent directory exists, creating it if
     * needed. It opens the file in append mode. If the file is not empty, it
     * prepends a newline character to ensure the new data starts on a fresh line,
     * preserving the integrity of the delimited format. Each inner vector is then
     * written as a new line.
     *
     * @param filePath The full path to the file to append data to.
     * @param data The 2D vector of strings to append.
     * @param delimiter The character to use for separating columns.
     * @return Returns `true` if the data was successfully appended, `false` otherwise.
     * An error is logged to std::cerr on failure.
     */
    inline bool append(const std::string& filePath, const std::vector<std::vector<std::string>>& data, char delimiter) {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream file(filePath, std::ios::app); 
        if (!file.is_open()) {
            std::cerr << "Error: Could not create or open file for appending " << filePath << std::endl;
            return false;
        }

         if (file.tellp() > 0) {
            file << "\n";
        }

        for (size_t i = 0; i < data.size(); ++i) {
            for (size_t j = 0; j < data[i].size(); ++j) {
                file << data[i][j];
                if (j < data[i].size() - 1) {
                    file << delimiter;
                }
            }
            if (i < data.size() - 1) {
                file << "\n";
            }
        }
        return true;
    }

    /**
     * @brief Checks if a file exists and counts its lines.
     * @details Verifies the existence of the file using std::filesystem. If the file
     * exists and can be opened, it iterates through the file counting the lines.
     * @param filePath The full path to the file.
     * @return The total number of lines in the file. Returns 0 if the file 
     * does not exist or cannot be opened.
     */
    inline int countLines(const std::string& filePath) {
        if (!std::filesystem::exists(filePath)) {
            return 0;
        }

        std::ifstream file(filePath);
        if (!file.is_open()) {
            return 0;
        }

        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            count++;
        }
        return count;
    }
}