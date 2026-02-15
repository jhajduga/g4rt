#include "G4NistManager.hh"
#include "GeometryDBReader.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "G4TessellatedSolid.hh"
#include "G4TriangularFacet.hh"
#include "NTupleEventAnalisys.hh"
#include "G4UserLimits.hh"
#include "GeometryBuilder.hh"
#include "Services.hh"
#include "D3DCell.hh"
#include "D3DDetector.hh"
#include <algorithm>
#include <cctype>

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a GeometryBuilder and initializes it as a TomlConfigModule named "GeometryBuilder".
 */
GeometryBuilder::GeometryBuilder():TomlConfigModule("GeometryBuilder"){

}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Parse TOML configuration and populate geometry parameters.
 *
 * Reads the configured TOML file (path and prefix obtained via the TomlConfigModule
 * interface) and extracts geometry placement values into the instance members:
 * - Centre position: m_centrePositionX, m_centrePositionY, m_centrePositionZ
 * - Phantom rotation (degrees): m_phantomRotationX, m_phantomRotationY, m_phantomRotationZ
 * - Excluded object name list: m_exclusde_object_list (case preserved)
 *
 * If a Position or Rotation array entry is missing, the corresponding value
 * defaults to 0.0. If an ExcludeObjList array exists, each string entry is
 * appended to m_exclusde_object_list. The function relies on toml::parse_file
 * to read and parse the file.
 *
 * @note The function obtains the TOML file path and prefix by calling
 * SetTomlConfigFile(), GetTomlConfigFile(), and GetTomlConfigPrefix() on the
 * TomlConfigModule base. It does not perform additional validation of the
 * file path beyond the parser.
 *
 * @throws toml::parse_error If the TOML file cannot be parsed.
 */
