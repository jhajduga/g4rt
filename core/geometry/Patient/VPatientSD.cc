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
 * @brief Constructs a VPatientSD sensitive detector with the specified name.
 *
 * @param sdName Name of the sensitive detector.
 */
VPatientSD::VPatientSD(const G4String& sdName):G4VSensitiveDetector(sdName){}

////////////////////////////////////////////////////////////////////////////////
/**
   * @brief Constructs a VPatientSD sensitive detector with a specified name and center position.
   *
   * @param sdName Name of the sensitive detector.
   * @param centre Center position of the sensitive detector in world coordinates.
   */
VPatientSD::VPatientSD(const G4String& sdName, const G4ThreeVector& centre)
  :G4VSensitiveDetector(sdName),m_sd_centre(centre){}

////////////////////////////////////////////////////////////////////////////////
/// Note that a SD can declare more than one hits collection being groupped by runCollName!
/// Bartek's note: Typically the adding HC call is being placed inside SD constructor,
/// within this application framework I've exposed this to the level of UserDecectorClass::DefineSensitiveDetector()
/**
 * @brief Register a new hits collection under a run-collection group.
 *
 * Adds a hits collection named @p hitsCollName to this sensitive detector and associates it with
 * the run collection identified by @p runCollName. The new collection is appended to the internal
 * scoring-volume list and validated for compatibility with other collections already registered
 * under the same run collection.
 *
 * @param runCollName Name of the run-level grouping for hits collections.
 * @param hitsCollName Unique name of the hits collection to register.
 *
 * @throws G4Exception if a hits collection with @p hitsCollName already exists (fatal).
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
    FATAL_GEO("{} Verify the specified hits collection name",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the specified hits collection name");
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Add and configure a voxelized scoring volume and its hits collection.
 *
 * Creates and registers a hits collection for the given run collection and hits-collection names,
 * sets the voxelization (NX, NY, NZ) and geometric envelope (box + translation), and—when NTuple
 * analysis is enabled—defines an NTuple TTree for event output. A scoring volume is treated as
 * "voxelised" for NTuple purposes when any of the voxel counts (NX, NY, NZ) is greater than 1.
 *
 * @param runCollName Run-collection identifier used to group related hits collections.
 * @param hitsCollName Name of the hits collection to create and register.
 * @param scoringBox Envelope box that defines the scoring volume in local coordinates.
 * @param scoringNX Number of voxels along the X axis (must be set prior to geometry setup).
 * @param scoringNY Number of voxels along the Y axis.
 * @param scoringNZ Number of voxels along the Z axis.
 * @param translation Translation applied to the envelope box to position the scoring volume in world coordinates.
 */
void VPatientSD::AddScoringVolume(const G4String& runCollName, const G4String& hitsCollName, const G4Box& scoringBox, int scoringNX, int scoringNY, int scoringNZ, const G4ThreeVector& translation){
  AddHitsCollection(runCollName,hitsCollName);
  SetScoringParameterization(hitsCollName,scoringNX,scoringNY,scoringNZ);
  SetScoringVolume(hitsCollName,scoringBox,translation);
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")){
    auto isVoxelised = false;
    if(scoringNX > 1 || scoringNY > 1 ||scoringNZ > 1)
      isVoxelised = true;
    NTupleEventAnalisys::DefineTTree(runCollName,isVoxelised,hitsCollName,"Event data");
  }
};


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Verify that a newly added hits collection's scoring volume is compatible with existing ones in the same run collection.
 *
 * Compares the provided scoring volume against the first previously registered scoring volume that belongs to the same run collection.
 * If no other scoring volume for that run collection exists yet, the function returns without action.
 * If a mismatch is detected between voxelization/geometry parameters of the new and the reference scoring volume, this routine logs a fatal error and raises a G4Exception with FatalException severity.
 *
 * @param runCollName Name of the run collection to which the hits collection is being added.
 * @param scoring_volume Pair whose first element is the hits collection name and whose second element is the scoring volume pointer to be checked.
 *
 * @throws G4Exception Thrown with FatalException if the new scoring volume is incompatible with an existing scoring volume in the same run collection.
 */
