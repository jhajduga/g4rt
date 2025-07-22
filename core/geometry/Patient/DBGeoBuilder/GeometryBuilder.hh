#ifndef GEOMETRY_BUILDER_HH
#define GEOMETRY_BUILDER_HH

#include <string>
#include <vector>

#include "G4PVPlacement.hh"
#include "IPhysicalVolume.hh"
#include "toml.hh"
#include "TomlConfigurable.hh"
#include "Services.hh"

// Builds and manages a db based geometry
class GeometryBuilder : public TomlConfigModule {
  private:
    GeometryBuilder();
    ~GeometryBuilder();
    
    // Config parsing
    void ParseTomlConfig() override;
    G4bool LoadDefaultParameterization();
    G4bool LoadParameterization();
    

    // Phantom parameters
    G4double m_centrePositionX = 0.0;
    G4double m_centrePositionY = 0.0;
    G4double m_centrePositionZ = 0.0;
    G4double m_phantomRotationX = 0.0;
    G4double m_phantomRotationY = 0.0;
    G4double m_phantomRotationZ = 0.0;

    G4RotationMatrix m_rot;

    std::vector<std::string> m_exclusde_object_list;
  public:
    // Singleton access
    static GeometryBuilder* GetInstance();

    // Geometry lifecycle
    void Build(G4VPhysicalVolume* parentPV);
    
    
    class Config { // Maybe not neccesary to exist? 
      public:
      std::string tempName = "None";  };
    
    Config m_det_config;
    
};

#endif // GEOMETRY_BUILDER_HH