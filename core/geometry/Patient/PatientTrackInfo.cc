#include "PatientTrackInfo.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4Step.hh"

////////////////////////////////////////////////////////////////////////////////
///
G4ThreadLocal G4Allocator<PatientTrackInfo>* aPatientTrackInfoAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prints the tracking information for the patient.
 *
 * This method is intended to output the current tracking information. The implementation is currently empty.
 */
void PatientTrackInfo::Print() const { }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates tracking status and accumulates track length from the given step.
 *
 * If tracking is already active, adds the step length from `aStep` to the total track length. Otherwise, activates tracking.
 */
void PatientTrackInfo::FillInfo(G4Step* aStep){
    if(m_trackingStatus==1){
        m_trackLength += aStep->GetStepLength();
    }
    else {
        m_trackingStatus = 1;
    }
}



