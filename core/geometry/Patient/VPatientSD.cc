#include "G4SDManager.hh"
#include "VPatientSD.hh"
#include "VPatient.hh"
#include "Services.hh"
#include <string>
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"
#include "NTupleEventAnalisys.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a VPatientSD sensitive detector.
 *
 * Initializes the sensitive detector with the provided unique name.
 *
 * @param sdName The identifier used to name the sensitive detector.
 */
VPatientSD::VPatientSD(const G4String& sdName):G4VSensitiveDetector(sdName){}
// VPatientSD::VPatientSD(const G4String& sdName):G4VSensitiveDetector(sdName),Logable("GeoAndScoring"){}

////////////////////////////////////////////////////////////////////////////////
/**
   * @brief Constructs a patient sensitive detector with a specified name and global center.
   *
   * This constructor initializes a VPatientSD instance by setting the sensitive detector's name and
   * its center in global coordinates. It calls the base class constructor (G4VSensitiveDetector) with
   * the provided detector name.
   *
   * @param sdName The identifier for the sensitive detector.
   * @param centre The global coordinates of the detector's center.
   */
VPatientSD::VPatientSD(const G4String& sdName, const G4ThreeVector& centre)
  :G4VSensitiveDetector(sdName),m_centre_in_global_coordinates(centre){}
  // :G4VSensitiveDetector(sdName),m_centre_in_global_coordinates(centre),Logable("GeoAndScoring"){}

////////////////////////////////////////////////////////////////////////////////
/// Note that a SD can declare more than one hits collection being groupped by runCollName!
/// Bartek's note: Typically the adding HC call is being placed inside SD constructor, 
/// within this application framework I've exposed this to the level of UserDecectorClass::DefineSensitiveDetector()
/**
 * @brief Adds a new hits collection to the sensitive detector.
 *
 * This function registers a hits collection under the specified run collection name by creating a new scoring volume.
 * It verifies that the collection has not been added before and checks its compatibility with existing collections 
 * associated with the same run collection. Upon success, the hits collection is added to the detector's list and 
 * registered with the control point. Otherwise, a fatal exception is thrown.
 *
 * @param runCollName The name of the run collection grouping the hits collections.
 * @param hitsCollName The name of the hits collection to add.
 *
 * @throws G4Exception if the specified hits collection has already been added.
 */
