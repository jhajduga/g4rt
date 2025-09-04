//
// Created by brachwal on 28.04.2020.
//

#include "RunAnalysis.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "VoxelHit.hh"
#include "G4AnalysisManager.hh"
#include "CsvRunAnalysis.hh"
#include "NTupleRunAnalysis.hh"
#include "Services.hh"
#ifdef G4MULTITHREADED
  #include "G4MTRunManager.hh"
#endif
#include "PatientGeometry.hh"

/**
 * @brief Initializes analysis modules for the simulation run.
 *
 * Ensures singleton instances of CSV and NTuple analysis modules are created if they have not been initialized. This setup is performed only once per process.
 */
RunAnalysis::RunAnalysis(){
  if(!m_is_initialized){
    if(!m_csv_run_analysis) // TODO: && RUN_CSV_ANALYSIS
        m_csv_run_analysis = CsvRunAnalysis::GetInstance();
    if(!m_ntuple_run_analysis) // TODO: && RUN_NTUPLE_ANALYSIS
        m_ntuple_run_analysis = NTupleRunAnalysis::GetInstance();
    // TODO: RUN_HDF5_ANALYSIS
  }
  m_is_initialized = true;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the RunAnalysis class.
 *
 * Ensures that only one instance of RunAnalysis exists throughout the application's lifetime.
 *
 * @return Pointer to the global RunAnalysis instance.
 */
RunAnalysis *RunAnalysis::GetInstance() {
    static RunAnalysis instance = RunAnalysis();
    return &instance;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize analysis at the start of a Geant4 run.
 *
 * Retrieves and stores the current control point from RunSvc and logs whether this invocation runs on the master or a worker thread.
 * Scoring-collection setup is performed separately by ControlPointRun::InitializeScoringCollection.
 *
 * @param runPtr Pointer to the current Geant4 run.
 * @param isMaster True if called on the master thread.
 */
void RunAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
    m_current_cp = Service<RunSvc>()->CurrentControlPoint();
    std::string worker = G4Threading::IsWorkerThread() ? "*WORKER*" : " *MASTER* ";
    ANA_DEBUG("RunAnalysis:: begin of run at {} thread.",worker);
    // Note: Everything is being care by ControlPointRun::InitializeScoringCollection
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Processes event data at the end of each simulation event.
 *
 * Retrieves the hit collections from the given event and passes them to the current control point for event-specific analysis and data accumulation.
 *
 * @param evt Pointer to the Geant4 event whose data will be processed.
 */
void RunAnalysis::EndOfEventAction(const G4Event *evt){
    auto hCofThisEvent = evt->GetHCofThisEvent();
    m_current_cp->FillEventCollections(hCofThisEvent);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finalize analysis for a completed Geant4 run and write configured outputs.
 *
 * Performs end-of-run processing on the current control point's Run, then writes dose and
 * optional field-mask outputs to the configured analysis backends. If CSV analysis is
 * active, writes per-run dose (and, if enabled, field masks) to CSV and — when CT export
 * is enabled — exports CT-formatted dose via PatientGeometry. If NTuple (ROOT) analysis is
 * active, writes dose and field-mask data to the ROOT/NTuple file.
 *
 * Configuration-controlled behaviors:
 * - RunSvc.WriteFieldMaskToCsv (bool): when true, write field masks to CSV.
 * - RunSvc.GenerateCT (bool): when true, export CT-format dose via PatientGeometry.
 *
 * Side effects:
 * - Calls m_current_cp->GetRun()->EndOfRun() to finalize run-level accumulation.
 * - May create/modify files via CSV and NTuple backends and PatientGeometry export.
 *
 * @param runPtr Pointer to the completed Geant4 run.
 */
void RunAnalysis::EndOfRun(const G4Run* runPtr){
    ANA_INFO("RunAnalysis::EndOfRun:: CtrlPoint-{} / G4Run-{}", m_current_cp->GetId(), runPtr->GetRunID());
    // Note: Multithreading merging is being performed before...
    m_current_cp->GetRun()->EndOfRun();
    if(m_csv_run_analysis){
        m_csv_run_analysis->WriteDoseToCsv(runPtr);
        if(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "WriteFieldMaskToCsv"))
            m_csv_run_analysis->WriteFieldMaskToCsv(runPtr);
        if(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "GenerateCT"))
            PatientGeometry::GetInstance()->ExportDoseToCsvCT(runPtr);
    }

    if(m_ntuple_run_analysis){
        m_ntuple_run_analysis->WriteDoseToTFile(runPtr);
        m_ntuple_run_analysis->WriteFieldMaskToTFile(runPtr);
    }
}