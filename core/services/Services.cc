//
// Created by brachwal on 2018-09-23.
//
#include "Services.hh"
#include <algorithm>
#include "colors.hh"
#include "G4Box.hh"
#include "IPhysicalVolume.hh"

#include <regex>

namespace fs = std::filesystem;


////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the current date and time formatted as "YYYY-MM-DD.HH:MM:SS".
 *
 * @return std::string Current date and time in the format "YYYY-MM-DD.HH:MM:SS".
 */
std::string svc::currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	return buf;
}

////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the current date in "YYYY-MM-DD" format.
 *
 * @return std::string The current date as a string formatted "YYYY-MM-DD".
 */
std::string svc::currentDate() {
    auto dateTime = currentDateTime();
	return dateTime.substr(0,dateTime.find_first_of('.'));
}



////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Append the current date (YYYY-MM-DD) to the given base path and ensure the resulting directory exists.
 *
 * @param path Base directory path to which the date subdirectory will be appended.
 * @return std::string Full path including the appended date subdirectory.
 */
std::string svc::createCurrentDateDirIfNotExits(const std::string& path){
  std::string dateDirPath = path;
	auto date = svc::currentDate();
	dateDirPath+=path.at(path.length()-1)=='/' ? date : "/"+date;
	fs::path dp (dateDirPath);
	if(!fs::exists (dp)){
		std::cout << "[INFO]:: Created directory: "<< dateDirPath <<std::endl;
		fs::create_directories(dp);
	}
	return dateDirPath;

}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Obtain the configured output directory, ensuring it exists.
 *
 * Reads the RunSvc:OutputDir value from the configuration service. If the configured value is empty the function logs a fatal error and terminates the process. If the configured directory does not exist, it is created (a dated subdirectory may be created by the helper routines).
 *
 * @return std::string Absolute path to the validated (and created, if necessary) output directory.
 */
