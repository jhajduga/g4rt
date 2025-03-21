/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 01.06.2018
*
*/

#ifndef Dose3D_SERVICES_H
#define Dose3D_SERVICES_H

#include "RunSvc.hh"
#include "GeoSvc.hh"
// #include "LogSvc.hpp"
#include "ConfigSvc.hh"
#include "MaterialsSvc.hh"
#include "DicomSvc.hh"
#include <sstream>
#include <string>
#include <filesystem>

///\brief Templated method to get a pointer to the different type of services.
template<typename T>
T *Service() {
  return T::GetInstance();
}

///\namespace svc The framework-specific namespace defining different utility functions.
namespace svc {
  /////////////////////////////////////////////////////////////////////////////////////////
  // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
  std::string currentDateTime();

  /////////////////////////////////////////////////////////////////////////////////////////
  // Get current date format is YYYY-MM-DD
  std::string currentDate();


  static std::string m_output_location;

  ///
  size_t countDirsInLocation(const std::string& path, const std::string& substr=std::string());

  ///
  std::string createDirIfNotExits(const std::string& path); 

  ////
  std::string createCurrentDateDirIfNotExits(const std::string& path);

  ///
  std::string getOutputDir(); // const std::string& path

  ///
  std::string createOutputDir(const std::string& userPath=std::string());

  ////
  bool checkIfFileExist(const std::string& file_full_or_relative_path);

  ///
  void deleteFileIfExists(const std::string& file_full_path);

  ///
  std::vector<std::string> getFilesInDir(const std::string& path, const std::string& extension=std::string());

  ///
  std::string getFileExtenstion(const std::string& filePath);

  //
  std::string getFileName(const std::string& filePath);

  ///\brief The number to std::string conversion function.
  ///\param v number to be converted.
  ///\param precision precision to be taken in the conversion.
  template<typename T>
  std::string to_string(const T &v) {
    std::ostringstream stm;
    stm << std::fixed << std::setprecision(2) << std::boolalpha;
    stm << v;
    return stm ? stm.str() : "*** error ***";
  }

  ///\brief Function to check if the given key exists in a given std::map.
  template<typename K, typename V>
  bool checkItem(std::map<K, V> &items, const K key) {
    if (items.find(key) != items.end())
      return true;
    else
      return false;
  }

  ///\brief Parsing a std::string containing a number in scientific notation.
  double scistr_to_double(const std::string &m_str);

  ///\brief General tool for reading from cvs file in a column wise manner.
  ///\param fname Name of the file to read from.
  ///\param data The std::map reference to input the read-in values. The map keys are for the storing of the next columns.
  bool readCsv(const std::string &fname, std::map<std::string, std::vector<double> *> &data);

  ///
  void invalidArgumentError(const std::string& caller, const std::string& message);

  ///
  double round_with_prec(double val, int prec);
  G4ThreeVector round_with_prec(const G4ThreeVector& val, int prec);

  ///
  std::string tolower(const std::string& source);

  ///
  std::vector<double> linearizeG4ThreeVector(const G4ThreeVector& vector);
  std::vector<double> linearizeG4ThreeVector(const std::vector<G4ThreeVector>& vector);

  G4ThreeVector getHalfSize(G4VPhysicalVolume* volume);

  enum class Transform {
    LocalToGlobal,
    GlobalToLocal
  };

  ///
  std::size_t getHashedStrFromIndexes(const std::vector<int>& indexes);

  /// @brief 
  /// @param localPosition 
  /// @param volumeOfLocalFrame 
  /// @param direction 
  /// @return 
  G4ThreeVector transformPosition(const G4ThreeVector& localPosition, IPhysicalVolume* volumeOfLocalFrame, Transform direction = Transform::LocalToGlobal);
}
#endif  // Dose3D_SERVICES_H
