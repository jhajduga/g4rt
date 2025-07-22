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
///
void RunAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
    m_current_cp = Service<RunSvc>()->CurrentControlPoint();
    std::string worker = G4Threading::IsWorkerThread() ? "*WORKER*" : " *MASTER* ";
    ANA_DEBUG("RunAnalysis:: begin of run at {} thread.",worker);
    // Note: Everything is being care by ControlPointRun::InitializeScoringCollection
}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from EventAction::EndOfEventAction
void RunAnalysis::EndOfEventAction(const G4Event *evt){
    auto hCofThisEvent = evt->GetHCofThisEvent();
    m_current_cp->FillEventCollections(hCofThisEvent);
}

////////////////////////////////////////////////////////////////////////////////
///
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