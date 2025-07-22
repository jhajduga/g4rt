//
// Created by g4dev on 20.11.19.
//

#ifndef Dose3D_IO_H
#define Dose3D_IO_H

#include <string>
#include <vector>
#include "TFile.h"
// #include "hdf5.h"

class IO {

  private:
  ///
  std::string m_path_to_binary_dir;

  ///
  std::string m_path_to_project_data_dir;

  ///
  std::string m_current_file;

  public:
  ///
  IO();

  IO(const std::string &file);

  ///
  ~IO() = default;

  ///
  std::string GetPathToTempRunDirectory();

  ///
  std::string GetPathProjectDataDirectory();

  ///
  std::vector<std::string> ReadAllLinesFromFile();

  ///
  void WriteToFile(const std::string &fileWithFullPath, const std::vector<std::string> &lines);

  ///
  // void ExtendAndWriteHdf5(const std::string &file_path, const std::vector<std::vector<double>> &data);

  ///
  void CleanUP();

  ///
  static void CreateDirIfNotExits(const std::string &path);

  ///
  static std::unique_ptr<TFile> CreateOutputTFile(const std::string& name, const std::string& dir=std::string());

  ///
  void DeleteFile(const std::string &file_path);
};

#endif //Dose3D_IO_H
