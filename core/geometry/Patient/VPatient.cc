//
// Created by brachwal on 30.04.2020.
//

#include "VPatient.hh"
#include "VPatientSD.hh"
#include "WorldConstruction.hh"
#include "G4SDManager.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Associates a sensitive detector with a specified logical volume.
 *
 * This function registers a provided sensitive detector with the simulation environment. If no sensitive detector
 * is already assigned, it sets the detector, adds it to the G4SDManager, and associates it with the indicated 
 * logical volume via the GeoSvc service.
 *
 * @param logicalVName Name of the logical volume for which the sensitive detector is set.
 * @param sensitiveDetectorPtr Pointer to the sensitive detector instance.
 */
void VPatient::SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr){
  if(m_patientSD.Get()==0) // NOTE: this should be checked already from the caller!
    m_patientSD.Put(sensitiveDetectorPtr);
  // LOGSVC_INFO("Setting Sensitive Detector ({}) for {}",sensitiveDetectorPtr->GetName(),logicalVName);
  G4SDManager::GetSDMpointer()->AddNewDetector(m_patientSD.Get());
  Service<GeoSvc>()->World()->SetSensitiveDetector(logicalVName, m_patientSD.Get());
}