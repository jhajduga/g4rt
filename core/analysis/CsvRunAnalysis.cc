#include "CsvRunAnalysis.hh"
#include "Services.hh"
#include "ControlPoint.hh"
#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of CsvRunAnalysis.
 *
 * Ensures only one instance of CsvRunAnalysis exists throughout the application.
 *
 * @return Pointer to the singleton CsvRunAnalysis instance.
 */
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports dose scoring data from a run to CSV files.
 *
 * For each scoring collection and scoring type in the current control point, writes detailed dose information for each voxel or volume hit to a CSV file. The output includes metadata such as field area and gravity center, as well as per-hit data: label, cell IDs, voxel IDs (if applicable), spatial coordinates, dose, and scaling factors. Output files are named according to the collection and scoring type.
 */
void CsvRunAnalysis::WriteDoseToCsv(const G4Run* runPtr){
    auto writeVolumeHitDataRaw = [](std::ofstream& file, const VoxelHit& hit, bool voxelised){
        auto label = hit.GetLabel();
        auto cxId = hit.GetGlobalID(0);
        auto cyId = hit.GetGlobalID(1);
        auto czId = hit.GetGlobalID(2);
        auto vxId = hit.GetID(0);
        auto vyId = hit.GetID(1);
        auto vzId = hit.GetID(2);
        auto volume_centre = hit.GetCentre();
        auto dose = hit.GetDose();
        auto fsf = hit.GetFieldScalingFactor();
        auto asf = hit.GetAngleScalingFactor();
        file <<label<<"," <<cxId<<","<<cyId<<","<<czId;
        if(voxelised)
            file <<","<<vxId<<","<<vyId<<","<<vzId;
        file <<","<<volume_centre.getX()<<","<<volume_centre.getY()<<","<<volume_centre.getZ();
        file <<","<<dose<<","<< fsf << ","<< asf << std::endl;
    };

    auto cp = Service<RunSvc>()->CurrentControlPoint();
    const auto& scoring_maps = cp->GetRun()->GetScoringCollections();
    
    ANA_INFO("CsvRunAnalysis::WriteDoseToCsv #{} collections:",scoring_maps.size());

    for(auto& scoring_map: scoring_maps){
        
        ANA_INFO("CsvRunAnalysis::WriteDoseToCsv for {} run collection:",scoring_map.first);
        for(auto& scoring: scoring_map.second){
            auto scoring_type = scoring.first;
            auto& data = scoring.second;
            auto type_str = svc::tolower(Scoring::to_string(scoring_type));
            auto coll_str = svc::tolower(scoring_map.first);
            auto file = cp->GetOutputFileName()+"_"+coll_str+"_"+type_str+".csv";
            std::ofstream c_outFile;
            c_outFile.open(file.c_str(), std::ios::out);
            // Control Point Meta data:
            c_outFile << "# FieldArea: " + std::to_string(cp->GetRun()->GetBeamMaskArea()) << std::endl;
            auto beam_grav_centre = cp->GetRun()->GetBeamMaskeGravCentre();
            c_outFile << "# FieldGravCentre: "+std::to_string(beam_grav_centre.first)+","+std::to_string (beam_grav_centre.second) << std::endl;
            std::string header = "Label,Cell IdX,Cell IdY,Cell IdZ,X [mm],Y [mm],Z [mm],Dose [Gy],FieldScalingFactor,AngleScalingFactor";
            if(scoring_type==Scoring::Type::Voxel)
                header = "Label,Cell IdX,Cell IdY,Cell IdZ,Voxel IdX,Voxel IdY,Voxel IdZ,X [mm],Y [mm],Z [mm],Dose [Gy],FieldScalingFactor,AngleScalingFactor";
            c_outFile << header << std::endl;
            for(auto& scoring : data){
                writeVolumeHitDataRaw(c_outFile, scoring.second, scoring_type==Scoring::Type::Voxel);
            }
            c_outFile.close();
            
        ANA_INFO("Output file closed: {}",file);
        }
        
        ANA_INFO("CsvRunAnalysis::WriteDoseToCsv for {} run collection - done!",scoring_map.first);
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports field mask data for each data type to CSV files and generates corresponding PNG images.
 *
 * For each available data type in the current control point, writes the 3D coordinates of the field mask points to a CSV file. After writing, generates a PNG image of the field mask using an embedded Python module.
 *
 * @param runPtr Pointer to the current Geant4 run (used for context).
 */
void CsvRunAnalysis::WriteFieldMaskToCsv(const G4Run* runPtr){
    auto cp = Service<RunSvc>()->CurrentControlPoint();
    auto data_types = cp->DataTypes();
    for(const auto& type : data_types){
        
        ANA_INFO("Writing field mask (type={}) to CSV...",type);
        const auto& field_mask = cp->GetFieldMask(type);
        if(field_mask.size()>0){
            auto file = cp->GetOutputFileName()+"_field_mask_"+svc::tolower(type)+".csv";
            std::string header = "X [mm],Y [mm],Z [mm]";
            std::ofstream c_outFile;
            c_outFile.open(file.c_str(), std::ios::out);
            c_outFile << header << std::endl;
            for(auto& mp : field_mask)
                c_outFile << mp.getX() << "," << mp.getY() << "," << mp.getZ() << std::endl;
            c_outFile.close();
            
        ANA_INFO("Writing Field Mask to file {} - done!",file);
            auto writePngCopy = py::module::import("field_mask_png");
            writePngCopy.attr("save_mask_as_png")(file);
        }
    }
}


