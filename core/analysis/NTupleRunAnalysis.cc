#include "NTupleRunAnalysis.hh"
#include "Services.hh"
#include "ControlPoint.hh"
#include "IO.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of NTupleRunAnalysis.
 *
 * Ensures that only one instance of NTupleRunAnalysis exists throughout the application.
 *
 * @return Pointer to the singleton NTupleRunAnalysis instance.
 */
NTupleRunAnalysis *NTupleRunAnalysis::GetInstance() {
    static NTupleRunAnalysis instance = NTupleRunAnalysis();
    return &instance;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Write 3D field-mask data for the current control point into a ROOT file.
 *
 * For the current control point (obtained from the run service) this writes each non-empty
 * field mask (per data type) as a linearized vector into a ROOT file named
 * "<control-point-output>_field_mask.root" under the directory "RT_Plan/CP_<id>".
 * Each object is written with the name "FieldMask_<type>".
 *
 * @param runPtr Unused in this implementation; present for API compatibility.
 */
void NTupleRunAnalysis::WriteFieldMaskToTFile(const G4Run* runPtr){
    auto cp = Service<RunSvc>()->CurrentControlPoint();
    auto fname = cp->GetOutputFileName()+"_field_mask.root";
    auto dir_name = "RT_Plan/CP_"+std::to_string(cp->GetId());
    auto file = IO::CreateOutputTFile(fname,dir_name);
    auto cp_dir = file->GetDirectory(dir_name.c_str());
    auto data_types = cp->DataTypes();
    for(const auto& type : data_types){
        auto field_mask = cp->GetFieldMask(type);
        if(field_mask.size()>0){
            auto linearized_mask_vec = svc::linearizeG4ThreeVector(field_mask);
            std::string name = "FieldMask_"+type;
            cp_dir->WriteObject(&linearized_mask_vec,name.c_str());
            file->Write();
        }
    }
    file->Close();
    ANA_INFO("Writing Field Mask to file: {} - done!",file->GetName()); 
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export dose-volume scoring data for the current control point to a ROOT file.
 *
 * Retrieves the current control point from RunSvc and writes, for each scoring collection and
 * scoring type, three flat vectors into a ROOT file named "<control_point_output>_dose.root"
 * under the directory "RT_Plan/CP_<id>":
 *  - "<collection>_VolumeFieldScalingFactorPosition_<type>": linearized 3D positions (double vector)
 *  - "<collection>_VolumeFieldScalingFactorValue_<type>": field scaling factors (double vector)
 *  - "<collection>_Dose_<type>": dose values (double vector)
 *
 * The function linearizes each 3D position into a flat double vector before writing. The file is
 * written to disk and closed after all collections are processed.
 *
 * @param runPtr Unused: the function obtains the run via the current control point from RunSvc.
 */
void NTupleRunAnalysis::WriteDoseToTFile(const G4Run* runPtr){
    auto cp = Service<RunSvc>()->CurrentControlPoint();
    auto fname = cp->GetOutputFileName()+"_dose.root";
    auto dir_name = "RT_Plan/CP_"+std::to_string(cp->GetId());
    auto file = IO::CreateOutputTFile(fname,dir_name);
    auto cp_dir = file->GetDirectory(dir_name.c_str());
    const auto& scoring_maps = cp->GetRun()->GetScoringCollections();
    ANA_INFO("NTupleRunAnalysis::WriteDoseToTFile #{} collections:",scoring_maps.size());
    for(auto& scoring_map: scoring_maps){
        ANA_INFO("NTupleRunAnalysis::WriteDoseToTFile::Processing {} run collection:",scoring_map.first);
        for(auto& scoring: scoring_map.second){
            auto scoring_type = scoring.first;
            auto& data = scoring.second;
            auto collection = svc::tolower(scoring_map.first);
            auto type = Scoring::to_string(scoring_type);
            std::vector<double> linearized_mask_vec;
            std::vector<double> infield_tag_vec;
            std::vector<double> dose_vec;
            for(auto& scoring : data){
                auto pos = scoring.second.GetCentre();
                for(const auto& i :  svc::linearizeG4ThreeVector(pos))
                    linearized_mask_vec.emplace_back(i);
                infield_tag_vec.emplace_back(scoring.second.GetFieldScalingFactor());
                dose_vec.emplace_back(scoring.second.GetDose());
            }
            std::string name_pos = collection+"_VolumeFieldScalingFactorPosition_"+type;
            std::string name_ftag = collection+"_VolumeFieldScalingFactorValue_"+type;
            std::string name_dose = collection+"_Dose_"+type;
            cp_dir->WriteObject(&linearized_mask_vec,name_pos.c_str());
            cp_dir->WriteObject(&infield_tag_vec,name_ftag.c_str());
            cp_dir->WriteObject(&dose_vec,name_dose.c_str());
            file->Write();
        }
    }
    file->Close();
    ANA_INFO("Writing Dose Volume Data to TFile {} - done!",file->GetName());
}


