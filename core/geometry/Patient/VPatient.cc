//
// Created by brachwal on 30.04.2020.
//

#include "VPatient.hh"
#include "VPatientSD.hh"
#include "WorldConstruction.hh"
#include "G4SDManager.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
void VPatient::SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr){
  if(m_patientSD.Get()==0) // NOTE: this should be checked already from the caller!
    m_patientSD.Put(sensitiveDetectorPtr);
  // LOGSVC_INFO("Setting Sensitive Detector ({}) for {}",sensitiveDetectorPtr->GetName(),logicalVName);
  G4SDManager::GetSDMpointer()->AddNewDetector(m_patientSD.Get());
  Service<GeoSvc>()->World()->SetSensitiveDetector(logicalVName, m_patientSD.Get());
}