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
#include "logger.hh"
#include <numeric> 

double ControlPoint::FIELD_MASK_POINTS_DISTANCE = 0.50 * mm;
std::string ControlPoint::m_sim_dir = "sim";

std::map<G4String,std::vector<G4String>> ControlPoint::m_run_collections = std::map<G4String,std::vector<G4String>>();

namespace {
    G4Mutex CPMutex = G4MUTEX_INITIALIZER;
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig::ControlPointConfig(int id, int nevts, double rot)
: Id(id), NEvts(nevts),RotationInDeg(rot){}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::InitializeScoringCollection(){
    std::string worker = G4Threading::IsWorkerThread() ? "worker" : "master";
    // if (G4Threading::IsWorkerThread() == false) {
    // Logger::LogInfo("Message from Worker thread.");
    auto scoring_types = Service<RunSvc>()->GetScoringTypes(); 
    auto run_collections = ControlPoint::m_run_collections; 
    // // LOGSVC_INFO("Run scoring initialization for #{} collections ({})",run_collections.size(),worker);
    for(const auto& run_collection : run_collections){
        auto run_collection_name = run_collection.first;
        for(const auto& scoring_type: scoring_types){
            if(m_hashed_scoring_map.find(run_collection_name)==m_hashed_scoring_map.end()){
                // // LOGSVC_INFO("Initializing new run collection map: {}",run_collection_name);
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
                // LOGSVC_WARN("Couldn't get scoring collection for {}",Scoring::to_string(scoring_type));
            }
            // // LOGSVC_INFO("Added scoring collection type: {}",Scoring::to_string(scoring_type));
            scoring_collection[scoring_type] = sc;
            if(scoring_collection[scoring_type].empty()){
                // LOGSVC_INFO("Erasing empty scoring collection {}",Scoring::to_string(scoring_type));
                scoring_collection.erase(scoring_type);
            }
            else
                continue;
                // // LOGSVC_INFO("Scoring collection size for {}: {}",Scoring::to_string(scoring_type),scoring_collection.at(scoring_type).size());
        }
        // G4cout << "Run scoring map size: " << m_hashed_scoring_map[run_collection_name].size() << G4endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::Merge(const G4Run* worker_run){
    // LOGSVC_INFO("Run-{} merging...",worker_run->GetRunID());
    auto cell_size = D3DCell::SIZE;
    auto cell_volume = cell_size*cell_size*cell_size;
    auto merge = [&](ScoringMap& left, const ScoringMap& right){
        for(auto& scoring : left){
            G4double total_dose(0);
            auto& type = scoring.first;
            bool isVoxel = type == Scoring::Type::Voxel ? true : false;
            // // LOGSVC_INFO("Scoring type: {}",Scoring::to_string(type));
            auto& hashed_scoring_left = scoring.second;
            const auto& hashed_scoring_right = right.at(type);
            for(auto& hashed_voxel : hashed_scoring_left){
                hashed_voxel.second.Cumulate(hashed_scoring_right.at(hashed_voxel.first),isVoxel); // VoxelHit+=VoxelHit
                auto voxel_volume = hashed_voxel.second.GetVolume();
                if(isVoxel && voxel_volume < cell_volume){
                    //// LOGSVC_INFO("Voxel / Cell volume: {} / {}",voxel_volume,cell_volume);
                    total_dose += hashed_voxel.second.GetDose()*voxel_volume/cell_volume;
                } else {
                    total_dose += hashed_voxel.second.GetDose();
                }
            }
            // // LOGSVC_INFO("Total dose: {}",total_dose);
        } 
    };

    for(auto& scoring : m_hashed_scoring_map){
        auto scoring_name = scoring.first;
        // // LOGSVC_INFO("Merging collection: {}",scoring_name);
        auto& master_scoring = scoring.second;
        // // LOGSVC_DEBUG("Master scoring #types: {}",master_scoring.size());
        const auto& worker_scoring = dynamic_cast<const ControlPointRun*>(worker_run)->m_hashed_scoring_map.at(scoring_name);
        // // LOGSVC_DEBUG("Worker scoring #types: {}",worker_scoring.size());
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
///
ScoringMap& ControlPointRun::GetScoringCollection(const G4String& name){
    if (m_hashed_scoring_map.find(name) == m_hashed_scoring_map.end())
        // LOGSVC_ERROR("Couldn't find scoring collection in current run: {}",name)
    return m_hashed_scoring_map.at(name);
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::EndOfRun(){
    if(m_hashed_scoring_map.size()>0){
        // LOGSVC_INFO("ControlPointRun::EndOfRun...");
        FillMlcFieldScalingFactor();
    }
    else {
        // LOGSVC_INFO("ControlPointRun::EndOfRun:: Nothing to do.");
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::FillMlcFieldScalingFactor(){
    auto current_cp = Service<RunSvc>()->CurrentControlPoint();
    // weights for each dimention
    G4double wx = 1.;
    G4double wy = 2.;
    G4double wz = 3.;

    // the patient parameterization
    G4int nx = 1.;
    G4int ny = 1.;
    G4int nz = 1.;
    auto patient = Service<GeoSvc>()->Patient();
    if(patient){
        auto d3d_det = dynamic_cast<const D3DDetector*>(patient);
        if(d3d_det){
            auto config = d3d_det->GetConfig();
            nx = config.m_nX_cells;
            ny = config.m_nY_cells;
            nz = config.m_nZ_cells;
        }
    }
    auto patientNormalizationFactor = wx*nx+wy*ny+wz*nz;
    // LOGSVC_INFO("ControlPointRun::Filling Field Scaling Factor with Patient Normalization Factor {},{},{}->{}",nx,ny,nz,patientNormalizationFactor);


    for(auto& scoring_map: m_hashed_scoring_map){
        // LOGSVC_INFO("ControlPointRun::Filling Field Scaling Factor for \"{}\" run collection",scoring_map.first);
        
        for(auto& scoring: scoring_map.second){
            // LOGSVC_INFO("ControlPointRun::Processing {} scoring... size: {}",Scoring::to_string(scoring.first),scoring.second.size()); 
            G4double max = -10000.;
            G4double min =  10000.;
            for(auto& hit : scoring.second){
                // hit.second.SetFieldScalingFactor(current_cp->GetMlcFieldScalingFactor(hit.second.GetCentre()));
                auto fsf = current_cp->GetMlcWeightedInfluenceFactor(hit.second.GetCentre());
                fsf = fsf/patientNormalizationFactor;
                hit.second.SetFieldScalingFactor(fsf);
                if (fsf > max) max = fsf;
                if (fsf < min) min = fsf;
            } 
            // LOGSVC_INFO("ControlPointRun:: Performing min-max normalization...");
            // Normalization (min-max scaling):
            G4double max_new = 0.98;
            G4double min_new = 0.02;
            for(auto& hit : scoring.second){
                auto hit_fsf = hit.second.GetFieldScalingFactor();
                auto new_fsf = (hit_fsf-min)/(max-min) * (max_new-min_new) + min_new;
                hit.second.SetFieldScalingFactor(new_fsf);
            } 
        }
    }
    // LOGSVC_INFO("ControlPointRun:: Field Scaling Factor processing - done!");
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(const ControlPointConfig& config): m_config(config){
    G4cout << " DEBUG: ControlPoint:Ctr: nEvts: " << m_config.NEvts << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: rotation: " << m_config.RotationInDeg << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: FieldType: " << m_config.FieldType << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: FieldSizeA: " << m_config.FieldSizeA << G4endl;
    G4cout << " DEBUG: ControlPoint:Ctr: FieldSizeB: " << m_config.FieldSizeB << G4endl;
    m_scoring_types = Service<RunSvc>()->GetScoringTypes();
    SetRotation(config.RotationInDeg);
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
///
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
///
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
///
ControlPoint::~ControlPoint() {
    if (m_rotation) delete m_rotation; m_rotation = nullptr;
    // for(auto run : m_mt_run){
    //     delete run;
    //     //TODO m_mt_run.erase(run);
    // }
};

////////////////////////////////////////////////////////////////////////////////
/// Multi-thread safe method
G4Run* ControlPoint::GenerateRun(bool scoring){
    G4AutoLock lock(&CPMutex);
    m_mt_run.push_back(new ControlPointRun(scoring));
    m_cp_run.Put(m_mt_run.back());
    return m_mt_run.back();
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
std::string ControlPoint::GetOutputDir() {
    auto dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir")+
                "/"+m_sim_dir;
    IO::CreateDirIfNotExits(dir);
    return dir;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string ControlPoint::GetOutputFileName() const {
    auto job = Service<RunSvc>()->GetJobNameLabel();
    auto plan_file_name = std::filesystem::path(GetPlanFile()).stem().string();
    IO::CreateDirIfNotExits(GetOutputDir()+"/"+plan_file_name);
    return GetOutputDir()+"/"+plan_file_name+"/"+plan_file_name; // No extension here!  
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
const std::vector<G4ThreeVector>& ControlPoint::GetFieldMask(const std::string& type) {
    if(!Service<GeoSvc>()->IsWorldBuilt()){
        std::cout << "World not yet built, returning empty sim mask point vector" << std::endl;
        // LOGSVC_WARN("World not yet built, returning empty sim mask point vector");
        return m_plan_mask_points;
    }
    if(type=="Plan") {
        if(m_plan_mask_points.empty())
            FillPlanFieldMask(); // World has to be constructed already
        return m_plan_mask_points;
    } else if(type=="Sim") {
        if(m_cp_run.Get()->GetSimMaskPoints().empty())
            std::cout << "Returning empty sim mask point vector" << std::endl;
            // LOGSVC_WARN("Returning empty sim mask point vector");
        return m_cp_run.Get()->GetSimMaskPoints();
    }
    G4String msg = "Couldn't recognize control point field mask type!";
    std::cout << msg << std::endl;
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("ControlPoint", "GetFieldMask", FatalErrorInArgument , msg);
    return m_plan_mask_points;
}

////////////////////////////////////////////////////////////////////////////////
/// The field mask is formed at the isocentre Z position,
/// => passed particle position is being propagated to the the plane at Z=0
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
/// 
void ControlPoint::FillPlanFieldMask(){
    // It should happen once for single control point at the time
    // when the current control point is set, see RunSvc::CurrentControlPoint
    // NOTE: The field mask is formed at the isocentre Z position!
    if(m_plan_mask_points.empty()){
        // LOGSVC_DEBUG("Filling the field mask points");
    }
    else{
        G4String msg = "Control Point mask is already filled! This shouldn't happen!";
        // LOGSVC_CRITICAL(msg.data());
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument , msg);
    }
    auto configSvc = Service<ConfigSvc>();
    // LOGSVC_DEBUG("Using the {} field shape and {} deg rotation",m_config.FieldType,GetDegreeRotation());

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
        // LOGSVC_CRITICAL(msg.data());
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument, msg);
    }
    // LOGSVC_DEBUG("Filled with {} number of points",m_plan_mask_points.size());
}

////////////////////////////////////////////////////////////////////////////////
///
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
        // LOGSVC_CRITICAL("Field size is not correct: A {}, B {}",x_range,y_range);
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
///
void ControlPoint::FillPlanFieldMaskForInputPlan(double current_z){
    auto rotate = [&](const G4ThreeVector& position) -> G4ThreeVector {
        return m_rotation ? *m_rotation * position : position;
    };

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
            m_plan_mask_points.push_back(rotate(G4ThreeVector(x,y,current_z)));
        }
        if(m_plan_mask_points.size()>=100000) break;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
G4double ControlPoint::GetMlcFieldScalingFactor(const G4ThreeVector& position) const {
    // TODO: DESCRIBE ME - HOW IT WORSKS !!!!
    G4double closest_dist{10.e9};
    auto maskLevelPosition = VMlc::GetPositionInMaskPlane(position);
    if(MLC()->IsInField(maskLevelPosition)){
        return 1;
    }
    else{
        for(const auto& mp : m_plan_mask_points){
            auto current_dist = sqrt(mp.diff2(maskLevelPosition));
            if(current_dist>0){
                if(closest_dist>current_dist)
                    closest_dist = current_dist;
            }
        }
        // return 1. / ((closest_dist+FIELD_MASK_POINTS_DISTANCE)/(FIELD_MASK_POINTS_DISTANCE));
        return  exp(-(closest_dist+FIELD_MASK_POINTS_DISTANCE)/(FIELD_MASK_POINTS_DISTANCE));
    }
    return 1.;
}

////////////////////////////////////////////////////////////////////////////////
///
G4double ControlPoint::GetMlcWeightedInfluenceFactor(const G4ThreeVector& position) const {
    auto mlc_positioning_y1 = MLC()->GetMlcPositioning("Y1");
    auto mlc_positioning_y2 = MLC()->GetMlcPositioning("Y2");
    // G4cout << "mlc_positioning Y1 | Y2: " << G4endl;
    // for (int i=0; i<mlc_positioning_y1.size();i++){
    //     G4cout << mlc_positioning_y1.at(i) << " | " << mlc_positioning_y2.at(i) << G4endl;
    // }
    // std::vector<G4ThreeVector> mlc_positioning_y1 = {{-2,1,-600},{-1,1,-600},{1,-1,-600},{-1,1,-600}};
    // std::vector<G4ThreeVector> mlc_positioning_y2 = {{-3,1,-600},{-1,1,-600},{1,-1,-600},{-1,2,-600}};
    auto mlc_centre = G4ThreeVector(0,0,mlc_positioning_y2.front().getZ());
    auto getInfluenceFactor = [&](const std::vector<G4ThreeVector>& mlc_positioning) -> G4double {
        G4double influence_factor = 0; 
        for(const auto& leaf_position : mlc_positioning){
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
    
    auto influence_factor_y1 = getInfluenceFactor(mlc_positioning_y1);
    // std::cout << std::endl;
    auto influence_factor_y2 = getInfluenceFactor(mlc_positioning_y2);

    // return std::pow((influence_factor_y1*influence_factor_y2),1/120.0);
    return influence_factor_y1+influence_factor_y2;
}

////////////////////////////////////////////////////////////////////////////////
/// 
void ControlPoint::FillEventCollections(G4HCofThisEvent* evtHC){
    for(const auto& run_collection: ControlPoint::m_run_collections){
        // // LOGSVC_DEBUG("RunAnalysis::EndOfEvent: RunColllection {}",run_collection.first);
        for(const auto& hc: run_collection.second){
            // Related SensitiveDetector collection ID (Geant4 architecture)
            auto collID = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
            // collID==-1 the collection is not found
            // collID==-2 the collection name is ambiguous
            if(collID<0){
                // LOGSVC_INFO("ControlPoint::FillEventCollections: HC: {} / G4SDManager Err: {}", hc, collID);
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
///
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
///
void ControlPoint::RegisterRunHCollection(const G4String& run_collection_name, const G4String& hc_name){
    if(m_run_collections.find( run_collection_name ) == m_run_collections.end()){
        m_run_collections[run_collection_name] = std::vector<G4String>();
        // LOGSVC_INFO("Register new run collection:  {} ", run_collection_name);
    }
    m_run_collections.at(run_collection_name).emplace_back(hc_name);
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4String> ControlPoint::GetRunCollectionNames() {
    std::vector<G4String> run_collection_names;
    for(const auto& run_collection: ControlPoint::m_run_collections){
        // G4cout << "RunCollection: " << run_collection.first << G4endl;
        run_collection_names.emplace_back(run_collection.first);
    }
    return run_collection_names;
}

////////////////////////////////////////////////////////////////////////////////
///
std::set<G4String> ControlPoint::GetHitCollectionNames() {
    static std::set<G4String> hit_collection_names;
        if(hit_collection_names.empty()){
        for(const auto& run_collection: ControlPoint::m_run_collections){
            const auto& rc_hcs = run_collection.second;
            hit_collection_names.insert(rc_hcs.begin(), rc_hcs.end());
        }
    }
    return hit_collection_names;
}

VMlc* ControlPoint::MLC() const {
    // G4cout << "ControlPoint::MLC:: #{} CP" << Id() << G4endl;
    auto mlc = Service<GeoSvc>()->MLC();
    if(!mlc->Initialized(this)){
        // LOGSVC_ERROR("ControlPoint::MLC:Initialization failed for #{} CP", Id());
        std::exit(EXIT_FAILURE);
    }
    return mlc;
}

////////////////////////////////////////////////////////////////////////////////
///
const std::vector<double>& ControlPoint::GetMlcPositioning(const std::string& side) const {
    auto dicomSvc = DicomSvc::GetInstance();
    if(side=="Y1"){
        return m_mlc_a_positioning;
    }
    else if(side=="Y2"){
        return m_mlc_b_positioning;
    }
    else{
        // LOGSVC_ERROR("ControlPoint::GetMlcPositioning: Unknown side: {}", side);
        std::exit(EXIT_FAILURE);
    }
    return m_mlc_a_positioning; // never reached, prevent warning
}

////////////////////////////////////////////////////////////////////////////////
///
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
        // LOGSVC_ERROR("ControlPoint::GetJawAperture: Unknown side: {}", side);
        std::exit(EXIT_FAILURE);
    }
    return 0.; // never reached, prevent warning
}




