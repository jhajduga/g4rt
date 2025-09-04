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
 * Returns a pointer to a Meyers-style singleton; the instance is created on first call
 * and lives for the lifetime of the program.
 *
 * @return CsvRunAnalysis* Pointer to the single CsvRunAnalysis instance.
 */
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export dose-scoring data from a run to CSV files.
 *
 * For each scoring collection and scoring type in the current control point, writes one CSV file containing
 * per-hit dose data and a small set of run-level metadata. Each CSV contains a header and one row per
 * VoxelHit (or volume hit) with the following fields:
 * - Label, Cell IdX, Cell IdY, Cell IdZ
 * - (if voxelized) Voxel IdX, Voxel IdY, Voxel IdZ
 * - X [mm], Y [mm], Z [mm], Dose [Gy], FieldScalingFactor, AngleScalingFactor
 *
 * Output files are created using the control point's output file base name with suffixes for the collection
 * name and scoring type: "<outputBase>_<collection>_<type>.csv". Two run-level metadata comment lines are
 * written at the top: FieldArea and FieldGravCentre.
 *
 * Side effects: creates and writes one CSV file per scoring collection/type.
 *
 * @param runPtr Pointer to the G4Run to export dose data for (used in conjunction with the current control point).
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
 * @brief Export field-mask points to CSV and generate PNG visualizations.
 *
 * Writes the 3D coordinates (X,Y,Z in mm) of the control point's field mask for each available data type
 * to a CSV file named "<outputFileName>_field_mask_<type>.csv". After a CSV is written, the function imports
 * the Python module "field_mask_png" and calls its `save_mask_as_png(file)` function to produce a PNG
 * representation of the mask.
 *
 * Side effects:
 * - Creates one CSV file per non-empty field mask type.
 * - Invokes embedded Python (pybind11) to generate a PNG from each CSV; this requires the Python module
 *   "field_mask_png" to be available at runtime.
 *
 * @param runPtr Pointer to the current Geant4 run (provided for API/context; not used by this implementation).
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


