/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.12.2017
*
*/

#ifndef Dose3D_GEOSVC_H
#define Dose3D_GEOSVC_H

#include "Types.hh"
#include "TomlConfigurable.hh"

class WorldConstruction;
class D3DDetector;
class VPatient;
class VMlc;

////////////////////////////////////////////////////////////////////////////////
///
///\class GeoSvc
///\brief The geometry management service.
/// It is a singleton type the pointer of which can be asses trough the templated method:
/// Service<GeoSvc>()
class GeoSvc : public TomlConfigurable{
  private:
  GeoSvc();

  ~GeoSvc();

  bool m_is_tfile_exported = false;
  bool m_is_gdml_exported = false;

  std::string m_world_file_name = "world_geometry";

  /**
 * @brief Deleted copy constructor to enforce singleton behavior.
 */
  GeoSvc(const GeoSvc &) = delete;

  /**
 * @brief Deleted copy assignment operator to enforce singleton behavior.
 */
GeoSvc &operator=(const GeoSvc &) = delete;

  /**
 * @brief Move constructor is deleted to enforce singleton behavior.
 */
GeoSvc(GeoSvc &&) = delete;

  /**
 * @brief Move assignment operator is deleted to enforce singleton behavior.
 */
GeoSvc &operator=(GeoSvc &&) = delete;

  ///\brief Main/top volume world pointer.
  WorldConstruction *my_world = nullptr;

  ///\brief Keep track of service initialization status.
  bool m_isInitialized = false;

  ///
  std::vector<const GeoComponet*> m_scoring_components;

  ///
  VPatient* m_patient = nullptr;

  ///\brief Virtual method implementation defining the list of configuration units for this module.
  void Configure() override;

  ///\brief Parse the User's request of saving the phsp plane.
  void ParseSavePhspPlaneRequest();

  ///
  void ExportCellPositioningToCsv() const;

  ///
  void ExportToGateGenericRepeater() const;

  public:
  ///\brief Static method to get instance of this singleton object.
  static GeoSvc *GetInstance();

  ///\brief Perform service and geometry related configuration initialization.
  void Initialize();

  ///\brief Virtual method implementation defining the default units configuration.
  void DefaultConfig(const std::string &unit);

  /// \brief Check if the main/top volume world is already built in the Geant4 framework.
  bool IsWorldBuilt() const;

  ///\brief Build the actual main/top volume in the Geant4 framework.
  WorldConstruction *Build();

  /**
 * @brief Access the top-level world geometry.
 *
 * @return WorldConstruction* Pointer to the current top-level WorldConstruction instance representing the simulation world, or `nullptr` if no world has been set or built.
 */
  WorldConstruction *World() const { return my_world; }
  /**
 * @brief Get the main world geometry, constructing it if necessary.
 *
 * If the world has not been built, this method constructs it before returning.
 *
 * @return WorldConstruction* Pointer to the main world geometry.
 */
WorldConstruction *World() { return my_world ? my_world : Build(); }
  
  /**
 * @brief Set the current main/top world geometry.
 *
 * Assigns the given WorldConstruction instance as the service's current world.
 *
 * @param world Pointer to the WorldConstruction to use as the current world.
 */
  void SetWorld(WorldConstruction *world) { my_world = world; }

  /**
 * @brief Register the patient geometry used by the geometry service.
 *
 * Associates the given VPatient instance with the service so it becomes the active patient geometry for subsequent geometry operations.
 *
 * @param patient Pointer to the patient geometry to register; may be `nullptr` to clear the current registration.
 */
  void RegisterPatient(VPatient* patient) { m_patient = patient; }
  
  ///
  VPatient* Patient();

  ///
  std::vector<VPatient*> CustomDetectors();

  ///
  VMlc* MLC();

  ///\brief Update the geometry.
  WorldConstruction *Update(int runId);

  ///\brief Destroy the main/top volume world in the Geant4 framework.
  void Destroy();

  ///\brief Get the actual device type registered in the service.
  EHeadModel GetHeadModel() const;

  ///
  EMlcModel GetMlcModel() const;

  ///
  void RegisterScoringComponent(const GeoComponet *element);

  ///
  void PerformDefaultExport();

  //
  void PerformRequestedExport();

  ///
  static std::string GetOutputDir();

  ///
  void WriteWorldToTFile();

  ///
  void WritePatientToCsvCT();

  ///
  void WritePatientToDicomCT();

  ///
  void WriteWorldToGdml(); 

  ///
  void WriteScoringComponentsPositioningToTFile() const;

  ///
  void WriteScoringComponentsPositioningToCsv() const;
  
  /**
 * @brief Overrides the TOML configuration parsing method with no implementation.
 *
 * This method is intentionally left empty as TOML configuration parsing is not required for this service.
 */
  void ParseTomlConfig() override {}

};

#endif  // Dose3D_GEOSVC_H