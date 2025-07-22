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
 * @brief Prepares analysis at the start of a Geant4 simulation run.
 *
 * Retrieves the current control point and logs the thread type (master or worker) at the beginning of the run. Scoring collection initialization is handled elsewhere.
 *
 * @param runPtr Pointer to the current Geant4 run.
 * @param isMaster Indicates if the current thread is the master thread.
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
 * @brief Finalizes analysis and writes output data at the end of a Geant4 run.
 *
 * Triggers end-of-run processing for the current control point, then writes dose and field mask data to CSV and/or ROOT files depending on the initialized analysis modules and configuration settings. Optionally exports dose data in CT format if enabled.
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