void VPatientSD::AddHitsCollection(const G4String&runCollName, const G4String& hitsCollName){
  if(! IsHitsCollectionExist(hitsCollName) ){
    G4VSensitiveDetector::collectionName.insert(hitsCollName);  // add to G4VSensitiveDetector container
    m_scoring_volumes.emplace_back(std::make_pair(hitsCollName,std::make_unique<ScoringVolume>()));
    m_scoring_volumes.back().second->m_run_collection = runCollName;
    // All hits collections are groupped by runCollName thus it has to be verified the new one
    // if it is compatible with the previously added ones!
    AcknowledgeHitsCollection(runCollName,m_scoring_volumes.back());
    ControlPoint::RegisterRunHCollection(runCollName,hitsCollName);
  }
  else {
    G4String msg =  "AddHitsCollection::The '"+hitsCollName+"' already added!";
    // LOGSVC_CRITICAL("{} Verify the specified hits collection name",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the specified hits collection name");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// 
void VPatientSD::AddScoringVolume(const G4String& runCollName, const G4String& hitsCollName, const G4Box& scoringBox, int scoringNX, int scoringNY, int scoringNZ, const G4ThreeVector& translation){
  AddHitsCollection(runCollName,hitsCollName);
  SetScoringParameterization(hitsCollName,scoringNX,scoringNY,scoringNZ);
  SetScoringVolume(hitsCollName,scoringBox,translation);
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")){
    auto isVoxelised = false;
    if(scoringNX > 1 || scoringNY > 1 ||scoringNZ > 1)
      isVoxelised = true;
    NTupleEventAnalisys::DefineTTree(runCollName,isVoxelised,hitsCollName,"Event data");
    NTupleEventAnalisys::SetTracksAnalysis(hitsCollName,m_tracks_analysis);
  }
};


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Checks compatibility of a new scoring volume with existing hits collections.
 *
 * This method verifies that the scoring volume to be added—specified by its name and associated unique
 * pointer—is compatible with any previously added scoring volumes in the same run collection. Compatibility
 * is determined using the ScoringVolume equality operator. If a scoring volume already exists in the run
 * collection and the new one is not compatible, a fatal exception is raised.
 *
 * Note: No compatibility check is performed if this is the first hits collection overall or the first one
 * for the specific run collection.
 *
 * @param runCollName The name of the run collection to which the scoring volume is being added.
 * @param scoring_volume A pair containing the scoring volume's identifier and its unique pointer.
 *
 * @throws G4Exception if the new scoring volume is incompatible with the existing ones.
 */
void VPatientSD::AcknowledgeHitsCollection(const G4String&runCollName,const std::pair<G4String,std::unique_ptr<ScoringVolume>>& scoring_volume){
  bool is_ok = false;
  G4String compatibility_reference_obj = "?";
  G4int n_sv_of_current_run_coll = m_scoring_volumes.size();
  if (IsHitsCollectionExist(scoring_volume.first) ){
    if(n_sv_of_current_run_coll < 2)
      return; // this is the very first HC added, no need to check compatibility
    const auto& current_sv = scoring_volume.second;
    // We have to count the number of SV of the current run collection
    // once the different run collections can be added
    n_sv_of_current_run_coll = 0;
    for(const auto& sv : m_scoring_volumes){
      if(sv.second->m_run_collection==current_sv->m_run_collection)
        ++n_sv_of_current_run_coll;
        if(n_sv_of_current_run_coll>2)
          break; // no need to continue
    }
    if(n_sv_of_current_run_coll<2)
          return; // this is the very first HC added of this run collection, no need to check compatibility

    // The actual comparing
    for(const auto& sv : m_scoring_volumes){
      const auto& existing_sv = sv.second;
      if(sv.first!=scoring_volume.first && existing_sv->m_run_collection==current_sv->m_run_collection){
        // The first matching instance is found, compare and break
        // - it has to ok, otherwise the new one is not compatible!
        is_ok = current_sv == existing_sv;
        compatibility_reference_obj = sv.first;
        break;
      }
    }
  }
  if(!is_ok){
    G4String msg =  "AcknowledgeHitsCollection::The '"+scoring_volume.first+"'";
    msg+=" being added to the run collection '"+runCollName+"' is not compatible with the previous added ones!";
    msg+=" Compatibility check performed by comparison to '"+compatibility_reference_obj+"'";
    // LOGSVC_CRITICAL("{} Verify the specified run collection name",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the specified run collection name");
  }
}

////////////////////////////////////////////////////////////////////////////////
///
bool VPatientSD::ScoringVolume::operator == (const ScoringVolume& other){
  bool is_voxelisation_eq =  m_nVoxelsX == other.m_nVoxelsX 
                          && m_nVoxelsY == other.m_nVoxelsY 
                          && m_nVoxelsZ == other.m_nVoxelsZ;
  bool is_volume_eq = GetVolume() == other.GetVolume();
  bool is_voxel_volume_eq = GetVoxelVolume() == other.GetVoxelVolume();
  bool is_run_collection_eq = m_run_collection == other.m_run_collection;
  
  return   is_voxelisation_eq
        && is_volume_eq
        && is_voxel_volume_eq
        && is_run_collection_eq;
}


////////////////////////////////////////////////////////////////////////////////
/// This method is being called at beginning of event
void VPatientSD::Initialize(G4HCofThisEvent* hCofThisEvent){

  InitializeChannelsID();
  auto sdManager = G4SDManager::GetSDMpointer();

  // Create new collection instances
  auto this_sd_name = GetName();
  for(const auto& i_sd : m_scoring_volumes){
    auto this_collection_name = i_sd.first;
    auto& this_collection_id = i_sd.second->id;
    i_sd.second->m_voxelHCollectionPtr = new VoxelHitsCollection(this_sd_name,this_collection_name);
    
    if(this_collection_id<0)
      this_collection_id = sdManager->GetCollectionID(this_collection_name); // Get ID from global storage

    hCofThisEvent->AddHitsCollection(this_collection_id,i_sd.second->m_voxelHCollectionPtr);

  }

}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4String> VPatientSD::GetScoringVolumeNames() const{
  std::vector<G4String> names;
  for(const auto& i_sd : m_scoring_volumes){
    names.emplace_back(i_sd.first);
  }
  return names;
}


////////////////////////////////////////////////////////////////////////////////
///
void VPatientSD::SetScoringParameterization(const G4String& hitsCollName, int nX, int nY, int nZ){
  auto scoring_sd = GetScoringVolumePtr(hitsCollName); // this verify if this name exists, otherwise FatalExeption
  scoring_sd->m_nVoxelsX = nX;
  scoring_sd->m_nVoxelsY = nY;
  scoring_sd->m_nVoxelsZ = nZ; 
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VPatientSD::GetScoringHcId(const G4String& hitsCollName) const{
  auto scoringSdIdx = GetScoringVolumeIdx(hitsCollName);
  return GetScoringHcId(scoringSdIdx);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the hits collection ID for a specified scoring volume index.
 *
 * This method returns the identifier of the scoring hits collection corresponding to the given index.
 * If the index is out of range, a fatal exception is raised.
 *
 * @param scoringSdIdx Index of the scoring volume.
 * @return G4int Identifier of the corresponding hits collection.
 */
G4int VPatientSD::GetScoringHcId(const G4int scoringSdIdx) const{
    G4int nElements = m_scoring_volumes.size();
  if (scoringSdIdx < nElements){
    return m_scoring_volumes.at(scoringSdIdx).second->id;
  } else{
    G4String msg = "GetScoringHcId::The idx "+std::to_string(scoringSdIdx)+" doesn't exist! (max size = "+std::to_string(nElements)+")";
    // LOGSVC_CRITICAL("{}. Verify given scoringSdIdx value!",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify given scoringSdIdx value");
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the index of the scoring volume for the specified hits collection.
 *
 * Iterates through the internal scoring volumes to locate one with a matching hits collection name.
 * If found, its zero-based index is returned. If the name is not present, a fatal exception is raised,
 * indicating that the scoring volume was not registered as expected.
 *
 * @param hitsCollName The name of the hits collection associated with the scoring volume.
 * @return G4int The zero-based index of the scoring volume.
 * @throws FatalException if no scoring volume matches the provided name.
 */
G4int VPatientSD::GetScoringVolumeIdx(const G4String& hitsCollName) const {
  G4int idx = -1;
  for(const auto& i_sd : m_scoring_volumes){
    if (i_sd.first == hitsCollName){
      ++idx; 
      break;
    } 
    ++idx;
  }
  if (idx < 0 ){
    G4String msg =  "GetScoringVolumeIdx::The '"+hitsCollName+"' doesn't exist!";
    // LOGSVC_CRITICAL("{} Verify the AddHitsCollection(...) calls!",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the AddHitsCollection(...) calls!");
  }
  return idx;
}

////////////////////////////////////////////////////////////////////////////////
///
bool VPatientSD::IsHitsCollectionExist(const G4String& hitsCollName) const {
  if (m_scoring_volumes.empty()) return false;
  for(const auto& i_sd : m_scoring_volumes){
    if (i_sd.first == hitsCollName) 
      return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves a pointer to the scoring volume at the specified index.
 *
 * Checks that the provided index is valid with respect to the internal collection of scoring volumes.
 * If the index is within bounds, it returns a pointer to the corresponding scoring volume.
 * Otherwise, a fatal exception is raised with an error message and the function returns a null pointer.
 *
 * @param scoringSdIdx Index of the scoring volume in the internal collection.
 * @return VPatientSD::ScoringVolume* Pointer to the scoring volume if the index is valid, or nullptr if not.
 */
VPatientSD::ScoringVolume* VPatientSD::GetScoringVolumePtr(G4int scoringSdIdx){
  G4int nElements = m_scoring_volumes.size();
  if (scoringSdIdx < nElements)
    return m_scoring_volumes.at(scoringSdIdx).second.get();
  else {
    G4String msg =  "GetScoringVolumePtr::The scoring SD for given index doesn't exist! (max size="+std::to_string(nElements)+")";
    // LOGSVC_CRITICAL("{}. Verify the AddHitsCollection(...) calls!",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the AddHitsCollection(...) calls!");
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the scoring volume pointer at the specified index.
 *
 * Obtains the pointer to the scoring volume corresponding to the given index from the internal container.
 * If the index is out of range, the function throws a fatal exception with a message indicating the maximum allowable index.
 *
 * @param scoringSdIdx Index of the scoring volume to retrieve.
 * @return Pointer to the scoring volume.
 *
 * @exception G4Exception Thrown if the provided index does not correspond to an existing scoring volume.
 */
VPatientSD::ScoringVolume* VPatientSD::GetScoringVolumePtr(G4int scoringSdIdx) const {
  G4int nElements = m_scoring_volumes.size();
  if (scoringSdIdx < nElements)
    return m_scoring_volumes.at(scoringSdIdx).second.get();
  else {
    G4String msg =  "GetScoringVolumePtr::The scoring SD for given index doesn't exist! (max size="+std::to_string(nElements)+")";
    // LOGSVC_CRITICAL("{}. Verify the AddHitsCollection(...) calls!", msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the AddHitsCollection(...) calls!");
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
VPatientSD::ScoringVolume* VPatientSD::GetScoringVolumePtr(const G4String& hitsCollName){
  auto scoringSdIdx = GetScoringVolumeIdx(hitsCollName); // this verify if this name exists, otherwise FatalExeption
  return GetScoringVolumePtr(scoringSdIdx);
}

////////////////////////////////////////////////////////////////////////////////
///
VPatientSD::ScoringVolume* VPatientSD::GetRunCollectionReferenceScoringVolume(const G4String& runCollName, bool voxelisation_check) const{
  for(const auto& i_sd : m_scoring_volumes){
    if (i_sd.second->m_run_collection == runCollName){
      if(voxelisation_check){
        if(i_sd.second->IsVoxelised())
          return i_sd.second.get();
      }
      else {
        return i_sd.second.get();
      }
    }
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
G4String VPatientSD::GetScoringHcName(G4int hitsCollId) const {
  if (hitsCollId < m_scoring_volumes.size())
    return m_scoring_volumes.at(hitsCollId).first;
  else
    return G4String();
}

////////////////////////////////////////////////////////////////////////////////
///
void VPatientSD::InitializeChannelsID(){
  for(const auto& i_sd : m_scoring_volumes){
    auto& channelHCollectionIndex = i_sd.second->m_channelHCollectionIndex;
    if(channelHCollectionIndex.empty()) {
      auto nvX  = i_sd.second->m_nVoxelsX;
      auto nvY  = i_sd.second->m_nVoxelsY;
      auto nvZ  = i_sd.second->m_nVoxelsZ;
      auto max_unique_index = static_cast<int>(i_sd.second->LinearizeIndex(nvX,nvY,nvZ));
      channelHCollectionIndex.reserve(max_unique_index);
      for (int i = 0; i < max_unique_index; ++i){
        channelHCollectionIndex.emplace_back(-1);
      }
  } else // reset of the vector (happens for each event initialization)
    std::fill(channelHCollectionIndex.begin(), channelHCollectionIndex.end(), -1);

  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VPatientSD::EndOfEvent(G4HCofThisEvent* hCofThisEvent){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the scoring shape for a specified hits collection.
 *
 * This function retrieves the scoring volume associated with the given hits collection name and assigns 
 * a new scoring shape to it. Only the shapes "Farmer30013" and "Farmer30013ScanZBox" are supported. 
 * If an unsupported shape is provided, a fatal exception is raised.
 *
 * @param hitsCollName Name of the hits collection whose scoring shape is to be set.
 * @param shapeName Desired scoring shape; must be either "Farmer30013" or "Farmer30013ScanZBox".
 *
 * @exception G4Exception Thrown if the provided shape name is not supported.
 */
void VPatientSD::SetScoringShape(const G4String& hitsCollName, const G4String& shapeName){
  auto scoringVolume = GetScoringVolumePtr(hitsCollName);
  if(shapeName!="Farmer30013" && shapeName!="Farmer30013ScanZBox" ){
    G4String msg = "SetScoringShape:: "+shapeName+" is not found. Available shape: Farmer30013";
    // LOGSVC_CRITICAL("{}. Verify your input!", msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify your input!");
  }
  scoringVolume->m_shape = shapeName;
}


////////////////////////////////////////////////////////////////////////////////
///
void VPatientSD::SetScoringVolume(const G4String& hitsCollName, const G4Box& envelopBox, const G4ThreeVector& translation){
  auto scoringSdIdx = GetScoringVolumeIdx(hitsCollName);
  SetScoringVolume(scoringSdIdx,envelopBox,translation);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Configures the scoring volume with envelope and voxelization parameters.
 *
 * This method sets the envelope for the scoring volume identified by its index by creating a copy of the provided
 * envelope box. It then computes the global coordinate ranges (minimum and maximum for x, y, and z) based on the
 * sensitive detector's global centre, the provided translation, and the scoring volume dimensions. Following this,
 * the method verifies that the scoring volume has been parameterized (i.e., non-zero voxel counts in each dimension)
 * and calculates the centre positions for each voxel. A fatal exception is thrown if the voxelization parameters have
 * not been defined.
 *
 * @param scoringSdIdx Index of the scoring volume within the collection.
 * @param envelopBox The envelope box defining the scoring volume boundaries. A copy of this box is stored internally.
 * @param translation Offset vector applied to the global centre to position the scoring volume.
 *
 * @throws G4Exception Thrown with a FatalException if the scoring volume has not been parameterized.
 */
void VPatientSD::SetScoringVolume(G4int scoringSdIdx, const G4Box& envelopBox, const G4ThreeVector& translation){
  auto sdHColPtr = GetScoringVolumePtr(scoringSdIdx);
  sdHColPtr->m_envelopeBoxPtr = std::make_unique<G4Box>(envelopBox); // make a copy of the envelope box
  //
  auto& minX = sdHColPtr->m_rangeMinX;
  auto& maxX = sdHColPtr->m_rangeMaxX;
  auto& minY = sdHColPtr->m_rangeMinY;
  auto& maxY = sdHColPtr->m_rangeMaxY;
  auto& minZ = sdHColPtr->m_rangeMinZ;
  auto& maxZ = sdHColPtr->m_rangeMaxZ;
  //
  minX = svc::round_with_prec((m_centre_in_global_coordinates.x() + translation.x() - sdHColPtr->GetSizeX() / 2.),8);
  maxX = svc::round_with_prec((m_centre_in_global_coordinates.x() + translation.x() + sdHColPtr->GetSizeX() / 2.),8);
  minY = svc::round_with_prec((m_centre_in_global_coordinates.y() + translation.y() - sdHColPtr->GetSizeY() / 2.),8);
  maxY = svc::round_with_prec((m_centre_in_global_coordinates.y() + translation.y() + sdHColPtr->GetSizeY() / 2.),8);
  minZ = svc::round_with_prec((m_centre_in_global_coordinates.z() + translation.z() - sdHColPtr->GetSizeZ() / 2.),8);
  maxZ = svc::round_with_prec((m_centre_in_global_coordinates.z() + translation.z() + sdHColPtr->GetSizeZ() / 2.),8);

  // LOGSVC_DEBUG("VPatientSD:: Defined Collection: {}", GetScoringHcName(scoringSdIdx));
  // LOGSVC_DEBUG("VPatientSD:: Voxelized SD range x {} - {}", sdHColPtr->m_rangeMinX, sdHColPtr->m_rangeMaxX);
  // LOGSVC_DEBUG("VPatientSD:: Voxelized SD range y {} - {}", sdHColPtr->m_rangeMinY, sdHColPtr->m_rangeMaxY);
  // LOGSVC_DEBUG("VPatientSD:: Voxelized SD range z {} - {}",sdHColPtr->m_rangeMinZ,sdHColPtr->m_rangeMaxZ);


  //G4cout << "[DEBUG]:: VPatientSD:: Defined Collection: " << hcName << G4endl;
  // G4cout << "[DEBUG]:: Voxelized SD range: X " << xMin << " - " << xMax << G4endl;
  // G4cout << "[DEBUG]:: Voxelized SD range: Y " << yMin << " - " << yMax << G4endl;
  // std::cout << "[DEBUG]:: Voxelized SD range: Z " << std::setprecision(16) << zMin*10000000000 <<  " - " << zMax*10000000000 << std::endl;
  // G4cout <<   << " - " << zMax*1000000  << G4endl;
  // G4cout << G4endl;

  // Fill the information about voxels positioning
  auto nvX = sdHColPtr->m_nVoxelsX;
  auto nvY = sdHColPtr->m_nVoxelsY;
  auto nvZ = sdHColPtr->m_nVoxelsZ;
  if(nvX==0 || nvY==0 || nvZ==0){
    G4String msg = "SetScoringVolume:: Parameterization is empty! You should call SetScoringParameterization(...) before!";
    // LOGSVC_CRITICAL("{}. Verify your logic...", msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify your logic...");
  }
  // Comppute and set voxel position
  auto& voxelsCentre = sdHColPtr->m_channelCentrePosition;
  auto max_uniqe_idx = sdHColPtr->LinearizeIndex(nvX,nvY,nvZ);
  voxelsCentre.reserve(max_uniqe_idx);
  for (int i = 0; i < static_cast<int>(max_uniqe_idx); ++i){
        voxelsCentre.emplace_back(G4ThreeVector(-999,-999,-999));
      }

  auto dx = static_cast<double>(maxX - minX)/nvX; //
  auto dy = static_cast<double>(maxY - minY)/nvY; //
  auto dz = static_cast<double>(maxZ - minZ)/nvZ; //

  for (int ix = 0; ix < nvX; ++ix){
    auto x = minX + dx/2. + ix*dx;
    for (int iy = 0; iy < nvY; ++iy){
      auto y = minY + dy/2. + iy*dy;
      for (int iz = 0; iz < nvZ; ++iz){
        auto z = minZ + dz/2. + iz*dz;
        auto current_idx = sdHColPtr->LinearizeIndex(ix,iy,iz);
        voxelsCentre.at(current_idx)= svc::round_with_prec(G4ThreeVector(x,y,z) - m_centre_in_global_coordinates,4); // go back to global origin if needed
      }
    }
  }

}

////////////////////////////////////////////////////////////////////////////////
///
G4int VPatientSD::ScoringVolume::LinearizeIndex(int idX, int idY, int idZ) const {
    return (idX * m_nVoxelsY + idY) * m_nVoxelsZ + idZ;
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector VPatientSD::ScoringVolume::GetVoxelCentre(int idX, int idY, int idZ) const {
    auto index = LinearizeIndex(idX,idY,idZ);
    return m_channelCentrePosition.at(index);
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector VPatientSD::ScoringVolume::GetVoxelCentre(int linearizedId) const {
    return m_channelCentrePosition.at(linearizedId);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Computes the voxel index along a specified axis based on a hit position.
 *
 * This function determines the voxel index along the axis indicated by `axisId` (0 for X, 1 for Y, 2 for Z)
 * by mapping the corresponding coordinate of the hit position into the configured voxel grid. The mapping
 * uses the axis-specific minimum and maximum bounds and the total number of voxels to compute an index via
 * proportional scaling. Values falling outside the expected range are clamped to the nearest valid index.
 * If the envelope box pointer is not set, the function returns 0.
 *
 * @param axisId The axis identifier (0 for X, 1 for Y, 2 for Z) for which the voxel index is computed.
 * @param hitPosition The global position of the hit.
 * @return G4int The calculated voxel index along the chosen axis.
 */
G4int VPatientSD::ScoringVolume::GetVoxelID(G4int axisId, const G4ThreeVector& hitPosition)  const {
  G4int voxelId = 0;

  if(!m_envelopeBoxPtr) {
    // LOGSVC_ERROR("Trying to extract channel ID but the envelope box is not being set!");
    return voxelId;
  }

  // Check for underflow and overflow is being performed below, however it shouldn't happen,
  // - we are inside sensitive volume boundaries. Nonetheless give warning in case...
  auto OutOfRangeWarning = [=](char axis, G4double val, G4double min, G4double max){
    // LOGSVC_WARN("Out of range hit {} | {}, value is {} => valid range: ({},{})", hitPosition,axis,val,min,max);
  };

  // Make sure that we does not critically rely on the result of a comparison of very close values
  auto IsEqual = [](G4double x, G4double y){
    return std::fabs(std::fabs(x) - std::fabs(y))  < 0.00000000001;
  };

  // Generic lambda to calculate id
  auto GetID = [=](char axis, G4double x, G4int nVoxels, G4double min, G4double max){
    G4int id = 0;
    if ( !IsEqual(x,min) && x < min ) {
      OutOfRangeWarning(axis,x,min,max);
    }
    else if( !IsEqual(x,max) && x > max ) {
      OutOfRangeWarning(axis,x,min,max);
      id = nVoxels-1;
    }
    else {
      id = int(std::floor(nVoxels * (x - min) / (max - min) ));
      if(x<=min){
      G4cout << "max " << max << G4endl;
      G4cout << "min " << min << G4endl<<G4endl;
      }
      if(id==nVoxels) --id; // for exact edge hit, it happens!
    }
    return id;
  };

  if( axisId==0 )  { // get X
    auto x = hitPosition.x();
    voxelId = GetID('X',x,m_nVoxelsX,m_rangeMinX,m_rangeMaxX);
  }

  if(axisId==1) { // get Y
    auto y = hitPosition.y();
    voxelId = GetID('Y',y,m_nVoxelsY,m_rangeMinY,m_rangeMaxY);
  }

  if(axisId==2) { // get Z
    auto z = hitPosition.z();
    voxelId = GetID('Z',z,m_nVoxelsZ,m_rangeMinZ,m_rangeMaxZ);
  }
  return voxelId;
}

////////////////////////////////////////////////////////////////////////////////
/// 
G4int VPatientSD::ScoringVolume::GetLinearizedVoxelID(const G4ThreeVector& hitPosition)  const {
  auto idX = GetVoxelID(0, hitPosition);
  auto idY = GetVoxelID(1, hitPosition);
  auto idZ = GetVoxelID(2, hitPosition);
  return LinearizeIndex(idX,idY,idZ);
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool VPatientSD::ScoringVolume::IsInside(const G4ThreeVector& position) const {
  if(m_shape=="Box" || m_shape=="Farmer30013ScanZBox"){
    auto pos = svc::round_with_prec(position,8);
    G4bool inX = ( pos.x() > m_rangeMinX && pos.x() < m_rangeMaxX );
    G4bool inY = ( pos.y() > m_rangeMinY && pos.y() < m_rangeMaxY );
    G4bool inZ = ( pos.z() > m_rangeMinZ && pos.z() < m_rangeMaxZ );
    return inX && inY && inZ;
  }
  else if (m_shape=="Farmer30013"){
    return IsInsideFarmer30013(position);
  }
  return false;
}

G4bool VPatientSD::ScoringVolume::IsOnBorder(const G4ThreeVector& position) const {
    auto IsEqual = [](G4double x, G4double y){
      return std::fabs(std::fabs(x) - std::fabs(y))  < 0.00000000001;
    };
    auto pos = svc::round_with_prec(position,8);
    G4bool inBorder = ( IsEqual(pos.x(), m_rangeMaxX) || IsEqual(pos.x(), m_rangeMinX) ||  IsEqual(pos.y(), m_rangeMaxY) || IsEqual(pos.y(), m_rangeMinY) ||  IsEqual(pos.z(), m_rangeMaxZ) || IsEqual(pos.z(), m_rangeMinZ) );
    return inBorder;
  }

G4double VPatientSD::ScoringVolume::GetVoxelVolume() const{
  if(m_shape=="Box"){
    return (GetSizeX()/m_nVoxelsX)*(GetSizeY()/m_nVoxelsY)*(GetSizeZ()/m_nVoxelsZ);
  }
  else if (m_shape=="Farmer30013"){
    G4double farmerR = 3.05;
    return GetSizeX()*3.14159*farmerR*farmerR;
  }
  return 0.;
}


////////////////////////////////////////////////////////////////////////////////
///
G4bool VPatientSD::ScoringVolume::IsInsideFarmer30013(const G4ThreeVector& position) const {
  G4bool inX = ( position.x() >= m_rangeMinX) && (position.x() < m_rangeMaxX);
  G4double farmerR = 3.05;
  G4double relativeY = (position.y() - (m_rangeMinY + m_rangeMaxY)/2.);
  G4double relativeZ = (position.z() - (m_rangeMinZ + m_rangeMaxZ)/2.);
  G4double possitionR = std::sqrt((relativeY*relativeY)+(relativeZ*relativeZ));
  G4bool inR = (possitionR <= farmerR);
  return inX && inR;
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector VPatientSD::GetSDCentre() const {
  return m_centre_in_global_coordinates;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Processes a simulation step to record or update a voxel hit in the designated hits collection.
 *
 * This method determines whether the step's pre-step point falls inside the scoring volume associated with the
 * specified hits collection name. If the hit is outside the volume or on its border with zero energy deposition,
 * the step is ignored. Otherwise, the function computes the voxel indices from the hit position, linearizes these
 * indices to a unique identifier, and either creates a new voxel hit or updates an existing one in the collection.
 *
 * @param hitsCollectionName Name identifier of the hits collection (and its associated scoring volume) to update.
 * @param aStep Pointer to the simulation step containing hit and energy deposition information.
 */
void VPatientSD::ProcessHitsCollection(const G4String& hitsCollectionName, G4Step* aStep){
    auto position = aStep->GetPreStepPoint()->GetPosition(); // in world volume frame;
    auto scoringVolumePtr = GetScoringVolumePtr(hitsCollectionName);
    auto inScoringVolume = scoringVolumePtr->IsInside(position);
    auto isOnBorder = scoringVolumePtr->IsOnBorder(position);

    if (!inScoringVolume || (isOnBorder && ((aStep->GetTotalEnergyDeposit())==0.))) return; // Nothing to do for this HC

    // IMPORTANT: this feature reduce photons hits at small angle scattering!!!
    //if (edep == 0.) return false;
    // ________________________________________________________________
    // Accumulate data from all steps in this event per volume/local channel.
    auto voxelIdX = scoringVolumePtr->GetVoxelID(0, position);
    auto voxelIdY = scoringVolumePtr->GetVoxelID(1, position);
    auto voxelIdZ = scoringVolumePtr->GetVoxelID(2, position);
    auto voxelId =  scoringVolumePtr->LinearizeIndex(voxelIdX,voxelIdY,voxelIdZ);

    if(voxelId>=0 && voxelId < scoringVolumePtr->m_channelHCollectionIndex.size() ) {
      if (scoringVolumePtr->m_channelHCollectionIndex[voxelId] == -1) { // This is new hit within given volume/channel
        
        auto voxelHit = new VoxelHit();
        voxelHit->SetTracksAnalysis(m_tracks_analysis);
        voxelHit->SetVolume(scoringVolumePtr->GetVoxelVolume());
        voxelHit->SetCentre(scoringVolumePtr->GetVoxelCentre(voxelId));
        voxelHit->SetId(voxelIdX, voxelIdY, voxelIdZ);
        voxelHit->SetGlobalId(m_id_x, m_id_y, m_id_z);
        voxelHit->SetGlobalCentre(GetSDCentre());
        voxelHit->Fill(aStep);
        voxelHit->FillTrackUserInfo<PatientTrackInfo>(aStep);
        voxelHit->FillPrimaryParticleUserInfo<PrimaryParticleInfo>();
        //voxelHit->PrintEvtInfo();

        // Add new voxel hit to the hits collection
        auto channelHCollectionIndex = scoringVolumePtr->m_voxelHCollectionPtr->insert(voxelHit) - 1;
        scoringVolumePtr->m_channelHCollectionIndex[voxelId] = channelHCollectionIndex;
      }
      else { // This is already existing hit within given volume/channel
        // Get voxel hit for given volume/channel
        auto channelHCollectionIndex = scoringVolumePtr->m_channelHCollectionIndex[voxelId];
        auto voxelHit = (*scoringVolumePtr->m_voxelHCollectionPtr)[channelHCollectionIndex];
        voxelHit->Update(aStep);
        voxelHit->FillTrackUserInfo<PatientTrackInfo>(aStep);

      }
    } else {
      auto maxId = scoringVolumePtr->m_channelHCollectionIndex.size()-1;
      // LOGSVC_DEBUG("Out of scope ChannelId: {}. Max voxel ID is: {}.\nPosition: {}\nIdX={}, IdY={}, IdZ={}",voxelId,maxId,position,voxelIdX,voxelIdY,voxelIdZ);
    }
}
////////////////////////////////////////////////////////////////////////////////
///
bool VPatientSD::ScoringVolume::IsVoxelised() const{
  // return true;
  // G4cout << "m_nVoxelsX " << m_nVoxelsX << "   m_nVoxelsY " << m_nVoxelsY << "   m_nVoxelsZ " << m_nVoxelsZ << G4endl;
  return m_nVoxelsX > 1 || m_nVoxelsY > 1 || m_nVoxelsZ > 1;
}
