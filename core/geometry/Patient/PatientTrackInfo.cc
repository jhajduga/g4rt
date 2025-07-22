#include "PatientTrackInfo.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4Step.hh"

////////////////////////////////////////////////////////////////////////////////
///
G4ThreadLocal G4Allocator<PatientTrackInfo>* aPatientTrackInfoAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
///
void PatientTrackInfo::Print() const { }

////////////////////////////////////////////////////////////////////////////////
///
void PatientTrackInfo::FillInfo(G4Step* aStep){
    if(m_trackingStatus==1){
        m_trackLength += aStep->GetStepLength();
    }
    else {
        m_trackingStatus = 1;
    }
}



