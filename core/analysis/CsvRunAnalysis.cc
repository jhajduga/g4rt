#include "CsvRunAnalysis.hh"
#include "Services.hh"
#include "ControlPoint.hh"
#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;


////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Writes simulation dose data to CSV files.
 *
 * This method retrieves the current run's scoring collections from the control point and writes each collection's dose data into corresponding CSV files.
 * The output file name is constructed using the control point's base name, the collection type, and the scoring type.
 * For voxel-based scoring, additional columns for voxel identifiers are included in the CSV header and data.
 * Note that the provided run pointer is not used directly; the active run is obtained via the run service.
 *
 * @param runPtr Pointer to the simulation run (currently unused).
 */
void CsvRunAnalysis::WriteDoseToCsv(const G4Run* runPtr){
    auto writeVolumeHitDataRaw = [](std::ofstream& file, const VoxelHit& hit, bool voxelised){
        auto cxId = hit.GetGlobalID(0);
        auto cyId = hit.GetGlobalID(1);
        auto czId = hit.GetGlobalID(2);
        auto vxId = hit.GetID(0);
        auto vyId = hit.GetID(1);
        auto vzId = hit.GetID(2);
        auto volume_centre = hit.GetCentre();
        auto dose = hit.GetDose();
        auto inField = hit.GetFieldScalingFactor();
        file <<cxId<<","<<cyId<<","<<czId;
        if(voxelised)
            file <<","<<vxId<<","<<vyId<<","<<vzId;
        file <<","<<volume_centre.getX()<<","<<volume_centre.getY()<<","<<volume_centre.getZ();
        file <<","<<dose<<","<< inField << std::endl;
    };

    auto cp = Service<RunSvc>()->CurrentControlPoint();
    const auto& scoring_maps = cp->GetRun()->GetScoringCollections();
    // LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv #{} collections:",scoring_maps.size());

    for(auto& scoring_map: scoring_maps){
        // LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv for {} run collection:",scoring_map.first);
        for(auto& scoring: scoring_map.second){
            auto scoring_type = scoring.first;
            auto& data = scoring.second;
            auto type_str = svc::tolower(Scoring::to_string(scoring_type));
            auto coll_str = svc::tolower(scoring_map.first);
            auto file = cp->GetOutputFileName()+"_"+coll_str+"_"+type_str+".csv";
            std::string header = "Cell IdX,Cell IdY,Cell IdZ,X [mm],Y [mm],Z [mm],Dose [Gy],FieldScalingFactor";

            if(scoring_type==Scoring::Type::Voxel)
                header = "Cell IdX,Cell IdY,Cell IdZ,Voxel IdX,Voxel IdY,Voxel IdZ,X [mm],Y [mm],Z [mm],Dose [Gy],FieldScalingFactor";

            std::ofstream c_outFile;
            c_outFile.open(file.c_str(), std::ios::out);
            c_outFile << header << std::endl;
            for(auto& scoring : data){
                writeVolumeHitDataRaw(c_outFile, scoring.second, scoring_type==Scoring::Type::Voxel);
            }
            c_outFile.close();
            // LOGSVC_INFO("Output file closed: {}",file);
        }
        // LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv for {} run collection - done!",scoring_map.first);
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports field mask data to CSV files and generates PNG copies.
 *
 * This method retrieves the current control point and iterates through each available data type.
 * For data types with a non-empty field mask, it creates a CSV file containing the 3D coordinates
 * (X, Y, Z) for each point and then invokes a Python module to generate a corresponding PNG file.
 *
 * @param runPtr Pointer to the current simulation run (currently unused).
 */
void CsvRunAnalysis::WriteFieldMaskToCsv(const G4Run* runPtr){
    auto cp = Service<RunSvc>()->CurrentControlPoint();
    auto data_types = cp->DataTypes();
    for(const auto& type : data_types){
        // LOGSVC_INFO("Writing field mask (type={}) to CSV...",type);
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
            // LOGSVC_INFO("Writing Field Mask to file {} - done!",file);
            auto writePngCopy = py::module::import("field_mask_png");
            writePngCopy.attr("save_mask_as_png")(file);
        }
    }
}