void VPatientSD::AcknowledgeHitsCollection(const G4String& runCollName, const std::pair<G4String, std::unique_ptr<ScoringVolume>>& scoring_volume) {
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
    FATAL_GEO("{} Verify the specified run collection name",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the specified run collection name");
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Compares two ScoringVolume objects for equivalence.
 *
 * Determines if two scoring volumes are equivalent by comparing their voxelization parameters, geometric volume, voxel volume, and associated run collection name.
 *
 * @param other The ScoringVolume to compare with.
 * @return true if the scoring volumes are equivalent; false otherwise.
 */
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
/**
 * @brief Initialize per-event voxel hits collections for all configured scoring volumes.
 *
 * Ensures per-voxel channel indices are prepared, creates a VoxelHitsCollection for each
 * scoring volume, obtains or assigns its hits-collection ID from the G4SDManager if unset,
 * and registers each collection with the provided G4HCofThisEvent.
 *
 * @param hCofThisEvent Event-level hits collection container to which the newly created voxel hit collections are added.
 */
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
/**
 * @brief Returns the names of all scoring volumes managed by this sensitive detector.
 *
 * @return Vector of hits collection names corresponding to each scoring volume.
 */
std::vector<G4String> VPatientSD::GetScoringVolumeNames() const{
  std::vector<G4String> names;
  for(const auto& i_sd : m_scoring_volumes){
    names.emplace_back(i_sd.first);
  }
  return names;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Set the voxel grid size for a scoring volume.
 *
 * Update the number of voxels along each axis for the scoring volume identified by hitsCollName.
 * If the named scoring volume does not exist this will trigger a fatal error and throw a G4Exception.
 *
 * @param hitsCollName Hits collection name identifying the scoring volume.
 * @param nX Number of voxels along the X axis.
 * @param nY Number of voxels along the Y axis.
 * @param nZ Number of voxels along the Z axis.
 */
void VPatientSD::SetScoringParameterization(const G4String& hitsCollName, int nX, int nY, int nZ){
  auto scoring_sd = GetScoringVolumePtr(hitsCollName); // this verify if this name exists, otherwise FatalExeption
  scoring_sd->m_nVoxelsX = nX;
  scoring_sd->m_nVoxelsY = nY;
  scoring_sd->m_nVoxelsZ = nZ;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the hits collection ID for a scoring hits collection by name.
 *
 * Looks up the scoring-volume index for the given hits-collection name and returns its associated
 * hits collection ID.
 *
 * @param hitsCollName Hits collection name to query.
 * @return G4int The hits collection ID.
 *
 * @note If no scoring volume with the given name exists, this delegates to GetScoringVolumeIdx which
 * will raise a fatal error / throw.
 */
G4int VPatientSD::GetScoringHcId(const G4String& hitsCollName) const{
  auto scoringSdIdx = GetScoringVolumeIdx(hitsCollName);
  return GetScoringHcId(scoringSdIdx);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Return the hits-collection ID for the scoring volume at the given index.
 *
 * The index is zero-based and must be less than the number of configured scoring volumes.
 *
 * @param scoringSdIdx Zero-based index of the scoring volume.
 * @return G4int The hits collection ID associated with that scoring volume.
 *
 * @throws FatalException if scoringSdIdx is out of range (logs and raises a G4Exception).
 */
G4int VPatientSD::GetScoringHcId(const G4int scoringSdIdx) const{
  G4int nElements = m_scoring_volumes.size();
  if (scoringSdIdx < nElements){
    return m_scoring_volumes.at(scoringSdIdx).second->id;
  } else{
    G4String msg = "GetScoringHcId::The idx "+std::to_string(scoringSdIdx)+" doesn't exist! (max size = "+std::to_string(nElements)+")";
    FATAL_GEO("{}. Verify given scoringSdIdx value!",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify given scoringSdIdx value");
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the 0-based index of the scoring volume for a given hits collection name.
 *
 * Searches the internal scoring-volume list for a scoring volume whose hits collection
 * name matches hitsCollName and returns its 0-based index.
 *
 * @param hitsCollName Hits collection name to look up.
 * @return G4int 0-based index of the matching scoring volume.
 *
 * @throws FatalException If no scoring volume with the given hits collection name exists
 *                        (the function calls G4Exception with FatalException severity).
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
    FATAL_GEO("{} Verify the AddHitsCollection(...) calls!",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the AddHitsCollection(...) calls!");
  }
  return idx;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Checks if a hits collection with the specified name exists.
 *
 * @param hitsCollName Name of the hits collection to check.
 * @return true if the hits collection exists; false otherwise.
 */
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
 * @brief Return the scoring volume at the given 0-based index.
 *
 * Returns a raw pointer to the ScoringVolume stored for the specified index.
 * If the index is out of range a fatal G4Exception is raised (execution does not continue).
 *
 * @param scoringSdIdx 0-based index of the scoring volume.
 * @return ScoringVolume* Pointer to the scoring volume.
 * @throws G4Exception Thrown with FatalException severity when scoringSdIdx is invalid.
 */
VPatientSD::ScoringVolume* VPatientSD::GetScoringVolumePtr(G4int scoringSdIdx){
  G4int nElements = m_scoring_volumes.size();
  if (scoringSdIdx < nElements)
    return m_scoring_volumes.at(scoringSdIdx).second.get();
  else {
    G4String msg =  "GetScoringVolumePtr::The scoring SD for given index doesn't exist! (max size="+std::to_string(nElements)+")";
    FATAL_GEO("{}. Verify the AddHitsCollection(...) calls!",msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the AddHitsCollection(...) calls!");
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Return the scoring volume pointer for a given scoring-volume index.
 *
 * Retrieves the ScoringVolume associated with scoringSdIdx in the internal
 * scoring-volume list.
 *
 * @param scoringSdIdx Index of the scoring volume to retrieve.
 * @return ScoringVolume* Pointer to the matching ScoringVolume on success.
 *
 * @throws G4Exception with severity FatalException if the provided index does
 *         not correspond to an existing scoring volume (an error is also
 *         reported via FATAL_GEO before throwing).
 */
VPatientSD::ScoringVolume* VPatientSD::GetScoringVolumePtr(G4int scoringSdIdx) const {
  G4int nElements = m_scoring_volumes.size();
  if (scoringSdIdx < nElements)
    return m_scoring_volumes.at(scoringSdIdx).second.get();
  else {
    G4String msg =  "GetScoringVolumePtr::The scoring SD for given index doesn't exist! (max size="+std::to_string(nElements)+")";
    FATAL_GEO("{}. Verify the AddHitsCollection(...) calls!", msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify the AddHitsCollection(...) calls!");
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the scoring volume pointer for a hits collection name.
 *
 * Returns the ScoringVolume associated with the given hits collection name.
 *
 * @param hitsCollName Hits collection name to look up.
 * @return ScoringVolume* Pointer to the corresponding ScoringVolume (never null).
 *
 * @throws G4Exception If no scoring volume exists with the provided name (fatal).
 */
VPatientSD::ScoringVolume* VPatientSD::GetScoringVolumePtr(const G4String& hitsCollName){
  auto scoringSdIdx = GetScoringVolumeIdx(hitsCollName); // this verify if this name exists, otherwise FatalExeption
  return GetScoringVolumePtr(scoringSdIdx);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a pointer to a scoring volume associated with the specified run collection name.
 *
 * If `voxelisation_check` is true, only returns a scoring volume if it is voxelized; otherwise, returns any matching scoring volume. Returns nullptr if no suitable scoring volume is found.
 *
 * @param runCollName Name of the run collection to search for.
 * @param voxelisation_check If true, only consider voxelized scoring volumes.
 * @return Pointer to the matching ScoringVolume, or nullptr if not found.
 */
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
/**
 * @brief Returns the hits collection name corresponding to the given collection ID.
 *
 * @param hitsCollId Index of the hits collection.
 * @return The name of the hits collection if the ID is valid; otherwise, an empty string.
 */
G4String VPatientSD::GetScoringHcName(G4int hitsCollId) const {
  if (hitsCollId < m_scoring_volumes.size())
    return m_scoring_volumes.at(hitsCollId).first;
  else
    return G4String();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize or reset per-voxel channel indices for all scoring volumes.
 *
 * Ensures each scoring volume's m_channelHCollectionIndex is sized to the total
 * number of voxels and filled with -1. If the vector is empty it is allocated
 * to the required size (derived from the scoring volume's voxel counts);
 * otherwise its existing entries are reset to -1. This prepares per-event
 * storage that maps voxels to hits (a value of -1 indicates no hit recorded).
 */
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
/**
 * @brief Placeholder for end-of-event processing in the sensitive detector.
 *
 * This method is intentionally left empty and performs no actions at the end of each event.
 */
void VPatientSD::EndOfEvent(G4HCofThisEvent* hCofThisEvent){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Assigns a predefined geometric shape to the scoring volume identified by a hits collection name.
 *
 * Only the shapes "Farmer30013" and "Farmer30013ScanZBox" are accepted. If an unsupported shapeName is supplied,
 * the function raises a fatal geometry error and throws a G4Exception.
 *
 * @param hitsCollName Hits collection name identifying the scoring volume to modify.
 * @param shapeName Desired shape; must be either "Farmer30013" or "Farmer30013ScanZBox".
 */
void VPatientSD::SetScoringShape(const G4String& hitsCollName, const G4String& shapeName){
  auto scoringVolume = GetScoringVolumePtr(hitsCollName);
  if(shapeName!="Farmer30013" && shapeName!="Farmer30013ScanZBox" ){
    G4String msg = "SetScoringShape:: "+shapeName+" is not found. Available shape: Farmer30013";
    FATAL_GEO("{}. Verify your input!", msg);
    G4Exception("VPatientSD", msg, FatalException,"Verify your input!");
  }
  scoringVolume->m_shape = shapeName;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Set the envelope box and translation for the scoring volume identified by a hits-collection name.
 *
 * Resolves the scoring-volume index for the given hits collection and delegates to the index-based
 * overload. If the hits-collection name is not registered this call will trigger a fatal error
 * (via GetScoringVolumeIdx).
 *
 * @param hitsCollName Hits-collection name identifying the scoring volume to update.
 * @param envelopBox Local envelope box that defines the scoring volume extents.
 * @param translation World-space translation to apply to the envelope box.
 */
void VPatientSD::SetScoringVolume(const G4String& hitsCollName, const G4Box& envelopBox, const G4ThreeVector& translation){
  auto scoringSdIdx = GetScoringVolumeIdx(hitsCollName);
  SetScoringVolume(scoringSdIdx,envelopBox,translation);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Configure the spatial envelope and precompute voxel center positions for a scoring volume.
 *
 * Copies the provided envelope box into the scoring volume at index `scoringSdIdx`, computes world-space
 * min/max bounds from the detector centre plus `translation`, validates that voxelization parameters are set,
 * and precomputes the centre position for every voxel (stored in the scoring volume's channel centre array).
 *
 * The voxel centres are computed as the regular grid cell centres within the computed bounds and rounded
 * to a fixed precision. The envelope box is copied (owned by the scoring volume).
 *
 * @param scoringSdIdx Index of the scoring volume to configure.
 * @param envelopBox Geant4 box describing the envelope extents (a copy is stored).
 * @param translation Translation applied to the detector centre to position the envelope in world coordinates.
 *
 * @throws G4Exception FatalException if voxelization parameters (number of voxels in any axis) are zero
 *                        — callers must call SetScoringParameterization(...) before this method.
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
  minX = svc::round_with_prec((m_sd_centre.x() + translation.x() - sdHColPtr->GetSizeX() / 2.),8);
  maxX = svc::round_with_prec((m_sd_centre.x() + translation.x() + sdHColPtr->GetSizeX() / 2.),8);
  minY = svc::round_with_prec((m_sd_centre.y() + translation.y() - sdHColPtr->GetSizeY() / 2.),8);
  maxY = svc::round_with_prec((m_sd_centre.y() + translation.y() + sdHColPtr->GetSizeY() / 2.),8);
  minZ = svc::round_with_prec((m_sd_centre.z() + translation.z() - sdHColPtr->GetSizeZ() / 2.),8);
  maxZ = svc::round_with_prec((m_sd_centre.z() + translation.z() + sdHColPtr->GetSizeZ() / 2.),8);

  DEBUG_GEO("VPatientSD:: Defined Collection: {}", GetScoringHcName(scoringSdIdx));
  DEBUG_GEO("VPatientSD:: Voxelized SD range x {} - {}", sdHColPtr->m_rangeMinX, sdHColPtr->m_rangeMaxX);
  DEBUG_GEO("VPatientSD:: Voxelized SD range y {} - {}", sdHColPtr->m_rangeMinY, sdHColPtr->m_rangeMaxY);
  DEBUG_GEO("VPatientSD:: Voxelized SD range z {} - {}",sdHColPtr->m_rangeMinZ,sdHColPtr->m_rangeMaxZ);

  // Fill the information about voxels positioning
  auto nvX = sdHColPtr->m_nVoxelsX;
  auto nvY = sdHColPtr->m_nVoxelsY;
  auto nvZ = sdHColPtr->m_nVoxelsZ;
  if(nvX==0 || nvY==0 || nvZ==0){
    G4String msg = "SetScoringVolume:: Parameterization is empty! You should call SetScoringParameterization(...) before!";
    FATAL_GEO("{}. Verify your logic...", msg);
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
        voxelsCentre.at(current_idx)= svc::round_with_prec(G4ThreeVector(x,y,z),4);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Converts 3D voxel indices to a linear index.
 *
 * Maps the voxel indices (idX, idY, idZ) to a single linear index based on the scoring volume's voxelization parameters.
 *
 * @param idX Voxel index along the X axis.
 * @param idY Voxel index along the Y axis.
 * @param idZ Voxel index along the Z axis.
 * @return G4int Linearized voxel index corresponding to the 3D indices.
 */
G4int VPatientSD::ScoringVolume::LinearizeIndex(int idX, int idY, int idZ) const {
    return (idX * m_nVoxelsY + idY) * m_nVoxelsZ + idZ;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the world-space center of a voxel identified by its 3D indices.
 *
 * Returns the precomputed center position for the voxel at (idX, idY, idZ).
 *
 * @param idX Voxel index along the X axis (0..nX-1).
 * @param idY Voxel index along the Y axis (0..nY-1).
 * @param idZ Voxel index along the Z axis (0..nZ-1).
 * @return G4ThreeVector Center position of the specified voxel.
 *
 * @throws std::out_of_range If the provided indices are outside the valid voxel range.
 */
G4ThreeVector VPatientSD::ScoringVolume::GetVoxelCentre(int idX, int idY, int idZ) const {
    auto index = LinearizeIndex(idX,idY,idZ);
  return m_channelCentrePosition.at(index);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Return the world-space center of a voxel by its linear index.
 *
 * Returns the precomputed center position for the voxel identified by
 * linearizedId (0..Nvoxels-1).
 *
 * @param linearizedId Linear index of the voxel.
 * @return G4ThreeVector Center position of the specified voxel.
 * @throws std::out_of_range if linearizedId is out of range.
 */
G4ThreeVector VPatientSD::ScoringVolume::GetVoxelCentre(int linearizedId) const {
    return m_channelCentrePosition.at(linearizedId);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Computes the voxel index along a specified axis for a given position.
 *
 * Determines the voxel ID (index) along the X, Y, or Z axis for the provided hit position within the scoring volume. Handles boundary conditions and issues a warning if the position is outside the expected range.
 *
 * @param axisId Axis index: 0 for X, 1 for Y, 2 for Z.
 * @param hitPosition The position to evaluate.
 * @return G4int The voxel index along the specified axis.
 */
G4int VPatientSD::ScoringVolume::GetVoxelID(G4int axisId, const G4ThreeVector& hitPosition)  const {
  G4int voxelId = 0;

  if(!m_envelopeBoxPtr) {
    //   LOGSVC_ERROR("Trying to extract channel ID but the envelope box is not being set!");
    return voxelId;
  }

  // Check for underflow and overflow is being performed below, however it shouldn't happen,
  // - we are inside sensitive volume boundaries. Nonetheless give warning in case...
  auto OutOfRangeWarning = [=](char axis, G4double val, G4double min, G4double max){
    // WARN_GEO("Out of range hit {} | {}, value is {} => valid range: ({},{})", hitPosition,axis,val,min,max);
    G4cout << "Out of range hit "<<hitPosition<< " | "<<axis<<", value is "<<val<<" => valid range: ("<<min<<","<<max<<")"<< G4endl;
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
/**
 * @brief Compute the linearized (1D) voxel index for a world position.
 *
 * Determines the voxel indices along X, Y and Z for the given position and maps them to the single linear index
 * used to address per-voxel arrays.
 *
 * @param hitPosition World-space position to locate inside the scoring volume.
 * @return G4int Linearized voxel index, or -1 if the position falls outside the voxelization (any axis index is invalid).
 */
G4int VPatientSD::ScoringVolume::GetLinearizedVoxelID(const G4ThreeVector& hitPosition)  const {
  auto idX = GetVoxelID(0, hitPosition);
  auto idY = GetVoxelID(1, hitPosition);
  auto idZ = GetVoxelID(2, hitPosition);
  return LinearizeIndex(idX,idY,idZ);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Determines whether a given position is inside the scoring volume.
 *
 * Checks if the specified position lies within the geometric boundaries of the scoring volume, considering its shape ("Box", "Farmer30013ScanZBox", or "Farmer30013").
 *
 * @param position The position to test for inclusion.
 * @return G4bool True if the position is inside the scoring volume; false otherwise.
 */
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

/**
 * @brief Check whether a point lies on any boundary plane of the scoring volume.
 *
 * The function rounds the input position to 8 decimal places and then compares each
 * coordinate (X, Y, Z) against the stored min/max range for that axis. Comparison
 * uses an absolute-magnitude tolerance of 1e-11 (i.e. two values are considered
 * equal when | |a| - |b| | < 1e-11). Returns true if the point lies on any of the
 * six bounding planes (min or max for X, Y or Z).
 *
 * @param position World-space position to test (rounded internally to 8 decimal places).
 * @return true if the position is on any border plane of the scoring volume; false otherwise.
 */
G4bool VPatientSD::ScoringVolume::IsOnBorder(const G4ThreeVector& position) const {
    auto IsEqual = [](G4double x, G4double y){
      return std::fabs(std::fabs(x) - std::fabs(y))  < 0.00000000001;
    };
    auto pos = svc::round_with_prec(position,8);
    G4bool inBorder = ( IsEqual(pos.x(), m_rangeMaxX) || IsEqual(pos.x(), m_rangeMinX) ||  IsEqual(pos.y(), m_rangeMaxY) || IsEqual(pos.y(), m_rangeMinY) ||  IsEqual(pos.z(), m_rangeMaxZ) || IsEqual(pos.z(), m_rangeMinZ) );
  return inBorder;
}

/**
 * @brief Compute the volume of a single voxel in this scoring volume.
 *
 * For shape "Box", returns the product of voxel extents along X, Y and Z:
 * (sizeX / nVoxelsX) * (sizeY / nVoxelsY) * (sizeZ / nVoxelsZ).
 *
 * For shape "Farmer30013", returns the X extent of the scoring volume multiplied
 * by the cross-sectional area of a cylinder of radius 3.05 (pi * r^2).
 *
 * @return The voxel volume for the current shape, or 0.0 if the shape is unrecognized.
 */
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
/**
 * @brief Determines if a position is inside the cylindrical "Farmer30013" scoring volume.
 *
 * Checks whether the given position lies within the defined X range and within a cylinder of radius 3.05 mm centered in the YZ plane of the scoring volume.
 *
 * @param position The position to test.
 * @return True if the position is inside the "Farmer30013" volume; false otherwise.
 */
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
/**
 * @brief Get the sensitive detector center in world coordinates.
 *
 * @return G4ThreeVector Centre position of the sensitive detector.
 */
G4ThreeVector VPatientSD::GetSDCentre() const {
  return m_sd_centre;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Process a single Geant4 step for the specified hits collection, creating or updating voxel hits.
 *
 * Checks the step's pre-step position against the scoring volume for hitsCollectionName using the pre-step point.
 * If the position is outside the scoring volume, or lies on the scoring-volume border and the step deposits no energy,
 * the step is ignored. Otherwise the position is mapped to a voxel (X,Y,Z) and a linear voxel index. For a voxel
 * with no existing hit a new VoxelHit is created, initialized (volume, centre, IDs, global centre, track/primary info)
 * and inserted into the volume's hits collection; for an existing voxel the corresponding VoxelHit is updated and its
 * per-track user information is appended.
 *
 * Side effects:
 * - May allocate and insert a new VoxelHit into the scoring volume's voxel hits collection.
 * - Updates the scoring volume's m_channelHCollectionIndex to point from voxel linear index -> collection index.
 *
 * @param hitsCollectionName Name of the hits collection / scoring volume to which the step should be mapped.
 * @param aStep The Geant4 step to process; the function uses the pre-step point position and the step's energy deposit.
 */
void VPatientSD::ProcessHitsCollection(const G4String& hitsCollectionName, G4Step* aStep){
    auto position = aStep->GetPreStepPoint()->GetPosition(); // in world volume frame;
  auto scoringVolumePtr = GetScoringVolumePtr(hitsCollectionName);
  auto inScoringVolume = scoringVolumePtr->IsInside(position);
  auto isOnBorder = scoringVolumePtr->IsOnBorder(position);

    if (!inScoringVolume || (isOnBorder && ((aStep->GetTotalEnergyDeposit())==0.))){
      if (!isOnBorder && aStep->GetTotalEnergyDeposit()>0 ){
        G4cout << "hit: " << position << " cell(" << m_id_x<<","<<m_id_y<<","<<m_id_z
                << ") ScoringBox x(" << scoringVolumePtr->m_rangeMinX << " - " << scoringVolumePtr->m_rangeMaxX
                              << ") y(" << scoringVolumePtr->m_rangeMinY << " - " << scoringVolumePtr->m_rangeMaxY
                              << ") z(" << scoringVolumePtr->m_rangeMinZ << " - " << scoringVolumePtr->m_rangeMaxZ 
                              << ")" <<  G4endl;
    }
      return; // Nothing to do for this HC
  }

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
      voxelHit->SetVolume(scoringVolumePtr->GetVoxelVolume());
      voxelHit->SetCentre(scoringVolumePtr->GetVoxelCentre(voxelId));
      voxelHit->SetId(voxelIdX, voxelIdY, voxelIdZ);
      voxelHit->SetGlobalId(m_id_x, m_id_y, m_id_z);
      voxelHit->SetStoreTracks(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
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
      DEBUG_GEO("Out of scope ChannelId: {}. Max voxel ID is: {}.\nPosition: {}\nIdX={}, IdY={}, IdZ={}",
                  voxelId,maxId,position,voxelIdX,voxelIdY,voxelIdZ);
  }
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Determines if the scoring volume is voxelized.
 *
 * @return true if any voxel dimension (X, Y, or Z) is greater than 1; false otherwise.
 */
bool VPatientSD::ScoringVolume::IsVoxelised() const{
  // return true;
  // G4cout << "m_nVoxelsX " << m_nVoxelsX << "   m_nVoxelsY " << m_nVoxelsY << "   m_nVoxelsZ " << m_nVoxelsZ << G4endl;
  return m_nVoxelsX > 1 || m_nVoxelsY > 1 || m_nVoxelsZ > 1;
}
