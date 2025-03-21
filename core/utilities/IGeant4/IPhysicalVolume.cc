#include "IPhysicalVolume.hh"
#include "G4LogicalVolume.hh"

////////////////////////////////////////////////////////////////////////////////
///
void IPhysicalVolume::ParameterisationInstantiation(IParameterisation instanceType) {
  if (IsParameterised()) {
    G4cout << "[INFO]:: ParameterisationInstantiation ";
    G4cout << " for " << GetParentPtr()->GetPhysicalVolume()->GetLogicalVolume()->GetName() << G4endl;
    if (instanceType == IParameterisation::SIMULATION)
      SimulationParameterisation(); // supposed to call overridden function!
    if (instanceType == IParameterisation::EXPORT)
      ExportParameterisation();     // supposed to call overridden function!
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4LogicalVolume *IPhysicalVolume::CloneLV(G4LogicalVolume *lVolume, G4String newName) {
  auto solid = lVolume->GetSolid();
  auto material = lVolume->GetMaterial();
  return new G4LogicalVolume(solid, material, newName, 0, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
///
void IPhysicalVolume::SimulationParameterisation() {
  G4cout << "[ERROR]:: SimulationParameterisation() is supposed to be implemented in the derived class! " << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
void IPhysicalVolume::ExportParameterisation() {
  G4cout << "[ERROR]:: ExportParameterisation() is supposed to be implemented in the derived class! " << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
size_t IPhysicalVolume::GetNoOfWorldVolumes(int tree_depth) const {
  auto pv = GetPhysicalVolume();
  auto depth = GetNDepth(pv,0);
  if(tree_depth==-1){ // count all volumes in the World
    return GetNDaughters(pv,0,depth);
  }

  if(tree_depth<static_cast<int>(depth)){
    return GetNDaughters(pv,0,tree_depth+1);
  }

  G4cout << "[ERROR]:: IPhysicalVolume::GetNoOfWorldVolumes; wrong tree depth given: "<< tree_depth << G4endl;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
///
size_t IPhysicalVolume::GetWorldVolumesTreeDepth() const {
  return GetNDepth(GetPhysicalVolume(),0);
}

////////////////////////////////////////////////////////////////////////////////
///
size_t IPhysicalVolume::GetNDaughters(G4VPhysicalVolume* pv, size_t parentDepth, size_t stopDepth) const {
  auto lv = pv->GetLogicalVolume();
  auto nDaughters = lv->GetNoDaughters();
  auto currentDepth = parentDepth+1;
  if(nDaughters==0 || currentDepth == stopDepth){
    return 1;
  }
  size_t nVolumes=1; // include already "this" volume
  for (size_t i = 0; i < nDaughters; i++) {
    nVolumes+=GetNDaughters(lv->GetDaughter(i),currentDepth,stopDepth);
  }
  return nVolumes;
}

////////////////////////////////////////////////////////////////////////////////
///
size_t IPhysicalVolume::GetNDepth(G4VPhysicalVolume* pv, size_t parentDepth) const {
  auto lv = pv->GetLogicalVolume();
  size_t nDepth=0;
  auto nDaughters = lv->GetNoDaughters();
  auto currentDepth = parentDepth+1;
  if(nDaughters==0){
    return currentDepth;
  }
  for (size_t i = 0; i < nDaughters; i++) {
    auto nDepthNew = GetNDepth(lv->GetDaughter(i),currentDepth);
    nDepth = nDepthNew > currentDepth ? nDepthNew : currentDepth;
  }
  return nDepth;
}

////////////////////////////////////////////////////////////////////////////////
///
void IPhysicalVolume::FillNDepthPVTreeContainer(std::vector<G4VPhysicalVolume*>& container, G4VPhysicalVolume* pv) const {
  container.push_back(pv);
  auto lv = pv->GetLogicalVolume();
  auto nDaughters = lv->GetNoDaughters();
  if(nDaughters==0){
    return;
  }
  for (size_t i = 0; i < nDaughters; i++) {
    FillNDepthPVTreeContainer(container,lv->GetDaughter(i));
  }
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4VPhysicalVolume*> IPhysicalVolume::GetWorldVolumesTreeContainer() const {
  std::vector<G4VPhysicalVolume*> my_tree;
  FillNDepthPVTreeContainer(my_tree,GetPhysicalVolume());
  return my_tree;
}

////////////////////////////////////////////////////////////////////////////////
///
G4VPhysicalVolume* IPhysicalVolume::GetPhysicalVolume(const std::string& name ) const { 
  if(name.empty())
    return m_physical_volume; 
  auto tree_volumes = GetWorldVolumesTreeContainer();
  for(const auto& volume : tree_volumes){
    auto pvname = volume->GetName();
    if(pvname.compare(name)==0)
      return volume;
  }
  G4cout << "[ERROR]:: IPhysicalVolume::GetPhysicalVolume: not found asked PV with the name: "<< name << G4endl;
  return m_physical_volume;
}

////////////////////////////////////////////////////////////////////////////////
///
G4LogicalVolume* IPhysicalVolume::GetLogicalVolume(const std::string& name ) const { 
  if(name.empty())
    return m_physical_volume->GetLogicalVolume(); 
  auto tree_volumes = GetWorldVolumesTreeContainer();
  for(const auto& volume : tree_volumes){
    auto lv = volume->GetLogicalVolume();
    auto lvname = lv->GetName();
    if(lvname.compare(name)==0){
      return lv;
    }
  }
  G4cout << "[ERROR]:: IPhysicalVolume::GetLogicalVolume: not found asked LV with the name: "<< name << G4endl;
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
void IPhysicalVolume::Construct(IPhysicalVolume* parent, const G4ThreeVector& position){
  m_position = position;
  m_parent = parent;                           // logical structure
  auto thisPV = GetPhysicalVolume();
  auto parentPV = parent->GetPhysicalVolume(); // physical structure
  if(!parentPV){
    G4cout << "[INFO]:: IPhysicalVolume::Construct: No direct pv to link..." << G4endl;
    while (!parentPV){ // find nearest pv in hierarchy
      auto next_parent = parent->GetParentPtr();
      if(next_parent){
        parentPV = next_parent->GetPhysicalVolume();
      } else { // reached the top level
        if(parent->GetName() != "WorldConstruction"){
          G4cout << "[FATAL]:: IPhysicalVolume::Construct: not found any parent with physical volume!" << G4endl;
          throw std::invalid_argument("Invalid IPhysicalVolume hierarchy");
        }
      }
      parent = next_parent;
    }
    G4cout << "[INFO]:: Link logical "<< GetName() << " with G4PV: " << parentPV->GetName() << G4endl;
  }
  Construct(parentPV);
}
