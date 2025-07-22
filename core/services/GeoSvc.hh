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

  // Delete the copy and move constructors
  GeoSvc(const GeoSvc &) = delete;

  GeoSvc &operator=(const GeoSvc &) = delete;

  GeoSvc(GeoSvc &&) = delete;

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

  ///\brief Get the main/top volume world pointer.
  WorldConstruction *World() const { return my_world; }
  WorldConstruction *World() { return my_world ? my_world : Build(); }
  
  /// 
  void SetWorld(WorldConstruction *world) { my_world = world; }

  ///
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
  
  ///
  void ParseTomlConfig() override {}

};

#endif  // Dose3D_GEOSVC_H
