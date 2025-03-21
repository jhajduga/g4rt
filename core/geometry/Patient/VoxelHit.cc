//
// Created by brachwal on 07.05.2020.
//

#include "VoxelHit.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include <iomanip>
#include <G4Gamma.hh>
#include <G4Electron.hh>
#include <G4Positron.hh>
#include <G4Neutron.hh>
#include <G4Proton.hh>
#include <G4Alpha.hh>
#include <numeric>
#include "Services.hh"

G4ThreadLocal G4Allocator<VoxelHit> *VoxelHitAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Fill(G4Step* aStep){
  if(m_Voxel.m_primaryID<0){
    // Fill current position
    auto position = aStep->GetPreStepPoint()->GetPosition(); // in world volume frame
    SetGravCentre(position);

    // Fill mass of the voxel
    if (m_Voxel.m_Volume>0.){
      auto density = aStep->GetPreStepPoint()->GetMaterial()->GetDensity();
      m_Voxel.m_Mass = density * m_Voxel.m_Volume;
    }

    // Fill energy deposited along G4Step (i.e. ionization)
    auto edep = aStep->GetTotalEnergyDeposit();
    m_Voxel.m_Edep += edep;
    if (edep>0){ // don't increase the vector size with zeros
      m_Voxel.m_stepsEdep.emplace_back(edep);
      if(m_Voxel.m_Mass!=0.)
      m_Voxel.m_Dose = (m_Voxel.m_Edep / m_Voxel.m_Mass) / gray;
      else {
        G4cout << "[WARNING]::VoxelHit::GetDose() The voxel mass is not set!" << G4endl;
      }
    }

    /// Fill track info
    FillTrack(aStep);

    // Set voxel primary (incident) particle info
    auto trkType = GetTrkType(aStep);
    auto trkEnergy = aStep->GetPreStepPoint()->GetKineticEnergy();
    SetPrimary(trkType,trkEnergy);

  } else { // this voxel hit has already been set, it should be updated..
    Update(aStep);
  }
}


