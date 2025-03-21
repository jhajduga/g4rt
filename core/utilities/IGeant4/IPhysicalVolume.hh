/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 14.02.2018
*
*/

#ifndef Dose3D_PHYSICALVOLUME_HH
#define Dose3D_PHYSICALVOLUME_HH

#include <set>
#include "G4PVPlacement.hh"
#include "G4LogicalVolume.hh"
#include "G4ThreeVector.hh"
#include "GeoSvc.hh"

class G4Run;

enum class IParameterisation {
  EXPORT,
  SIMULATION
};

///\class IPhysicalVolume
///\brief The pure virtual base class for any Geant4PhysicalVolume object constituting an interface to the Dose3D app framework.
class IPhysicalVolume {
  public:
  ///
  virtual void Construct(G4VPhysicalVolume*) = 0;

  ///
  void Construct(IPhysicalVolume* parent, const G4ThreeVector& position=G4ThreeVector());

  ///
  virtual void Destroy() = 0;

  ///
  virtual G4bool Update() = 0;

  ///
  virtual void Reset() = 0;

  ///
  virtual void WriteInfo() = 0;

  ///
  inline IPhysicalVolume* GetParentPtr() { return m_parent; }

  ///
  void ParameterisationInstantiation(IParameterisation type);

  ///
  virtual void SimulationParameterisation();

  ///
  virtual void ExportParameterisation();
  
  ///
  virtual void DefineSensitiveDetector(){};
  
  ///
  IPhysicalVolume() = delete;
  
  ///
  IPhysicalVolume(const std::string& name):m_name(name){}

  ///
  virtual ~IPhysicalVolume() = default;

  ///
  std::string GetName() const { return m_name; }

  ///
  std::string SetName(const std::string& name) { m_name=name; return m_name; }

  /// -1 : get number of volumes for the whole deep tree
  ///  0 : get number of volumes for the current tree level only
  ///  X : any natural number specify the tree level of interest
  size_t GetNoOfWorldVolumes(int tree_depth = -1) const;

  /// The depth of the tree is the length of the path 
  /// from this physical volumeto to the farthest daughter
  size_t GetWorldVolumesTreeDepth() const;

  std::vector<G4VPhysicalVolume*> GetWorldVolumesTreeContainer() const;

  /// 
  G4VPhysicalVolume* GetPhysicalVolume(const std::string& name=std::string() ) const;

  ///
  G4LogicalVolume* GetLogicalVolume(const std::string& name=std::string() ) const;

  protected:

  ///
  GeoSvc *m_geoSvc = GeoSvc::GetInstance();
  
  ///
  mutable IPhysicalVolume* m_parent = nullptr;

  ///
  G4ThreeVector m_position;

  ///
  G4bool IsParameterised() const { return is_parameterised; }

  ///
  void IsParameterised(G4bool value) { is_parameterised = value; }

  /// Clone Logical Volume utility
  G4LogicalVolume *CloneLV(G4LogicalVolume *lVolume, G4String newName);

  ///
  void SetPhysicalVolume(G4VPhysicalVolume* pv) { m_physical_volume=pv; }

  private:
  std::string m_name = "Noname";

  ///
  G4VPhysicalVolume* m_physical_volume = nullptr;

  /// Once any volume is being built with G4PhantomParameterisation, G4VNestedParameterisation or
  /// G4PartialPhantomParameterisation the following variable should be changed to true!
  G4bool is_parameterised = false;

  ///
  IParameterisation m_parameterization_instance = IParameterisation::SIMULATION;

  ///
  size_t GetNDaughters(G4VPhysicalVolume* pv, size_t parentDepth, size_t stopDepth) const;

  ///
  size_t GetNDepth(G4VPhysicalVolume* pv, size_t parentDepth=0) const;

  ///
  void FillNDepthPVTreeContainer(std::vector<G4VPhysicalVolume*>& container, G4VPhysicalVolume* pv) const;
};

#endif  // Dose3D_PHYSICALVOLUME_HH