void GeometryBuilder::ParseTomlConfig(){
  SetTomlConfigFile();
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();

  // std::cout << "Temp" << "\n";
                //   LOGSVC_INFO("Importing configuration from:\n{}",configFile); // Not Logable RN
  // if (!svc::checkIfFileExist(configFile)) {
  //   FATAL_GEO("File {} not fount.", configFile);
  //   G4Exception("GeometryBuilder", "ParseTomlConfig", FatalErrorInArgument, "");
  // }

  std::cout << "Importing configuration from:\n" << configFile << "\n";
  auto config = toml::parse_file(configFile);
  std::cout << config << "\n";


  m_centrePositionX = config[configPrefix]["Position"][0].value_or(0.0);
  m_centrePositionY = config[configPrefix]["Position"][1].value_or(0.0);
  m_centrePositionZ = config[configPrefix]["Position"][2].value_or(0.0);

  m_phantomRotationX = config[configPrefix]["Rotation"][0].value_or(0.0);
  m_phantomRotationY = config[configPrefix]["Rotation"][1].value_or(0.0);
  m_phantomRotationZ = config[configPrefix]["Rotation"][2].value_or(0.0);

  auto* arr = config[configPrefix]["ExcludeObjList"].as_array();
  if (arr) {
      for (const auto& val : *arr) {
          if (const auto* s = val.as_string()) {
            m_exclusde_object_list.push_back(s->get());
          }
      }
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Provides global access to the unique GeometryBuilder instance.
 *
 * @return Pointer to the singleton GeometryBuilder instance.
 */
GeometryBuilder* GeometryBuilder::GetInstance() {
  static GeometryBuilder instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroys the GeometryBuilder instance.
 *
 * Default destructor with no custom cleanup logic.
 */
GeometryBuilder::~GeometryBuilder() {
  
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads geometry parameterization from a TOML configuration file if available, or uses default values.
 *
 * Checks for the existence of a TOML configuration file and parses it to set geometry parameters. If the file does not exist, loads default parameterization instead.
 *
 * @return Always returns true.
 */
G4bool GeometryBuilder::LoadParameterization(){
  // Configurable::ValidateConfig();
  if(IsTomlConfigExists()){
    ParseTomlConfig();
  }
  else{
    LoadDefaultParameterization();
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads default geometry parameterization.
 *
 * This stub implementation always returns true and does not perform any parameter loading.
 *
 * @return G4bool Always returns true.
 */
G4bool GeometryBuilder::LoadDefaultParameterization(){
  return true;
}


/**
 * @brief Return a lowercase copy of the given string.
 *
 * Converts each character in the input to its lowercase equivalent and returns
 * the resulting string. Characters are cast to `unsigned char` before calling
 * `std::tolower` to avoid undefined behavior for negative `char` values.
 *
 * @param input The string to convert.
 * @return std::string Lowercase copy of `input`.
 */
static std::string to_lower(const std::string& input) {
    std::string out;
    out.resize(input.size());
    std::transform(input.begin(), input.end(), out.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return out;
}

////////////////////////////////////////////////////////////////////////////////
/**
   * @brief Build and place tessellated geometry from the database into a parent world volume.
   *
   * Loads geometry parameterization, iterates database objects, and for each object that is not
   * excluded (case-insensitive substring match against the configured exclusion list) and whose
   * `sc_id` is empty or the literal string `"nan"`, constructs a G4TessellatedSolid from the
   * object's vertices and triangle indices, assigns the material from the Materials service, and
   * creates and places a corresponding G4LogicalVolume into the supplied parent world. Placement
   * applies the configured phantom rotations and an environment-dependent translation.
   *
   * Notes:
   * - Objects with `sc_id` that is neither empty nor `"nan"` are skipped.
   * - Exclusion matching is case-insensitive and uses substring matching.
   * - Material lookup is performed via ConfigSvc ("MaterialsSvc").
   *
   * @param parentWorld Parent Geant4 physical volume under which assembled volumes are placed.
   */
void GeometryBuilder::Build(G4VPhysicalVolume *parentWorld) {
  LoadParameterization();
  // auto* nist = G4NistManager::Instance();
  // auto* mat  = nist->FindOrBuildMaterial(m_phantomMedium); // Default temp material
    const auto& list =  GeometryDBReader::Instance().GetData();
  


  for (const auto& obj : list) {
    std::string component_lower = to_lower(obj.component);

    // Check if any exclusion string is a case-insensitive substring of the component name
    bool is_excluded = std::any_of(
        m_exclusde_object_list.begin(),
        m_exclusde_object_list.end(),
        [&](const std::string& excl) {
            return component_lower.find(to_lower(excl)) != std::string::npos;
        });

    if (is_excluded) {
        std::cout << "Skipping excluded object: " << obj.component << "\n";
        continue;
    }

    if (obj.sc_id != "nan" && obj.sc_id != "" && !obj.sc_id.empty()) {
      continue;
    }
    auto* tessSolid = new G4TessellatedSolid(obj.component + "_Solid");

    for (size_t i = 0; i < obj.nodes.size(); ++i) {
      const auto& t = obj.nodes[i];
      G4ThreeVector p1 = obj.vertices[t[0]];
      G4ThreeVector p2 = obj.vertices[t[1]];
      G4ThreeVector p3 = obj.vertices[t[2]];
      
      G4ThreeVector geomN = (p2 - p1).cross(p3 - p1).unit();
      
      
      G4ThreeVector desiredN = obj.normals[i];
      tessSolid->AddFacet(new G4TriangularFacet(p1, p2, p3, ABSOLUTE));

      }
      
    auto mat = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", std::string(obj.mat));

    tessSolid->SetSolidClosed(true);
    auto* componentLV = new G4LogicalVolume(tessSolid, mat.get(), obj.component + "_Logic");
    G4ThreeVector tranlation;
      if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "IbaImRT_3mf"){
        tranlation = G4ThreeVector(-95.0,90.0,90.0);
      }
      else if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "ModularWaterPhantom_3mf"){
        tranlation = G4ThreeVector(-271.0,275.0,225.0);
      }
      m_rot = G4RotationMatrix();
      m_rot.rotateX(m_phantomRotationX * deg);
      m_rot.rotateX(180.0 * deg);
      m_rot.rotateY(m_phantomRotationY * deg);
      m_rot.rotateZ(m_phantomRotationZ * deg);
      new G4PVPlacement(&m_rot, tranlation, obj.component + "_PV", componentLV, parentWorld, false, 0);
    }

  }
    
  