std::string svc::getOutputDir(){ // const std::string& path
  auto configSvc = Service<ConfigSvc>();
  auto output_dir = configSvc->GetValue<std::string>("RunSvc", "OutputDir");
    if(output_dir.empty()){
      LOGSVC_FATAL("Service","OutputDir must be not empty!");
      std::exit(EXIT_FAILURE);
      
      // configSvc->SetValue("RunSvc", "OutputDir", output_dir);
      //   output_dir = svc::createOutputDir();
    }
    else {
      if(!svc::checkIfFileExist(output_dir)){
        output_dir = svc::createOutputDir(output_dir);
      }
    }
  return output_dir;
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Creates a directory at the specified path if it does not already exist.
 *
 * If the directory does not exist, it is created (including any necessary parent directories). Returns the original path.
 *
 * @param path The directory path to create if missing.
 * @return std::string The input path.
 */
std::string svc::createDirIfNotExits(const std::string& path) {
  fs::path dp (path);
  if(!fs::exists (dp)){
		std::cout << "[INFO]:: Created directory: "<< path <<std::endl;
		fs::create_directories(dp);
	}
	return path;
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Creates an output directory with a subdirectory for the current date.
 *
 * If a user-specified path is provided, it is used as the base output directory; otherwise, a default project output path is used. A subdirectory named with the current date is created within the base directory if it does not already exist. Returns the full path to the dated output directory.
 *
 * @param userArgPath Optional user-specified base directory for output. If empty, the default project output directory is used.
 * @return std::string Full path to the created dated output directory.
 */
std::string svc::createOutputDir(const std::string& userArgPath) {
    // define the output dir path
    std::string output_dir_path;
    std::string project_path = PROJECT_LOCATION_PATH;
    project_path+="/output";
    if(!userArgPath.empty())
        output_dir_path=userArgPath;
    else
        output_dir_path = project_path;

    // create current day-date dir if not exists
    auto date_output_dir = svc::createCurrentDateDirIfNotExits(output_dir_path);
    // std::cout << "[INFO]:: Output location is defined as:\n"
    //                 "[INFO]:: "<< date_output_dir << std::endl;
    return date_output_dir;
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Checks if a file exists at the given path or relative to the project data directory.
 *
 * @param file_full_or_relative_path The file path to check, either absolute or relative.
 * @return true if the file exists at the specified path or within the project data directory; false otherwise.
 */
bool svc::checkIfFileExist(const std::string& file_full_or_relative_path){
  if(file_full_or_relative_path.empty()) 
    return false;
  // try to get file from full path
	if(fs::exists(fs::path(file_full_or_relative_path)))
    return true;
  // try to get file from relative path
  std::string data_path = PROJECT_DATA_PATH;
  auto file = data_path+"/"+file_full_or_relative_path;
  if(fs::exists(fs::path(file)))
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Remove the file at the given path if it exists.
 *
 * If a file exists at the provided path, the file is removed; if no file exists, the call has no effect.
 *
 * @param file_full_path Path to the file to remove (absolute or relative).
 */
void svc::deleteFileIfExists(const std::string& file_full_path){
	fs::path fp = file_full_path;
	if(fs::exists(fp)){
		std::cout << "[INFO]:: Remove file:\n"
			   		 "[INFO]:: " << file_full_path <<std::endl;
		fs::remove(fp);
	}
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Determines the next available numeric index for directories matching a pattern.
 *
 * Scans the specified path for subdirectories whose names match the given substring, optionally followed by an underscore and a number (e.g., "substr", "substr_1", "substr_2", ...). Returns the next available index number, which is one greater than the highest found. Returns 1 if only the base pattern exists, or 0 if no matching directories are found.
 *
 * @param path Directory to search for matching subdirectories.
 * @param substr Substring pattern to match at the start of directory names.
 * @return size_t The next available numeric index for the pattern.
 */
size_t svc::countDirsInLocation(const std::string& path,const std::string& substr){
  auto dirIter = std::filesystem::directory_iterator(path);
  std::string patternNum = substr + std::string("_([0-9]+)");
  std::string pattern0 = substr + std::string("$");
  //   LOGSVC_TRACE("pattern: {}", patternNum);
  std::regex regexPatternNum(patternNum);
  std::regex regexPattern0(pattern0);
  int maxNumber = -1; // Initialize with a minimum value
  for (auto& entry : dirIter){
      if (entry.is_directory()){
        std::smatch match;
        std::string path = entry.path().string(); //nedded for regexp search
        //   LOGSVC_TRACE("Path: {}", path);
        int number = -1;
        if (std::regex_search(path, match, regexPattern0)) {
          //   LOGSVC_TRACE("Match first.");
          number = 0;
        }
        else if (std::regex_search(path, match, regexPatternNum)) {
            std::string numberStr = match[1].str();
            //   LOGSVC_TRACE("Matchstr: {}", numberStr);
            try {
                number = std::stoi(numberStr);
            } catch (const std::exception& e) {
                //   LOGSVC_TRACE("Conversion to int error. Skiping.")
            }
        }
        // Update the maximum number if necessary
        if (number > maxNumber) {
            maxNumber = number;
        }
        //   LOGSVC_TRACE("Maxnum: {}", maxNumber);
      }
  }
  return maxNumber + 1;
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief List files in a directory, optionally filtering by an extension substring.
 *
 * @param path Directory to search.
 * @param extension Optional substring to filter file paths (e.g., ".txt"); when empty, all files are returned.
 * @return std::vector<std::string> Vector of matching file paths.
 */
std::vector<std::string> svc::getFilesInDir(const std::string& path, const std::string& extension){
  std::vector<std::string> files;
  if(path.empty())
    return files;
  for (const auto & file: std::filesystem::directory_iterator(path)) {
    auto file_path = static_cast<std::string>(file.path());
    if(extension.empty()){
      files.push_back(file_path);
    }
    else {
      if(file_path.find(extension) != std::string::npos)
        files.push_back(file_path);
    }
  }
  return files;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Return the file extension from a filesystem path.
 *
 * If the path has no extension or the filename begins with a dot (e.g., hidden Unix files like ".gitignore"),
 * an empty string is returned.
 *
 * @param filePath Path or filename to inspect.
 * @return The file extension without the leading dot, or an empty string if none exists.
 */
std::string svc::getFileExtenstion(const std::string& filePath){
  // Find the last dot position
  std::size_t dotPos = filePath.rfind('.');
  // If there's no dot, or it's the first character (hidden files in Unix), return an empty string
  if (dotPos == std::string::npos || dotPos == 0)
      return "";
  // Extract and return the substring after the last dot
  return filePath.substr(dotPos + 1);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Extracts the file name (base name) without its extension from a file path.
 *
 * @param filePath Full or relative path to the file.
 * @return std::string The file name with any trailing extension removed.
 *
 * @note If the file name begins with a leading dot (e.g., ".bashrc"), the leading dot is treated as the extension delimiter and this function will return an empty string.
 */
std::string svc::getFileName(const std::string& filePath){
  // Find last occurrence of '/' or '\\' to handle different path separators
  size_t lastSlash = filePath.find_last_of("/\\");
  std::string fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

  // Find last occurrence of '.' to handle file extension
  size_t lastDot = fileName.find_last_of('.');
  return (lastDot != std::string::npos) ? fileName.substr(0, lastDot) : fileName;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Converts a string in scientific notation to a double.
 *
 * Parses the input string as a floating-point number, supporting scientific notation (e.g., "1.23e-4").
 * @param m_str The string representing a number in scientific notation.
 * @return The parsed double value.
 * @throws std::string if the input cannot be converted to a number.
 */
double svc::scistr_to_double(const std::string &m_str) {

  std::stringstream ss(m_str);
  double converted = 0;
  ss >> converted;
  if (ss.fail()) {
    std::string str = "Unable to format ";
    str += m_str;
    str += " as a number!";
    throw (str);
  }
  return converted;
}

////////////////////////////////////////////////////////////////////////////////
/// Function template specialization for G4String type
template<>
/**
 * @brief Converts a G4String to a standard std::string.
 *
 * @param v The G4String to convert.
 * @return std::string The equivalent standard string.
 */
std::string svc::to_string<G4String>(const G4String &v) {
  return std::string(v.data());
}

////////////////////////////////////////////////////////////////////////////////
/// Function template specialization for G4bool type
template<>
/**
 * @brief Converts a G4bool value to its string representation.
 *
 * @return "TRUE" if the value is true, "FALSE" otherwise.
 */
std::string svc::to_string<G4bool>(const G4bool &v) {
  return v ? "TRUE" : "FALSE";
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reads a CSV file into a map of column names to vectors of doubles.
 *
 * Each column is named "Col0", "Col1", etc., based on its position. The function skips lines that are empty or start with '#', and supports values in scientific notation. If the data map is not empty, it clears and deletes its contents before reading. Returns true if the file is successfully read, false if the file cannot be opened.
 *
 * @param fname Path to the CSV file.
 * @param data Map to be filled with column data; previous contents are deleted.
 * @return true if the file was read successfully, false otherwise.
 */
bool svc::readCsv(const std::string &fname, std::map<std::string, std::vector<double> *> &data) {

  G4cout << "[INFO]:: Reading the csv file :: " << fname << G4endl;

  if (data.size() > 0) {
    G4cout << "[WARNING]:: Svc :: readCsv :: clearing the data container " << G4endl;
    for (auto &i: data) delete i.second;
    data.clear();
  }

  bool isInitialized(false);

  std::string line;
  std::ifstream file(fname.c_str());
  if (file.is_open()) {  // check if file exists
    while (getline(file, line)) {
      // get rid of commented out or empty lines:
      if (line.length() > 0 && line.at(0) != '#') {
        std::size_t lcomm = line.find('#');
        // comments at the end of the line (?)
        unsigned end = lcomm != std::string::npos ? lcomm - 1 : line.length();
        line = line.substr(0, end - 1);

        if (!isInitialized) {
          size_t n = std::count(line.begin(), line.end(), ',');
          for (size_t i = 0; i < n + 1; ++i) {
            data["Col" + std::to_string(i)] = new std::vector<double>{};
            isInitialized = true;
          }
        }

        std::istringstream ssLine(line);
        std::string val_str;
        int n(0);
        while (std::getline(ssLine, val_str, ',')) {
          std::size_t vSci = val_str.find('e'); // Number in Scientific Notation
          if (vSci != std::string::npos)
            data.at("Col" + std::to_string(n++))->push_back(svc::scistr_to_double(val_str));
          else
            data.at("Col" + std::to_string(n++))->push_back(std::stod(val_str));
        }
      }
    }
    return true;
  } else {
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Report an invalid argument error for the given caller and message.
 *
 * Prints an error message including the caller and message to standard output, then throws `std::invalid_argument`.
 *
 * @param caller Identifier of the function or component reporting the error.
 * @param message Human-readable description of the invalid argument.
 * @throws std::invalid_argument Always thrown when this function is invoked.
 */
void svc::invalidArgumentError(const std::string& caller, const std::string& message){
    std::cout << FRED("[ERROR]")<<"::"<< caller << ":: " << message << std::endl;
    throw std::invalid_argument("invalid argument");
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Rounds a floating-point value to the specified number of decimal places.
 *
 * @param val Value to round.
 * @param prec Number of decimal places to round to.
 * @return double Rounded value with the specified number of decimal places.
 */
double svc::round_with_prec(double val, int prec){
  return  std::round(val * std::pow(10,prec)) / std::pow(10,prec);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Rounds each component of a G4ThreeVector to the specified decimal precision.
 *
 * @param val The input G4ThreeVector to round.
 * @param prec The number of decimal places to round to.
 * @return G4ThreeVector A new vector with each component rounded to the given precision.
 */
G4ThreeVector svc::round_with_prec(const G4ThreeVector& val, int prec){
  return  G4ThreeVector(
    svc::round_with_prec(val.getX(),prec),
    svc::round_with_prec(val.getY(),prec),
    svc::round_with_prec(val.getZ(),prec)
  );
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Produce a lowercase copy of the input string.
 *
 * Converts every character in the provided string to its lowercase equivalent
 * using the C locale rules.
 *
 * @param source Input string to convert.
 * @return std::string Lowercase copy of `source`.
 */
std::string svc::tolower(const std::string& source){
  auto data = source;
  std::transform(data.begin(), data.end(), data.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return data;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Linearizes a G4ThreeVector into a std::vector of its components.
 *
 * @param vector Source vector.
 * @return std::vector<double> A 3-element vector containing [x, y, z] in that order.
 */
std::vector<double> svc::linearizeG4ThreeVector(const G4ThreeVector& vector){
  return std::vector<double>{vector.getX(),vector.getY(),vector.getZ()};
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Flattens a vector of G4ThreeVector objects into a single vector of doubles.
 *
 * Each G4ThreeVector is expanded into its three components (x, y, z), resulting in a contiguous sequence of doubles.
 *
 * @param vector Vector of G4ThreeVector objects to be linearized.
 * @return std::vector<double> Flattened vector containing all components in order.
 */
std::vector<double> svc::linearizeG4ThreeVector(const std::vector<G4ThreeVector>& vector){
  std::vector<double> linearized_vector;
  for(const auto& iv : vector){ 
    for(const auto& i :  svc::linearizeG4ThreeVector(iv))
      linearized_vector.emplace_back(i);
  }
  return std::move(linearized_vector);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the half-lengths of the X, Y, and Z dimensions of a G4Box solid associated with the given physical volume.
 *
 * Assumes the provided physical volume's solid is a G4Box and extracts its half-size dimensions.
 *
 * @param volume Pointer to the Geant4 physical volume whose box half-sizes are to be retrieved.
 * @return G4ThreeVector The half-lengths along the X, Y, and Z axes.
 */
G4ThreeVector svc::getHalfSize(G4VPhysicalVolume* volume){
  auto solid = dynamic_cast<G4Box*>(volume->GetLogicalVolume()->GetSolid());
  return G4ThreeVector(solid->GetXHalfLength(),solid->GetYHalfLength(),solid->GetZHalfLength());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Convert a position between a volume's local coordinate frame and the global frame by applying parent-frame rotations.
 *
 * Traverses the parent-volume chain starting from the provided local frame and applies each parent frame's rotation to produce the position in the target frame. Translation components are intentionally not applied.
 *
 * @param localPosition Position expressed in the starting local frame.
 * @param volumeOfLocalFrame The volume whose local frame is the reference for the input position.
 * @param direction If `Transform::LocalToGlobal`, rotates the input toward the global frame; if `Transform::GlobalToLocal`, rotates toward the local frame.
 * @return G4ThreeVector The input position rotated into the requested target coordinate frame.
 */
G4ThreeVector svc::transformPosition(const G4ThreeVector& localPosition, IPhysicalVolume* volumeOfLocalFrame, Transform direction){
  G4cout << "transformPosition: " << localPosition << "..." << G4endl;
  G4ThreeVector globalPosition = localPosition; // Start with local position
  // Traverse up the hierarchy
  auto currentVolume = volumeOfLocalFrame->GetParentPtr();
  while (currentVolume) {
    // G4cout << " current volume of local frame: " << currentVolume->GetName() << G4endl;
    auto pv = currentVolume->GetPhysicalVolume();
    if (pv){
      // G4cout << " got physical volume: " << pv->GetName() << G4endl;

      auto is_rotated = false;
      auto is_translated = false;
      // Get the frame rotation and translation of the current volume
      auto frameRotation = pv->GetFrameRotation();
      auto frameTranslation = pv->GetFrameTranslation();

      if (frameRotation)
        is_rotated = frameRotation->norm2() > 1e-10 ? true : false;

      is_translated = frameTranslation.mag2() > 1e-10 ? true : false;

      if(is_rotated){
        G4cout << " got frame rotation: " << *frameRotation << G4endl;
        G4cout << " performing inverse rotation... " << G4endl;
        if (direction==Transform::LocalToGlobal)
          globalPosition = frameRotation->inverse() * globalPosition;
        else if (direction==Transform::GlobalToLocal)
         globalPosition = *frameRotation * globalPosition;
      } else {
        G4cout << " no rotation. " << G4endl;
      }
      // if(is_translated){
      //   G4cout << " got frame translation: " << frameTranslation << G4endl;
      //   G4cout << " performing inverse translation... " << G4endl;
      //   globalPosition -= frameTranslation;
      // } else {
      //   G4cout << " no translation. " << G4endl;
      // }
    } else {
        G4cout << " no physical volume found." << G4endl;
    }
    // Move to the parent volume
    currentVolume = currentVolume->GetParentPtr();
    // G4LogicalVolume* motherLogical = currentVolume->GetMotherLogical();
    // if (motherLogical) {
    //     // G4cout << " end of geoemtry tree... " << G4endl;
    //     break; // This is the world volume, stop traversal
    // }
    // // Find the physical volume corresponding to the mother
    // currentVolume = motherLogical->GetDaughter(0); // Traverse upwards
  }
  return globalPosition;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Generates a hash value from a vector of integers by concatenating them into a hyphen-separated string.
 *
 * If the input vector is empty, a warning is printed and the hash of an empty string is returned.
 *
 * @param indexes Vector of integers to be concatenated and hashed.
 * @return std::size_t Hash value of the resulting string.
 */
std::size_t svc::getHashedStrFromIndexes(const std::vector<int>& indexes){
  std::string hash_str;
  for(const auto& idx : indexes)
    hash_str+=std::to_string(idx)+"-";
  if (!hash_str.empty())
    hash_str.pop_back(); // Removes the last character
  else
    G4cout << "[WARNING]::Svc::getHashedStrFromIndexes:: Returning hashed key for empty string! " << G4endl;
  return std::hash<std::string>{}(hash_str);
}


