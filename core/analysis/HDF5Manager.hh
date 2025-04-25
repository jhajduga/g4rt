// HDF5Manager.hh
#ifndef HDF5_MANAGER_HH
#define HDF5_MANAGER_HH

#include <highfive/H5File.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class HDF5Manager {
public:
    /// Singleton instance accessor
    static HDF5Manager& Instance();

    /// Opens or creates an HDF5 file
    void OpenFile(const std::string& fileName);

    /// Closes the currently open HDF5 file
    void CloseFile();

    /// Creates a group inside the HDF5 file
    void CreateGroup(const std::string& groupName);

    /// Writes a vector of doubles into a dataset
    void WriteDataset(const std::string& path, const std::vector<double>& data);

    /// Writes a vector of integers into a dataset
    void WriteDataset(const std::string& path, const std::vector<int>& data);

    /// Checks if a given group exists
    bool GroupExists(const std::string& groupName);

    /// Expose underlying file object for advanced operations
    HighFive::File& File();

    /// Expose mutex to synchronize writes
    std::mutex& Mutex();

private:
    HDF5Manager();
    ~HDF5Manager();

    /// Delete copy and move semantics
    HDF5Manager(const HDF5Manager&) = delete;
    HDF5Manager& operator=(const HDF5Manager&) = delete;

    std::unique_ptr<HighFive::File> m_file;
    std::mutex m_mutex; ///< Mutex for thread-safe file operations
};

#endif // HDF5_MANAGER_HH
