//
// Created by brachwal on 11.08.2020.
//

#ifndef DOSE3D_TYPES_HH
#define DOSE3D_TYPES_HH

#include <memory>
#include "G4ThreeVector.hh"
#include "G4VPhysicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4Material.hh"

using VecG4double = std::vector<G4double>;
using VecG4doubleUPtr = std::unique_ptr<VecG4double>;
using VecG4doubleSPtr = std::shared_ptr<VecG4double>;
using VecG4doubleWPtr = std::weak_ptr<VecG4double>;

using G4VPhysicalVolumeUPtr = std::unique_ptr<G4VPhysicalVolume>;
using G4VPhysicalVolumeSPtr = std::shared_ptr<G4VPhysicalVolume>;
using G4VPhysicalVolumeWPtr = std::weak_ptr<G4VPhysicalVolume>;

using G4RotationMatrixUPtr = std::unique_ptr<G4RotationMatrix>;
using G4RotationMatrixSPtr = std::shared_ptr<G4RotationMatrix>;
using G4RotationMatrixWPtr = std::weak_ptr<G4RotationMatrix>;

using G4MaterialUPtr = std::unique_ptr<G4Material>;
using G4MaterialSPtr = std::shared_ptr<G4Material>;
using G4MaterialWPtr = std::weak_ptr<G4Material>;

////////////////////////////////////////////////////////////////////////////////
///\brief geometry definitions types
enum class EHeadModel {
  None,
  BeamCollimation
};

////////////////////////////////////////////////////////////////////////////////
///\brief MLC types
enum class EMlcModel {
  None,
  Millennium, // Varian
  HD120,       // Varian
  Simplified
};


////////////////////////////////////////////////////////////////////////////////
///
namespace Scoring {
  enum class Type {
    Cell,
    Voxel
  };
  std::string to_string(Scoring::Type type);
}

////////////////////////////////////////////////////////////////////////////////
///
class GeoSvc;
class GeoComponet {
  public:
    virtual ~GeoComponet(){}
    virtual void AcceptGeoVisitor(GeoSvc *visitor) const = 0;
    virtual void ExportCellPositioningToCsv(const std::string& path_to_output_dir) const = 0;
    virtual void ExportVoxelPositioningToCsv(const std::string& path_to_output_dir) const {};
    virtual void ExportPositioningToTFile(const std::string& path_to_output_dir) const = 0;
    virtual void ExportToGateCsv(const std::string& path_to_output_dir) const {};
};

////////////////////////////////////////////////////////////////////////////////
///
class RunSvc;
class ControlPoint;
class RunComponet {
  public:
    virtual ~RunComponet(){}
    virtual void AcceptRunVisitor(RunSvc *visitor) = 0;
    virtual void SetRunConfiguration(const ControlPoint* ) = 0;
};

////////////////////////////////////////////////////////////////////////////////
///
enum class FieldType {
  Rectangular,
  Elipsoidal,
  RTPlan,
  CustomPlan
};
#endif //DOSE3D_TYPES_HH
