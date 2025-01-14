//
// Created by g4dev on 20.11.19.
//

#include "IO.hh"
#include <fstream>
#include <iostream>
#include <utility>
#include <filesystem>

namespace fs = std::filesystem;

////////////////////////////////////////////////////////////////////////////////
///
IO::IO() {
  m_path_to_binary_dir = PROJECT_BINARY_PATH;
  m_path_to_project_data_dir = PROJECT_DATA_PATH;
}

////////////////////////////////////////////////////////////////////////////////
///
IO::IO(const std::string &file) {
  m_current_file = file;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string IO::GetPathToTempRunDirectory() {
  auto temp_dir = m_path_to_binary_dir + "/temporary_run_dir";
  CreateDirIfNotExits(temp_dir);
  return temp_dir;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string IO::GetPathProjectDataDirectory() {
  return m_path_to_project_data_dir;
}

////////////////////////////////////////////////////////////////////////////////
///
void IO::CleanUP() {
  // todo in future...
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::string> IO::ReadAllLinesFromFile() {
  std::vector<std::string> lines;
  if (!m_current_file.empty()) {
    std::ifstream infile(m_current_file);
    std::string line;
    while (std::getline(infile, line)) lines.push_back(line);
  }
  return lines;
}

////////////////////////////////////////////////////////////////////////////////
///
void IO::WriteToFile(const std::string &fileWithFullPath, const std::vector<std::string> &lines) {
  DeleteFile(fileWithFullPath);

  // open a file in write mode.
  std::ofstream outfile;
  outfile.open(fileWithFullPath);

  // again write inputted data into the file.
  for (const auto &line : lines)
    outfile << line << std::endl;

  // close the opened file.
  outfile.close();
}


void IO::ExtendAndWriteHdf5(const std::string &file_path, const std::vector<std::vector<double>> &data) {
    // Open the file
    hid_t file_id = H5Fopen(file_path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (file_id < 0) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }

    // Open the dataset
    hid_t dataset_id = H5Dopen(file_id, "/dataset", H5P_DEFAULT);
    if (dataset_id < 0) {
        std::cerr << "Failed to open dataset" << std::endl;
        H5Fclose(file_id);
        return;
    }

    // Get the current dimensions of the dataset
    hid_t dataspace_id = H5Dget_space(dataset_id);
    hsize_t current_dims[2];
    H5Sget_simple_extent_dims(dataspace_id, current_dims, NULL);

    // Calculate the new dimensions
    hsize_t new_dims[2] = {current_dims[0] + data.size(), current_dims[1]};
    H5Dset_extent(dataset_id, new_dims);

    // Select the hyperslab in the extended portion of the dataset
    hsize_t start[2] = {current_dims[0], 0};
    hsize_t count[2] = {data.size(), data[0].size()};
    hid_t memspace_id = H5Screate_simple(2, count, NULL);
    dataspace_id = H5Dget_space(dataset_id);
    H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL);

    // Flatten the 2D vector into a 1D vector
    std::vector<double> flat_data;
    for (const auto &row : data) {
        flat_data.insert(flat_data.end(), row.begin(), row.end());
    }

    // Write the new data to the extended portion of the dataset
    H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, flat_data.data());

    // Close the dataset, dataspace, and file
    H5Sclose(memspace_id);
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    H5Fclose(file_id);
}

/////////////////////////////////////////////////////////////////////////////////////////
void IO::CreateDirIfNotExits(const std::string &path) {
  fs::path dp(path);
  if (!fs::exists(dp)) {
    std::cout << "[INFO]:: Created directory: " << path << std::endl;
    fs::create_directories(dp);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
void IO::DeleteFile(const std::string &file_path) {
  fs::path fp = file_path;
  if (fs::exists(fp)) {
    std::cout << "[INFO]:: Remove existing file:\n"
                 "[INFO]:: " << file_path << std::endl;
    fs::remove(fp);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
std::unique_ptr<TFile> IO::CreateOutputTFile(const std::string& name, const std::string& dir){
  auto output_tfile = std::make_unique<TFile>(name.c_str(),"RECREATE");
  if(!dir.empty()){
    std::string tmp; 
    std::stringstream ss(dir);
    std::vector<std::string> dirs;
    while(getline(ss, tmp, '/')){
      dirs.push_back(tmp);
    }
    auto current_dir = output_tfile->mkdir(dirs.at(0).c_str());
    std::cout << "Created dir: " << dirs.at(0) << std::endl;
    for(const auto& d : dirs){
      if(d!=dirs.at(0)){
        current_dir = current_dir->mkdir(d.c_str());
        std::cout << "Created dir: " << d << std::endl;
      }
    }
  }
  return std::move(output_tfile);
}

