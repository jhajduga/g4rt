#include "ControlPoint.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include "IO.hh"
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "D3DCell.hh"
#include "D3DDetector.hh"
#include "G4SDManager.hh"
#ifdef G4MULTITHREADED
    #include "G4Threading.hh"
    #include "G4MTRunManager.hh"
#endif
#include <random>
#include "VMlc.hh"
#include "Services.hh"
#include "LogSvc.hh"
#include <numeric> 
#include <cmath>

double ControlPoint::FIELD_MASK_POINTS_DISTANCE = 0.50 * mm;
std::string ControlPoint::m_sim_dir = "sim";

G4Cache<std::map<G4String,std::vector<G4String>>> ControlPoint::m_run_collections;

namespace {
    G4Mutex CPMutex = G4MUTEX_INITIALIZER;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a ControlPointConfig with the specified ID, number of events, and rotation angle.
 *
 * @param id Unique identifier for the control point.
 * @param nevts Number of events associated with this control point.
 * @param rot Rotation angle in degrees for the control point.
 */
ControlPointConfig::ControlPointConfig(int id, int nevts, double rot)
: Id(id), NEvts(nevts),RotationInDeg(rot){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes scoring collections for each run collection and scoring type.
 *
 * For each registered run collection and scoring type, attempts to retrieve the corresponding scoring map from the patient or custom detectors. Adds non-empty scoring collections to the internal map and removes empty ones.
 */
void ControlPointRun::InitializeScoringCollection(){
    std::string worker = G4Threading::IsWorkerThread() ? "worker" : "master";
    auto scoring_types = Service<RunSvc>()->GetScoringTypes(); 
    auto run_collections = ControlPoint::m_run_collections.Get(); 
    LOGSVC_INFO("ControlPoint","Run scoring initialization for #{} collections ({})",run_collections.size(),worker);
    for(const auto& run_collection : run_collections){
        auto run_collection_name = run_collection.first;
        for(const auto& scoring_type: scoring_types){
            if(m_hashed_scoring_map.find(run_collection_name)==m_hashed_scoring_map.end()){
                LOGSVC_INFO("ControlPoint","Initializing new run collection map: {}",run_collection_name);
                m_hashed_scoring_map.insert(std::pair<G4String,ScoringMap>(run_collection_name,ScoringMap()));
            }
            auto& scoring_collection = m_hashed_scoring_map.at(run_collection_name);
            // Try to get scoring collection from any scoring volume in the world..
            std::map<std::size_t, VoxelHit> sc; 
            if(Service<GeoSvc>()->Patient())
                sc = Service<GeoSvc>()->Patient()->GetScoringHashedMap(run_collection_name,scoring_type);
            if(sc.empty()){
                auto customDetectors = Service<GeoSvc>()->CustomDetectors();
                for(auto& cd : customDetectors){
                    sc = cd->GetScoringHashedMap(run_collection_name,scoring_type);
                    if (!sc.empty())
                        break;
                }
            }
            if(sc.empty()){
                LOGSVC_WARN("ControlPoint","Couldn't get scoring collection for {}/{}",run_collection_name,Scoring::to_string(scoring_type));
            }
            // 
                LOGSVC_INFO("ControlPoint","Added scoring collection type: {}",Scoring::to_string(scoring_type));
            scoring_collection[scoring_type] = sc;
            if(scoring_collection[scoring_type].empty()){
                
                LOGSVC_INFO("ControlPoint","Erasing empty scoring collection {}",Scoring::to_string(scoring_type));
                scoring_collection.erase(scoring_type);
            }
            else
                // continue;
                
                
                LOGSVC_INFO("ControlPoint","Scoring collection size for {}: {}",Scoring::to_string(scoring_type),scoring_collection.at(scoring_type).size());
        }
        // G4cout << "Run scoring map size: " << m_hashed_scoring_map[run_collection_name].size() << G4endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Merges scoring data and simulation mask points from a worker run into the current run.
 *
 * Accumulates voxel and cell scoring data from the worker run into the master run, normalizing dose values as needed. After merging, clears the worker run's scoring data and simulation mask points, and transfers the mask points to the master run.
 */
void ControlPointRun::Merge(const G4Run* worker_run){
    
                LOGSVC_INFO("ControlPoint","Run-{} merging...",worker_run->GetRunID());
    auto cell_volume = Service<GeoSvc>()->Patient()->GetCellVolume();
    auto merge = [&](ScoringMap& left, const ScoringMap& right){
        for(auto& scoring : left){
            G4double total_dose(0);
            auto& type = scoring.first;
            bool isVoxel = type == Scoring::Type::Voxel ? true : false;
            // 
                LOGSVC_INFO("ControlPoint","Scoring type: {}",Scoring::to_string(type));
            auto& hashed_scoring_left = scoring.second;
            const auto& hashed_scoring_right = right.at(type);
            for(auto& hashed_voxel : hashed_scoring_left){
                hashed_voxel.second.Cumulate(hashed_scoring_right.at(hashed_voxel.first),isVoxel); // VoxelHit+=VoxelHit
                auto voxel_volume = hashed_voxel.second.GetVolume();
                if(isVoxel && voxel_volume < cell_volume){
                    //
                    // LOGSVC_INFO("ControlPoint","Voxel / Cell volume: {} / {}",voxel_volume,cell_volume);
                    total_dose += hashed_voxel.second.GetDose()*voxel_volume/cell_volume;
                } else {
                    total_dose += hashed_voxel.second.GetDose();
                }
            }
            // 
                LOGSVC_INFO("ControlPoint","Total dose: {}",total_dose);
        } 
    };

    for(auto& scoring : m_hashed_scoring_map){
        auto scoring_name = scoring.first;
        // 
                LOGSVC_INFO("ControlPoint","Merging collection: {}",scoring_name);
        auto& master_scoring = scoring.second;
        // 
                LOGSVC_DEBUG("ControlPoint","Master scoring #types: {}",master_scoring.size());
        const auto& worker_scoring = dynamic_cast<const ControlPointRun*>(worker_run)->m_hashed_scoring_map.at(scoring_name);
        // 
                LOGSVC_DEBUG("ControlPoint","Worker scoring #types: {}",worker_scoring.size());
        merge(master_scoring,worker_scoring);
    }

    // Realease memory after merging... we don't need this anymore.
    dynamic_cast<const ControlPointRun*>(worker_run)->m_hashed_scoring_map.clear();
    auto& w_sim_mask_points = dynamic_cast<const ControlPointRun*>(worker_run)->m_sim_mask_points;
    for(const auto& pos : w_sim_mask_points){
        m_sim_mask_points.push_back(pos);
    }
    w_sim_mask_points.clear();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the scoring collection map for the specified collection name.
 *
 * Logs an error if the collection name is not found.
 *
 * @param name Name of the scoring collection to retrieve.
 * @return Reference to the scoring map associated with the given name.
 */
ScoringMap& ControlPointRun::GetScoringCollection(const G4String& name){
    if (m_hashed_scoring_map.find(name) == m_hashed_scoring_map.end())
        LOGSVC_ERROR("Couldn't find scoring collection in current run: {}",name);
    return m_hashed_scoring_map.at(name);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finalizes the run by processing MLC-related scoring data if present.
 *
 * If scoring collections exist and an MLC model is active, computes field scaling factors and parameterization for the current control point.
 */
void ControlPointRun::EndOfRun(){
    if(m_hashed_scoring_map.size()>0){
        
                LOGSVC_INFO("ControlPoint","ControlPointRun::EndOfRun...");
        if(Service<ConfigSvc>()->GetValue<std::string>("GeoSvc", "MlcModel").compare("None") != 0 ){
            FillMlcFieldScalingFactor();
            FillParameterization();
        }
    }
    else {
        
                LOGSVC_INFO("ControlPoint","ControlPointRun::EndOfRun:: Nothing to do.");
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Calculates and normalizes field and angle scaling factors for all hits in each scoring collection.
 *
 * For each hit, computes the field scaling factor and angle scaling factor using the current control point, then applies min-max normalization to both factors across the collection.
 */
void ControlPointRun::FillMlcFieldScalingFactor(){
    auto current_cp = Service<RunSvc>()->CurrentControlPoint();

    for(auto& scoring_map: m_hashed_scoring_map){
        
                LOGSVC_INFO("ControlPoint","ControlPointRun::Filling Field Scaling Factor for \"{}\" run collection",scoring_map.first);
        
        for(auto& scoring: scoring_map.second){
            
                LOGSVC_INFO("ControlPoint","ControlPointRun::Processing {} scoring... size: {}",Scoring::to_string(scoring.first),scoring.second.size()); 
            G4double max_fsf = -10000.;
            G4double min_fsf =  10000.;
            G4double max_asf = max_fsf;
            G4double min_asf = min_fsf;
            for(auto& hit : scoring.second){
                auto fsf = current_cp->GetFieldScalingFactor(hit.second.GetCentre());
                hit.second.SetFieldScalingFactor(fsf);
                if (fsf > max_fsf) max_fsf = fsf;
                if (fsf < min_fsf) min_fsf = fsf;
                auto asf = current_cp->GetAngleScalingFactor(current_cp->GetDegreeRotation(),hit.second.GetCentre());
                hit.second.SetAngleScalingFactor(asf);
                if (asf > max_asf) max_asf = asf;
                if (asf < min_asf) min_asf = asf;
            } 
            
                LOGSVC_INFO("ControlPoint","ControlPointRun:: Performing min-max normalization...");
            // Normalization (min-max scaling):
            G4double max_new = 0.98;
            G4double min_new = 0.02;
            for(auto& hit : scoring.second){
                auto hit_fsf = hit.second.GetFieldScalingFactor();
                auto new_fsf = (hit_fsf-min_fsf)/(max_fsf-min_fsf) * (max_new-min_new) + min_new;
                hit.second.SetFieldScalingFactor(new_fsf);

                auto hit_asf = hit.second.GetAngleScalingFactor();
                auto new_asf = (hit_asf-min_asf)/(max_asf-min_asf) * (max_new-min_new) + min_new;
                hit.second.SetAngleScalingFactor(new_asf);
            } 
        }
    }
    
                LOGSVC_INFO("ControlPoint","ControlPointRun:: Field Scaling Factor processing - done!");
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Calculates and stores the MLC-defined beam mask area and center of gravity for the current control point.
 *
 * Filters out closed MLC leaves, computes the total open field area and the geometric center of gravity based on the remaining open leaves, and stores these parameters for later use.
 */
void ControlPointRun::FillParameterization(){
    auto current_cp = Service<RunSvc>()->CurrentControlPoint();
    auto mlc_positioning_y1 = current_cp->MLC()->GetMlcPositioning("Y1");
    auto mlc_positioning_y2 = current_cp->MLC()->GetMlcPositioning("Y2");

    // Keep only open leafs
    
                LOGSVC_INFO("ControlPoint","ControlPointRun:: MLC #leafs {}",mlc_positioning_y1.size());
    std::vector<size_t> indices_to_remove;
    for(size_t idx = 0; idx < mlc_positioning_y1.size(); idx++){
        auto leaf_position = mlc_positioning_y1.at(idx);
        auto leaf_position_pair = mlc_positioning_y2.at(idx);
        if((std::abs(leaf_position.getY() - leaf_position_pair.getY())) < 0.00001 ){
            indices_to_remove.push_back(idx);
        }
    }
    // Remove in reverse to avoid shifting
    for (auto it = indices_to_remove.rbegin(); it != indices_to_remove.rend(); ++it) {
        mlc_positioning_y1.erase(mlc_positioning_y1.begin() + *it);
        mlc_positioning_y2.erase(mlc_positioning_y2.begin() + *it);
    }

    
                LOGSVC_INFO("ControlPoint","ControlPointRun:: Filtered MLC #leafs {}",mlc_positioning_y1.size());

    double total_area = 0.0;
    double moment_x = 0.0;
    double moment_y = 0.0;

    double leaf_width = 2.5; // mm

    for(size_t idx=0; idx < mlc_positioning_y1.size(); idx++ ){
        // Leafs are moving along Y axis
        double y_length = std::abs(mlc_positioning_y1.at(idx).getY() - mlc_positioning_y2.at(idx).getY());
        double y_mid = (mlc_positioning_y2.at(idx).getY() + mlc_positioning_y1.at(idx).getY())/2.;
        total_area += y_length * leaf_width;;
        moment_x += mlc_positioning_y1.at(idx).getX();
        moment_y += y_mid;

    }
    m_beam_mask_area = svc::round_with_prec(total_area,3);
    
                LOGSVC_INFO("ControlPoint","ControlPointRun:: Field Area: {}",m_beam_mask_area);

    double cx = svc::round_with_prec(moment_x / mlc_positioning_y1.size(),3);
    double cy = svc::round_with_prec(moment_y / mlc_positioning_y1.size(),3);
    m_beam_mask_gravity_centre = std::make_pair(cx,cy);
    
                LOGSVC_INFO("ControlPoint","ControlPointRun:: Field Centre Of Gravity: {},{}",cx,cy);

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a ControlPoint instance from the given configuration.
 *
 * Initializes scoring types, sets the rotation matrix, and, for RTPlan or CustomPlan field types, reads jaw apertures and MLC leaf positions from the plan file.
 */
ControlPoint::ControlPoint(const ControlPointConfig& config): m_config(config){
    G4cout << " DEBUG: ControlPoint:Ctr: nEvts: " << m_config.NEvts << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: rotation: " << m_config.RotationInDeg << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: FieldType: " << m_config.FieldType << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: FieldSizeA: " << m_config.FieldSizeA << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: FieldSizeB: " << m_config.FieldSizeB << G4endl;
    m_scoring_types = Service<RunSvc>()->GetScoringTypes();
    SetRotation(m_config.RotationInDeg);
    if(m_config.FieldType=="RTPlan" || m_config.FieldType=="CustomPlan"){
        auto dicomSvc = DicomSvc::GetInstance();
        m_jaw_x_aperture = dicomSvc->GetPlan()->ReadJawsAperture(m_config.PlanFile,"X",0,0); // file, side, beamId, cpId
        m_jaw_y_aperture = dicomSvc->GetPlan()->ReadJawsAperture(m_config.PlanFile,"Y",0,0); // file, side, beamId, cpId
        m_mlc_a_positioning.clear();
        m_mlc_b_positioning.clear();
        m_mlc_a_positioning = dicomSvc->GetPlan()->ReadMlcPositioning(m_config.PlanFile,"Y1",0,0); // file, side, beamId, cpId
        m_mlc_b_positioning = dicomSvc->GetPlan()->ReadMlcPositioning(m_config.PlanFile,"Y2",0,0);
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Copy constructor for ControlPoint.
 *
 * Creates a new ControlPoint instance by copying the configuration, rotation matrix, scoring types, field mask points, jaw apertures, and MLC leaf positions from another ControlPoint.
 *
 * @param cp The ControlPoint instance to copy.
 */
ControlPoint::ControlPoint(const ControlPoint& cp):m_config(cp.m_config){
    m_rotation = new G4RotationMatrix(*cp.m_rotation);
    m_scoring_types = cp.m_scoring_types;
    m_plan_mask_points = cp.m_plan_mask_points;
    m_jaw_x_aperture = cp.m_jaw_x_aperture;
    m_jaw_y_aperture = cp.m_jaw_y_aperture;
    m_mlc_a_positioning = cp.m_mlc_a_positioning;
    m_mlc_b_positioning = cp.m_mlc_b_positioning;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Move constructor for ControlPoint, transferring resources from another instance.
 *
 * Transfers configuration, scoring types, rotation matrix pointer, mask points, jaw apertures, and MLC positioning from the source ControlPoint. The source's rotation matrix pointer is set to nullptr to prevent double deletion.
 */
ControlPoint::ControlPoint(ControlPoint&& cp):m_config(cp.m_config){
    m_scoring_types = cp.m_scoring_types;
    m_rotation = cp.m_rotation;
    cp.m_rotation = nullptr;
    m_plan_mask_points = cp.m_plan_mask_points;
    m_jaw_x_aperture = cp.m_jaw_x_aperture;
    m_jaw_y_aperture = cp.m_jaw_y_aperture;
    m_mlc_a_positioning = cp.m_mlc_a_positioning;
    m_mlc_b_positioning = cp.m_mlc_b_positioning;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the ControlPoint class.
 *
 * Releases memory allocated for the rotation matrix, if present.
 */
ControlPoint::~ControlPoint() {
    if (m_rotation) delete m_rotation; m_rotation = nullptr;

};

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Creates and stores a new ControlPointRun instance in a thread-safe manner.
 *
 * If scoring is enabled, the new run is configured accordingly. The run is added to the thread-local collection and registered for the current control point.
 *
 * @param scoring Whether to enable scoring for the new run.
 * @return Pointer to the newly created ControlPointRun.
 */
G4Run* ControlPoint::GenerateRun(bool scoring){
    G4AutoLock lock(&CPMutex);
    m_mt_run.Get().push_back(new ControlPointRun(scoring));
    m_cp_run.Put(m_mt_run.Get().back());
    return m_mt_run.Get().back();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the rotation matrix for the control point based on the specified rotation angle in degrees.
 *
 * Updates the internal rotation matrix to represent a rotation around the Y-axis by the given angle.
 *
 * @param rotationInDegree Rotation angle in degrees.
 */
void ControlPoint::SetRotation(double rotationInDegree) { 
    m_config.RotationInDeg = rotationInDegree;
    if(m_rotation) 
        delete m_rotation;
    G4ThreeVector AxisOfRotation = G4ThreeVector(0.,1.,0.).unit();
    m_rotation = new G4RotationMatrix();
    // Geant4::SystemOfUnitsconst::rad = 1.;
    // Geant4::SystemOfUnitsconst::deg = rad*pi/180.;
    m_rotation->rotate(rotationInDegree*deg, AxisOfRotation); // convert deg to rad
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the output directory path for simulation results, creating it if it does not exist.
 *
 * @return std::string Path to the simulation output directory.
 */
std::string ControlPoint::GetOutputDir() {
    auto dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir")+
                "/"+m_sim_dir;
    IO::CreateDirIfNotExits(dir);
    return dir;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the base output file path for simulation results, ensuring the directory exists.
 *
 * The returned path does not include a file extension and is constructed using the plan file name within the output directory.
 * The necessary directory structure is created if it does not already exist.
 *
 * @return std::string The base output file path for the current control point.
 */
std::string ControlPoint::GetOutputFileName() const {
    auto job = Service<RunSvc>()->GetJobNameLabel();
    auto plan_file_name = std::filesystem::path(GetPlanFile()).stem().string();
    IO::CreateDirIfNotExits(GetOutputDir()+"/"+plan_file_name);
    return GetOutputDir()+"/"+plan_file_name+"/"+plan_file_name; // No extension here!  
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the simulation output ROOT file name for this control point.
 *
 * If running in a multithreaded worker context, returns a path within a subjob directory specific to the control point; otherwise, returns the standard output file name with a "_sim.root" suffix.
 *
 * @param workerMT Indicates if the function is called from a multithreaded worker.
 * @return std::string Full path to the simulation output ROOT file.
 */
std::string ControlPoint::GetSimOutputTFileName(bool workerMT) const {
    auto numberOfThreads = Service<ConfigSvc>()->GetValue<int>("RunSvc", "NumberOfThreads");
    std::string postfix =  "_sim.root";
    if(workerMT){ // && numberOfThreads > 1
        auto subjob_dir = GetOutputDir()+"/subjobs";
        IO::CreateDirIfNotExits(subjob_dir);
        return subjob_dir+"/cp-"+std::to_string(GetId())+postfix;
    }
    
    return GetOutputFileName()+postfix;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the field mask points for the specified type.
 *
 * Retrieves either the plan or simulation field mask points, generating the plan mask if necessary. If the type is unrecognized, a fatal exception is thrown.
 *
 * @param type The type of field mask to retrieve ("Plan" or "Sim").
 * @return Reference to the vector of field mask points.
 */
const std::vector<G4ThreeVector>& ControlPoint::GetFieldMask(const std::string& type) {
    if(!Service<GeoSvc>()->IsWorldBuilt()){
        LOGSVC_WARN("ControlPoint","World not yet built, returning empty sim mask point vector");
        return m_plan_mask_points;
    }
    if(type=="Plan") {
        if(m_plan_mask_points.empty())
            FillPlanFieldMask(); // World has to be constructed already
        return m_plan_mask_points;
    } else if(type=="Sim") {
        if(m_cp_run.Get()->GetSimMaskPoints().empty())
            LOGSVC_WARN("ControlPoint","Returning empty sim mask point vector");
        return m_cp_run.Get()->GetSimMaskPoints();
    }
    G4String msg = "Couldn't recognize control point field mask type!";
    LOGSVC_FATAL("ControlPoint", msg.data());
    G4Exception("ControlPoint", "GetFieldMask", FatalErrorInArgument , msg);
    return m_plan_mask_points;
}

////////////////////////////////////////////////////////////////////////////////
/// The field mask is formed at the isocentre Z position,
/**
 * @brief Adds particle positions projected onto the mask plane (Z=0) to the simulation field mask.
 *
 * For each provided primary vertex, its position is transformed to the mask plane and added to the simulation mask points, up to a threshold determined by the number of threads.
 */
void ControlPoint::FillSimFieldMask(const std::vector<G4PrimaryVertex*>& p_vrtx){
    auto configSvc = Service<ConfigSvc>();

    auto& sim_mask_points = m_cp_run.Get()->GetSimMaskPoints();
    auto nCPU = configSvc->GetValue<int>("RunSvc", "NumberOfThreads");
    if(sim_mask_points.size() < 50000./nCPU){
        for(const auto& vrtx : p_vrtx){
            sim_mask_points.push_back(VMlc::GetPositionInMaskPlane(vrtx));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Fills the plan field mask points for the current control point.
 *
 * Generates the set of mask points at the isocenter Z position based on the configured field type. For rectangular or ellipsoidal fields, fills the mask using a regular grid; for RTPlan or custom plans, samples points according to the MLC-defined field. Throws a fatal exception if the mask remains empty after filling.
 */
void ControlPoint::FillPlanFieldMask(){
    // It should happen once for single control point at the time
    // when the current control point is set, see RunSvc::CurrentControlPoint
    // NOTE: The field mask is formed at the isocentre Z position!
    if(m_plan_mask_points.empty()){
        
                LOGSVC_DEBUG("ControlPoint", "Filling the field mask points");
    }
    else{
        G4String msg = "Control Point mask is already filled! This shouldn't happen!";
        LOGSVC_FATAL("ControlPoint", msg.data());
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument , msg);
    }
    auto configSvc = Service<ConfigSvc>();
    
                LOGSVC_DEBUG("ControlPoint", "Using the {} field shape and {} deg rotation",m_config.FieldType,GetDegreeRotation());

    double z_position = configSvc->GetValue<G4ThreeVector>("WorldConstruction", "Isocentre").getZ();
    
    // NOTE: The MLC instance takes care for being set for the current
    //       control point configuration!

    if( m_config.FieldType.compare("Rectangular")==0 ||
        m_config.FieldType.compare("Elipsoidal")==0){
        FillPlanFieldMaskForRegularShapes(z_position);
    }
    if(m_config.FieldType.compare("RTPlan")==0 || m_config.FieldType.compare("CustomPlan")==0){
        FillPlanFieldMaskForInputPlan(z_position);
    }
    if(m_plan_mask_points.empty()){
        G4String msg = "Field Mask not filled! Verify job configuration!";
        LOGSVC_FATAL("ControlPoint",msg.data());
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument, msg);
    }
    
                LOGSVC_DEBUG("ControlPoint", "Filled with {} number of points",m_plan_mask_points.size());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Fills the plan field mask points for rectangular or ellipsoidal fields at a given Z position.
 *
 * Generates a grid of points within the defined field size, applying rotation if specified. For ellipsoidal fields, only points inside the ellipse are included; for rectangular fields, all grid points are used.
 *
 * @param current_z The Z-coordinate at which the mask points are generated.
 */
void ControlPoint::FillPlanFieldMaskForRegularShapes(double current_z){
    double current_x,current_y;
    double x_range, y_range;

    auto rotate = [&](const G4ThreeVector& position) -> G4ThreeVector {
        return m_rotation ? *m_rotation * position : position;
    };
    x_range = m_config.FieldSizeA;
    y_range = m_config.FieldSizeB;
    if(x_range < 0 || y_range < 0){
        G4String msg = "Field size is not correct";
        LOGSVC_FATAL("ControlPoint","Field size is not correct: A {}, B {}",x_range,y_range);
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument, msg);
    }

    double min_x = - x_range / 2.;
    double max_x = + x_range / 2.;
    double dx = FIELD_MASK_POINTS_DISTANCE;
    int n_x = (max_x - min_x ) / dx ;
    double min_y = - y_range / 2.;
    double max_y = + y_range / 2.;
    double dy = FIELD_MASK_POINTS_DISTANCE;
    int n_y = (max_y - min_y ) / dy;
    current_x = min_x + (0.5 * FIELD_MASK_POINTS_DISTANCE);
    current_y = min_y + (0.5 * FIELD_MASK_POINTS_DISTANCE);
    for(int i = 0; i<n_x; i++){
        current_y = min_y + (0.5 * FIELD_MASK_POINTS_DISTANCE);
        for(int j = 0; j<n_y; j++){
            if(m_config.FieldType.compare("Elipsoidal")==0){
                if ((pow(current_x,2)/ pow((x_range / 2.),2) + pow(current_y,2)/pow((y_range / 2.),2)) < 1 ){
                    m_plan_mask_points.push_back(rotate(G4ThreeVector(current_x,current_y,current_z)));
                } 
            } else { // Rectangular
                m_plan_mask_points.push_back(rotate(G4ThreeVector(current_x,current_y,current_z)));
            }
            current_y += dy;
        }
        current_x += dx ;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Fills the plan field mask with random points inside the MLC-defined field for input plans.
 *
 * Randomly samples up to 10 million points within a large square region at the specified Z position, adding those that fall inside the MLC field to the plan mask. Stops when 100,000 points have been collected or the sample limit is reached.
 *
 * @param current_z The Z-coordinate at which to sample the field mask points.
 */
void ControlPoint::FillPlanFieldMaskForInputPlan(double current_z){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis_x(-200*mm, 200*mm);
    std::uniform_real_distribution<double> dis_y(-200*mm, 200*mm);

    for (int i = 0; i < 10000000; ++i) {
        auto x = dis_x(gen);
        auto y = dis_y(gen);
        auto in_field = MLC()->IsInField(G4ThreeVector(x,y,current_z));
        // std::cout << x << " " << y << " " << current_z << " inField "<< in_field <<  std::endl;
        if(in_field){
            m_plan_mask_points.push_back(G4ThreeVector(x,y,current_z));
        }
        if(m_plan_mask_points.size()>=100000) break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports voxel scoring volume data to a CSV file.
 *
 * Writes the center position, mask plane-transformed position, and field scaling factor for each voxel in the provided scoring map to a CSV file named according to the control point ID and scoring volume name.
 *
 * @param scoring_vol_name Name identifier for the scoring volume, used in the output filename.
 * @param volume_scoring Map of voxel indices to VoxelHit objects containing scoring and position data.
 */
void ControlPoint::DumpVolumeMaskToFile(std::string scoring_vol_name, const std::map<std::size_t, VoxelHit>& volume_scoring) const { // TODEL? 
    auto output_dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "OutputDir");
    const std::string file = output_dir+"/cp-"+std::to_string(GetId())+"_scoring_volume"+scoring_vol_name+"mask.csv";
    std::string header = "X [mm],Y [mm],Z [mm],mX [mm],mY [mm],mZ [mm],inFieldTag";
    std::ofstream c_outFile;
    c_outFile.open(file.c_str(), std::ios::out);
    c_outFile << header << std::endl;
    for(auto& vol : volume_scoring){
        auto pos = vol.second.GetCentre();
        auto trans_pos = VMlc::GetPositionInMaskPlane(pos);
        auto inFieldTag = vol.second.GetFieldScalingFactor();
        // std::cout << "z: " << pos.getZ() << "  trans z: "<< trans_pos.getZ() << std::endl;
        c_outFile << pos.getX() << "," << pos.getY() << "," << pos.getZ();
        c_outFile << "," << trans_pos.getX() << "," << trans_pos.getY() << "," << trans_pos.getZ() << "," << inFieldTag << std::endl;

    }
    c_outFile.close();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Calculates the field scaling factor at a given position based on MLC leaf positions.
 *
 * Computes a scaling factor that quantifies the geometric influence of the multi-leaf collimator (MLC) leaf positions on a specified point in space. The factor is derived from the angular relationships between the point and each pair of opposing MLC leaves, summing the squared angles for both Y1 and Y2 leaf banks.
 *
 * @param position The spatial point at which to evaluate the field scaling factor.
 * @return G4double The computed field scaling factor for the given position.
 */
G4double ControlPoint::GetFieldScalingFactor(const G4ThreeVector& position) const {
    auto mlc_positioning_y1 = MLC()->GetMlcPositioning("Y1");
    auto mlc_positioning_y2 = MLC()->GetMlcPositioning("Y2");
    // G4cout << "mlc_positioning Y1 | Y2: " << G4endl;
    // for (int i=0; i<mlc_positioning_y1.size();i++){
    //     G4cout << mlc_positioning_y1.at(i) << " | " << mlc_positioning_y2.at(i) << G4endl;
    // }
    // std::vector<G4ThreeVector> mlc_positioning_y1 = {{-2,1,-600},{-1,1,-600},{1,-1,-600},{-1,1,-600}};
    // std::vector<G4ThreeVector> mlc_positioning_y2 = {{-3,1,-600},{-1,1,-600},{1,-1,-600},{-1,2,-600}};
    auto mlc_centre = G4ThreeVector(0,0,mlc_positioning_y2.front().getZ());
    auto getInfluenceFactor = [&](
        const std::vector<G4ThreeVector>& mlc_positioning_1,
        const std::vector<G4ThreeVector>& mlc_positioning_2) -> G4double {
        G4double influence_factor = 0; 
        for(size_t idx=0; idx < mlc_positioning_1.size(); idx++ ){
            auto leaf_position = mlc_positioning_1.at(idx);
            auto leaf_position_pair = mlc_positioning_2.at(idx);
            if((std::abs(leaf_position.getY() - leaf_position_pair.getY())) < 0.00001 )
                continue;
            auto relative_mlc_position = mlc_centre - position;
            auto relative_leaf_position = leaf_position - position;
            // auto lambda_i = relative_mlc_position.mag() / relative_leaf_position.mag();
            auto lambda_i = relative_mlc_position.angle(relative_leaf_position);
            // influence_factor*=lambda_i;
            // std::cout << "position = " << position << " lambda_i = " << lambda_i << std::endl;
            lambda_i = lambda_i * (180.0 / M_PI);
            influence_factor+=(lambda_i*lambda_i);
        }
        // std::cout << "position = " << position <<"  influence_factor = " << influence_factor << std::endl;
        return influence_factor;
    };
    
    auto influence_factor_y1 = getInfluenceFactor(mlc_positioning_y1,mlc_positioning_y2);
    // std::cout << std::endl;
    auto influence_factor_y2 = getInfluenceFactor(mlc_positioning_y2,mlc_positioning_y1);

    // return std::pow((influence_factor_y1*influence_factor_y2),1/120.0);
    return influence_factor_y1+influence_factor_y2;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Calculates an angle-based scaling factor for a given position relative to the beam direction.
 *
 * Computes the perpendicular distance from the beam line at a specified rotation angle to the given position, then applies a modified sigmoid function to the angle to produce a scaling factor. Used for field scaling in radiation therapy simulations.
 *
 * @param angle Rotation angle in degrees.
 * @param position Position in 3D space for which the scaling factor is calculated.
 * @return G4double The computed angle scaling factor.
 */
G4double ControlPoint::GetAngleScalingFactor(G4double angle, const G4ThreeVector& position) const {
    if (angle==180) // Edge case, not to divide by 0
        angle+=0.01;
    double angleInRadians = angle * M_PI / 180.0;
    
    // Make the angle symmetric [-M_PI,M_PI] insted of [0,2M_PI]
    angleInRadians = angleInRadians - M_PI;

    G4ThreeVector isocentre(0.0, 0.0, 0.0); // Point on the beam line
    G4ThreeVector beamDirection(std::sin(angleInRadians), 0.0, std::cos(angleInRadians)); // Direction of the line (in the XZ plane)

    // Calculate the vector from the isocentre to the given point
    G4ThreeVector isoToPoint = position - isocentre;

    // Calculate the cross product of the direction and the vector to the point
    G4ThreeVector crossProduct = beamDirection.cross(isoToPoint);

    // Calculate the distance
    double distance = crossProduct.mag() / beamDirection.mag();

    // Modified sigmoid function:
    auto sig = [](double x){ 
        return -2./(1 + std::exp(-x))+1;
    };

    return distance * sig(angleInRadians);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Fills scoring collections with hit data from the current event.
 *
 * Iterates over all registered run and hit collections, retrieves the corresponding hit collections from the event, and accumulates their data into the appropriate scoring collections for this control point.
 *
 * @param evtHC Pointer to the event's hit collection container.
 */
void ControlPoint::FillEventCollections(G4HCofThisEvent* evtHC){
    for(const auto& run_collection: ControlPoint::m_run_collections.Get()){
        // 
                // LOGSVC_INFO("ControlPoint","RunAnalysis::EndOfEvent: RunColllection {}",run_collection.first);
                // LOGSVC_WARN("ControlPoint","RunAnalysis::EndOfEvent: RunColllection {}",run_collection.first);
                // LOGSVC_DEBUG("ControlPoint","RunAnalysis::EndOfEvent: RunColllection {}",run_collection.first);
        for(const auto& hc: run_collection.second){
            // Related SensitiveDetector collection ID (Geant4 architecture)
            auto collID = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
            // collID==-1 the collection is not found
            // collID==-2 the collection name is ambiguous
            if(collID<0){
                
                LOGSVC_INFO("ControlPoint","ControlPoint::FillEventCollections: HC: {} / G4SDManager Err: {}", hc, collID);
            }
            else {
                auto thisHitsCollPtr = evtHC->GetHC(collID);
                if(thisHitsCollPtr) // The particular collection is stored at the current event.
                    FillEventCollection(run_collection.first,dynamic_cast<VoxelHitsCollection*>(thisHitsCollPtr));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Accumulates hit data from a voxel hits collection into the specified run collection's scoring maps.
 *
 * For each hit in the provided collection, updates the corresponding scoring collection (cell or voxel) within the run collection by cumulating hit data based on the appropriate hashed identifier.
 */
void ControlPoint::FillEventCollection(const G4String& run_collection, VoxelHitsCollection* hitsColl){
    int nHits = hitsColl->entries();
    auto hc_name = hitsColl->GetName();
    if(nHits==0){
        return; // no hits in this event
    }
    auto& scoring_collection = GetRun()->GetScoringCollection(run_collection);
    for (int i=0;i<nHits;i++){ // a.k.a. voxel loop
        auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
        for(auto& sc : scoring_collection ){
            const auto& current_scoring_type = sc.first;
            auto& current_scoring_collection = sc.second;
            std::size_t hashed_id;
            bool exact_volume_match = true;
            switch (current_scoring_type){
                case Scoring::Type::Cell:
                    hashed_id = hit->GetGlobalHashedStrId();
                    exact_volume_match = false;
                    break;
                case Scoring::Type::Voxel:
                    hashed_id = hit->GetHashedStrId();
                    break;
                default:
                    break;
            }
            current_scoring_collection.at(hashed_id).Cumulate(*hit,exact_volume_match);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Registers a hit collection name under a specified run collection.
 *
 * If the run collection does not exist, it is created. The hit collection name is then added to the run collection.
 */
void ControlPoint::RegisterRunHCollection(const G4String& run_collection_name, const G4String& hc_name){
    if(m_run_collections.Get().find( run_collection_name ) == m_run_collections.Get().end()){
        m_run_collections.Get()[run_collection_name] = std::vector<G4String>();
        
                LOGSVC_INFO("ControlPoint","Register new run collection:  {} ", run_collection_name);
    }
    m_run_collections.Get().at(run_collection_name).emplace_back(hc_name);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the names of all registered run collections.
 *
 * @return Vector of run collection names.
 */
std::vector<G4String> ControlPoint::GetRunCollectionNames() {
    std::vector<G4String> run_collection_names;
    for(const auto& run_collection: ControlPoint::m_run_collections.Get()){
        // G4cout << "RunCollection: " << run_collection.first << G4endl;
        run_collection_names.emplace_back(run_collection.first);
    }
    return run_collection_names;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the set of all unique hit collection names registered across all run collections.
 *
 * The set is cached after the first call for efficiency.
 *
 * @return Set of unique hit collection names.
 */
std::set<G4String> ControlPoint::GetHitCollectionNames() {
    static std::set<G4String> hit_collection_names;
        if(hit_collection_names.empty()){
        for(const auto& run_collection: ControlPoint::m_run_collections.Get()){
            const auto& rc_hcs = run_collection.second;
            hit_collection_names.insert(rc_hcs.begin(), rc_hcs.end());
        }
    }
    return hit_collection_names;
}

/**
 * @brief Returns the MLC (multi-leaf collimator) instance for the current control point.
 *
 * Verifies that the MLC is properly initialized for this control point. If initialization fails, logs an error and terminates the program.
 *
 * @return VMlc* Pointer to the initialized MLC instance.
 */
VMlc* ControlPoint::MLC() const {
    // G4cout << "ControlPoint::MLC:: #{} CP" << Id() << G4endl;
    auto mlc = Service<GeoSvc>()->MLC();
    if(!mlc->Initialized(this)){
        LOGSVC_ERROR("ControlPoint","ControlPoint::MLC:Initialization failed for #{} CP", Id());
        std::exit(EXIT_FAILURE);
    }
    return mlc;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the MLC leaf positioning vector for the specified side.
 *
 * @param side The MLC side ("Y1" or "Y2").
 * @return const std::vector<double>& Reference to the vector of leaf positions for the given side.
 *
 * @note Exits the program if an unknown side is specified.
 */
const std::vector<double>& ControlPoint::GetMlcPositioning(const std::string& side) const {
    auto dicomSvc = DicomSvc::GetInstance();
    if(side=="Y1"){
        return m_mlc_a_positioning;
    }
    else if(side=="Y2"){
        return m_mlc_b_positioning;
    }
    else{
        LOGSVC_ERROR("ControlPoint","ControlPoint::GetMlcPositioning: Unknown side: {}", side);
        std::exit(EXIT_FAILURE);
    }
    return m_mlc_a_positioning; // never reached, prevent warning
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the jaw aperture size for the specified side.
 *
 * Retrieves the aperture value for one of the jaws ("X1", "X2", "Y1", or "Y2") of the radiation field.
 * Exits the program if an unknown side is provided.
 *
 * @param side The jaw side ("X1", "X2", "Y1", or "Y2").
 * @return double The aperture size for the specified jaw side.
 */
double ControlPoint::GetJawAperture(const std::string& side) const{
    if(side=="X1"){
        return m_jaw_x_aperture.first;
    }
    else if(side=="X2"){
        return m_jaw_x_aperture.second;
    }
    if(side=="Y1"){
        return m_jaw_y_aperture.first;
    }
    else if(side=="Y2"){
        return m_jaw_y_aperture.second;
    }
    else{
        LOGSVC_ERROR("ControlPoint","ControlPoint::GetJawAperture: Unknown side: {}", side);
        std::exit(EXIT_FAILURE);
    }
    return 0.; // never reached, prevent warning
}




