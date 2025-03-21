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
#include "LogSvc.hpp"
#ifdef G4MULTITHREADED
  #include "G4MTRunManager.hh"
#endif
#include "PatientGeometry.hh"

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
///
RunAnalysis *RunAnalysis::GetInstance() {
    static RunAnalysis instance = RunAnalysis();
    return &instance;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes run analysis at the beginning of a run.
 *
 * Retrieves the current control point from the run service and prints a message indicating
 * whether the current thread is a worker or a master thread. This helps in distinguishing
 * thread-specific behavior during multi-threaded analysis runs.
 *
 * @param runPtr Pointer to the current run instance (unused in the current implementation).
 * @param isMaster Boolean flag indicating if the current thread is the master (unused in the current implementation).
 */
void RunAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
    m_current_cp = Service<RunSvc>()->CurrentControlPoint();
    std::string worker = G4Threading::IsWorkerThread() ? "*WORKER*" : " *MASTER* ";
    // LOGSVC_INFO("Message from {} thread.", worker);
    std::cout << "Message from " << worker << std::endl;
    // LOGSVC_DEBUG("RunAnalysis:: begin of run at {} thread.",worker);
    // Note: Everything is being care by ControlPointRun::InitializeScoringCollection
}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from EventAction::EndOfEventAction
void RunAnalysis::EndOfEventAction(const G4Event *evt){
    auto hCofThisEvent = evt->GetHCofThisEvent();
    m_current_cp->FillEventCollections(hCofThisEvent);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finalizes analysis for the simulation run.
 *
 * Ends the current control point run and, depending on the active analysis instances,
 * writes dose and field mask data to CSV or TFile formats. If CSV analysis is enabled,
 * dose and field mask data are written to CSV files and, if configured, CT dose data is exported.
 * Similarly, if NTuple analysis is active, dose and field mask data are written to TFile format.
 *
 * @param runPtr Pointer to the current Geant4 run instance.
 */
void RunAnalysis::EndOfRun(const G4Run* runPtr){
    // LOGSVC_INFO("RunAnalysis::EndOfRun:: CtrlPoint-{} / G4Run-{}", m_current_cp->GetId(), runPtr->GetRunID());
    // Note: Multithreading merging is being performed before...
    m_current_cp->GetRun()->EndOfRun();
    if(m_csv_run_analysis){
        m_csv_run_analysis->WriteDoseToCsv(runPtr);
        m_csv_run_analysis->WriteFieldMaskToCsv(runPtr);
        if(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "GenerateCT"))
            PatientGeometry::GetInstance()->ExportDoseToCsvCT(runPtr);
    }

    if(m_ntuple_run_analysis){
        m_ntuple_run_analysis->WriteDoseToTFile(runPtr);
        m_ntuple_run_analysis->WriteFieldMaskToTFile(runPtr);
    }
}