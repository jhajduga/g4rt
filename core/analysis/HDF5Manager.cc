// HDF5Manager.cc
#include "HDF5Manager.hh"
#include <iostream>

HDF5Manager& HDF5Manager::Instance() {
    static HDF5Manager instance;
    return instance;
}

HDF5Manager::HDF5Manager() {}

HDF5Manager::~HDF5Manager() {
    CloseFile();
}

void HDF5Manager::OpenFile(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file && m_file->isValid()) {
        std::cerr << "HDF5 file already open!" << std::endl;
        return;
    }

    m_file = std::make_unique<HighFive::File>(fileName, HighFive::File::Overwrite);
}

void HDF5Manager::CloseFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file) {
        m_file.reset();
    }
}

void HDF5Manager::CreateGroup(const std::string& groupName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file) {
        throw std::runtime_error("HDF5 file is not open.");
    }

    if (!m_file->exist(groupName)) {
        m_file->createGroup(groupName);
    }
}

void HDF5Manager::WriteDataset(const std::string& path, const std::vector<double>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file) {
        throw std::runtime_error("HDF5 file is not open.");
    }

    m_file->createDataSet<double>(path, HighFive::DataSpace::From(data)).write(data);
}

void HDF5Manager::WriteDataset(const std::string& path, const std::vector<int>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file) {
        throw std::runtime_error("HDF5 file is not open.");
    }

    m_file->createDataSet<int>(path, HighFive::DataSpace::From(data)).write(data);
}

bool HDF5Manager::GroupExists(const std::string& groupName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file) {
        return false;
    }
    return m_file->exist(groupName);
}

HighFive::File& HDF5Manager::File() {
  if (!m_file || !m_file->isValid()) {
      throw std::runtime_error("HDF5 file is not open");
  }
  return *m_file;
}

std::mutex& HDF5Manager::Mutex() {
  return m_mutex;
}