////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Update(G4Step* aStep){
  // Fill current position
  auto position = aStep->GetPreStepPoint()->GetPosition(); // in world volume frame
  SetGravCentre(position);

  auto edep = aStep->GetTotalEnergyDeposit();
  m_Voxel.m_Edep += edep;
  if (edep>0){ // don't increase the vector size with zeros
      m_Voxel.m_stepsEdep.emplace_back(edep);
      if(m_Voxel.m_Mass!=0.)
        m_Voxel.m_Dose = (m_Voxel.m_Edep / m_Voxel.m_Mass) / gray;
      else {
        G4cout << "[WARNING]::VoxelHit::GetDose() The voxel mass is not set!" << G4endl;
      }
  }
  FillTrack(aStep);
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VoxelHit::GetTrkType(G4Step* aStep) const {
  auto dynamic = aStep->GetTrack()->GetDynamicParticle();
  auto trkDef = dynamic->GetDefinition();
  G4int trkType = -1;
  if (trkDef == G4Gamma::Definition())    trkType = 1;
  if (trkDef == G4Electron::Definition()) trkType = 2;
  if (trkDef == G4Positron::Definition()) trkType = 3;
  if (trkDef == G4Neutron::Definition())  trkType = 4;
  if (trkDef == G4Proton::Definition())   trkType = 5;
  if (trkDef == G4Alpha::Definition())   trkType = 6;
  return trkType;
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::FillTrack(G4Step* aStep){
  if(m_tracks_analysis){
    auto aTrack = aStep->GetTrack();
    auto trkID = aTrack->GetTrackID();
    auto ret = m_Voxel.m_trksId.insert(trkID);
    auto postStepPoint = aStep->GetPostStepPoint();
    auto preStepPoint = aStep->GetPreStepPoint();
      if (ret.second==true) { // new element inserted
        m_Voxel.m_trksTypeId.emplace_back(GetTrkType(aStep));
        m_Voxel.m_trksE.emplace_back(aTrack->GetDynamicParticle()->GetKineticEnergy());
        // m_Voxel.m_trksPotentialE.emplace_back((aTrack->GetDynamicParticle()->GetTotalEnergy())-(aTrack->GetDynamicParticle()->GetKineticEnergy())) (>dla niefotonów)
        m_Voxel.m_trksTheta.emplace_back(aTrack->GetDynamicParticle()->GetMomentum().theta());
        m_Voxel.m_trksLength.emplace_back(aTrack->GetTrackLength());
        m_Voxel.m_trksPosition.emplace_back(preStepPoint->GetPosition());
      } else {
        // auto evtGlobalTime = aTrack->GetGlobalTime();
        auto edep = aStep->GetTotalEnergyDeposit();
      }
      // time: since the beginning of the event
      m_global_time = aTrack->GetGlobalTime();
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::SetId(G4int xId, G4int yId, G4int zId) {
  if (m_Voxel.m_idx_x == -1 && m_Voxel.m_idx_y== -1 && m_Voxel.m_idx_z == -1) {
    m_Voxel.m_idx_x = xId;
    m_Voxel.m_idx_y = yId;
    m_Voxel.m_idx_z = zId;
  } else if (m_Voxel.m_idx_x != xId || m_Voxel.m_idx_y != yId || m_Voxel.m_idx_z != zId) {
    // TODO: handle this error!?
    G4cout <<"[ERROR]::VoxelHit::SetChannelID:: unadjusted data"<< G4endl;
    G4cout <<"[ERROR]::VoxelHit::SetChannelID:: previous X " << m_Voxel.m_idx_x << " given "<< xId << G4endl;
    G4cout <<"[ERROR]::VoxelHit::SetChannelID:: previous Y " << m_Voxel.m_idx_y << " given "<< yId << G4endl;
    G4cout <<"[ERROR]::VoxelHit::SetChannelID:: previous Z " << m_Voxel.m_idx_z << " given "<< zId << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::SetGlobalId(G4int xId, G4int yId, G4int zId) {
  if (m_Voxel.m_global_idx_x == -1 && m_Voxel.m_global_idx_y== -1 && m_Voxel.m_global_idx_z == -1) {
    m_Voxel.m_global_idx_x = xId;
    m_Voxel.m_global_idx_y = yId;
    m_Voxel.m_global_idx_z = zId;
  } else if (m_Voxel.m_global_idx_x != xId || m_Voxel.m_global_idx_y != yId || m_Voxel.m_global_idx_z != zId) {
    // TODO: handle this error!?
    G4cout <<"[ERROR]::VoxelHit::SetGlobalChannelID:: unadjusted data"<< G4endl;
    G4cout <<"[ERROR]::VoxelHit::SetGlobalChannelID:: previous X " << m_Voxel.m_global_idx_x << " given "<< xId << G4endl;
    G4cout <<"[ERROR]::VoxelHit::SetGlobalChannelID:: previous Y " << m_Voxel.m_global_idx_y << " given "<< yId << G4endl;
    G4cout <<"[ERROR]::VoxelHit::SetGlobalChannelID:: previous Z " << m_Voxel.m_global_idx_z << " given "<< zId << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::SetGravCentre(const G4ThreeVector& position){

  if(m_Voxel.m_GravitationalCentre.x()==0.)
    m_Voxel.m_GravitationalCentre.setX(position.x());
  else
    m_Voxel.m_GravitationalCentre.setX((position.x()+m_Voxel.m_GravitationalCentre.x())/2.);

  if(m_Voxel.m_GravitationalCentre.y()==0.)
    m_Voxel.m_GravitationalCentre.setY(position.y());
  else
    m_Voxel.m_GravitationalCentre.setY((position.y()+m_Voxel.m_GravitationalCentre.y())/2.);

  if(m_Voxel.m_GravitationalCentre.z()==0.)
    m_Voxel.m_GravitationalCentre.setZ(position.z());
  else
    m_Voxel.m_GravitationalCentre.setZ((position.z()+m_Voxel.m_GravitationalCentre.z())/2.);
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VoxelHit::GetID(G4int axisId) const {
  switch (axisId){
    case 0: return m_Voxel.m_idx_x;
    case 1: return m_Voxel.m_idx_y;
    case 2: return m_Voxel.m_idx_z;
    default: G4cout <<"[ERROR]::VoxelHit::GetID:: unset data"<< G4endl;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VoxelHit::GetGlobalID(G4int axisId) const {
  switch (axisId){
    case 0: return m_Voxel.m_global_idx_x;
    case 1: return m_Voxel.m_global_idx_y;
    case 2: return m_Voxel.m_global_idx_z;
    default: G4cout <<"[ERROR]::VoxelHit::GetID:: unset data"<< G4endl;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Print() const {
  const_cast<VoxelHit*>(this)->Print();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Outputs a temporary placeholder message.
 *
 * This method currently writes "Temp" to G4cout as a temporary indicator of the print functionality.
 * Detailed voxel hit information, including IDs, mass, and volume, is not printed as the original
 * logging code has been commented out.
 */
void VoxelHit::Print(){
  // LOGSVC_INFO("Voxel ID ({},{},{})/({},{},{}) \n\tMass {}, Volume {}"
  // ,m_Voxel.m_global_idx_x,m_Voxel.m_global_idx_y,m_Voxel.m_global_idx_z
  // ,m_Voxel.m_idx_x,m_Voxel.m_idx_y,m_Voxel.m_idx_z
  // ,m_Voxel.m_Mass,m_Voxel.m_Volume);
  G4cout <<"Temp"<< G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int,G4double>> VoxelHit::GetTrkEnergy() const {
  std::vector<std::pair<G4int,G4double>> trkIdE;
  auto itE = m_Voxel.m_trksE.begin();
  for (auto itId=m_Voxel.m_trksId.begin(); itId!=m_Voxel.m_trksId.end(); ++itId)
    trkIdE.emplace_back(*itId,*itE++);
  return trkIdE;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int,G4double>> VoxelHit::GetTrkTheta() const {
  std::vector<std::pair<G4int,G4double>> trkIdTheta;
  auto itTheta = m_Voxel.m_trksTheta.begin();
  for (auto itId=m_Voxel.m_trksId.begin(); itId!=m_Voxel.m_trksId.end(); ++itId)
    trkIdTheta.emplace_back(*itId,*itTheta++);
  return trkIdTheta;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int,G4double>> VoxelHit::GetTrkLength() const {
  std::vector<std::pair<G4int,G4double>> trkIdLength;
  auto itL = m_Voxel.m_trksLength.begin();
  for (auto itId=m_Voxel.m_trksId.begin(); itId!=m_Voxel.m_trksId.end(); ++itId)
    trkIdLength.emplace_back(*itId,*itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int,G4double>> VoxelHit::GetUserTrkLength() const {
  std::vector<std::pair<G4int,G4double>> trkIdLength;
  auto itL = m_Voxel.m_usrTrksLength.begin();
  for (auto itId=m_Voxel.m_usrTrksId.begin(); itId!=m_Voxel.m_usrTrksId.end(); ++itId)
    trkIdLength.emplace_back(*itId,*itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int,G4ThreeVector>> VoxelHit::GetTrkPosition() const {
  std::vector<std::pair<G4int,G4ThreeVector>> trkPosition;
  auto itP = m_Voxel.m_trksPosition.begin();
  for (auto itId=m_Voxel.m_trksLength.begin(); itId!=m_Voxel.m_trksLength.end(); ++itId)
    trkPosition.emplace_back(*itId,*itP++);
  return trkPosition;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int,G4int>> VoxelHit::GetTrkType() const {
  std::vector<std::pair<G4int,G4int>> trkTypeId;
  auto itType = m_Voxel.m_trksTypeId.begin();
  for (auto itId=m_Voxel.m_trksId.begin(); itId!=m_Voxel.m_trksId.end(); ++itId)
    trkTypeId.emplace_back(*itId,*itType++);
  return trkTypeId;
}

////////////////////////////////////////////////////////////////////////////////
///
G4double VoxelHit::GetPrimaryTrkEnergy() const {
  for (auto& iIdE : GetTrkEnergy())
    if(iIdE.first==1) return iIdE.second;
  return -1; // No primary in this voxel hit
}

////////////////////////////////////////////////////////////////////////////////
///
G4double VoxelHit::GetMeanEnergyDeposit() const {
  if(m_Voxel.m_stepsEdep.size()>0){
    double sum = std::accumulate(m_Voxel.m_stepsEdep.begin(), m_Voxel.m_stepsEdep.end(), 0.0);
    return sum / m_Voxel.m_stepsEdep.size();
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::PrintEvtInfo() const {
  for (int i=0; i < m_evtPrimariesIncidentE.size(); ++i){
    G4cout << "[INFO]::VoxelHit::EvtInfo() incident primary("<<i<<") energy: "<< m_evtPrimariesIncidentE.at(i) << " [keV]"  << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
std::size_t VoxelHit::GetGlobalHashedStrId() const {
  return std::hash<std::string>{}(std::to_string(m_Voxel.m_global_idx_x)
                                  +std::to_string(m_Voxel.m_global_idx_y)
                                  +std::to_string(m_Voxel.m_global_idx_z) );
}

////////////////////////////////////////////////////////////////////////////////
///
std::size_t VoxelHit::GetHashedStrId() const {
  return std::hash<std::string>{}(std::to_string(m_Voxel.m_global_idx_x)
                                  +std::to_string(m_Voxel.m_global_idx_y)
                                  +std::to_string(m_Voxel.m_global_idx_z) 
                                  +std::to_string(m_Voxel.m_idx_x) 
                                  +std::to_string(m_Voxel.m_idx_y) 
                                  +std::to_string(m_Voxel.m_idx_z));
}

////////////////////////////////////////////////////////////////////////////////
///
bool VoxelHit::operator==(const VoxelHit& other) const {
  return  m_Voxel.m_global_idx_x == other.m_Voxel.m_global_idx_x &&
          m_Voxel.m_global_idx_y == other.m_Voxel.m_global_idx_y &&
          m_Voxel.m_global_idx_z == other.m_Voxel.m_global_idx_z &&
          m_Voxel.m_idx_x == other.m_Voxel.m_idx_x &&
          m_Voxel.m_idx_y == other.m_Voxel.m_idx_y &&
          m_Voxel.m_idx_z == other.m_Voxel.m_idx_z;
}

////////////////////////////////////////////////////////////////////////////////
///
bool VoxelHit::IsAligned(const VoxelHit& other, bool global_and_local) const {
  if(global_and_local){
    return *this == other;
  }
  // only global
  return  m_Voxel.m_global_idx_x == other.m_Voxel.m_global_idx_x &&
          m_Voxel.m_global_idx_y == other.m_Voxel.m_global_idx_y &&
          m_Voxel.m_global_idx_z == other.m_Voxel.m_global_idx_z;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Accumulates data from another VoxelHit if the hits are aligned.
 *
 * Checks whether the current voxel hit is aligned with the provided hit using either a combined global
 * and local check or only a global check, based on the flag. If aligned, the method cumulates the energy
 * dose by scaling it according to the ratio of voxel volumes when the flag is false, or by directly adding
 * the dose via the overloaded operator when the flag is true. In case of misalignment, diagnostic information
 * is printed for both voxel hits.
 *
 * @param other The other VoxelHit to cumulate data from.
 * @param global_and_local_allignemnt_check If true, both global and local indices are used for alignment checking;
 *                                          if false, only the voxel cell mapping is considered.
 * @return VoxelHit& Reference to this instance with updated cumulative data.
 */
VoxelHit& VoxelHit::Cumulate(const VoxelHit& other, bool global_and_local_allignemnt_check){
  // if(!global_and_local_allignemnt_check)
  //   Print(); // LOGSVC_INFO("+"); other.Print();
  if(IsAligned(other,global_and_local_allignemnt_check)){
    if(!global_and_local_allignemnt_check){ // cell/voxel
      // G4cout << "[INFO]::VoxelHit::Cumulate m_Voxel.m_Dose pre summ "<<m_Voxel.m_Dose << G4endl;
      // G4cout << "[INFO]::VoxelHit::Cumulate other.GetVolume() "<<other.GetVolume() << G4endl;
      // G4cout << "[INFO]::VoxelHit::Cumulate GetVolume(); "<<GetVolume() << G4endl;


      m_Voxel.m_Dose += other.GetDose()*other.GetVolume() / GetVolume();
      
      // G4cout << "[INFO]::VoxelHit::Cumulate m_Voxel.m_Dose post summ "<<m_Voxel.m_Dose << G4endl;

    } else {
      return *this+=other;
    }
  } else {
    // LOGSVC_WARN("Trying to cumulate misaligned VoxelHits...");
    Print();
    // LOGSVC_WARN("+");
    other.Print();
  }
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
///
VoxelHit& VoxelHit::operator+=(const VoxelHit& other){
  // TODO: m_Voxel.m_Dose+=other.GetDose()*other.GetVolume() / GetVolume(); Tu też? 
  m_Voxel.m_Dose+=other.GetDose();
  return *this;
}

