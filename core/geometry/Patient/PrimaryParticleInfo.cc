#include "PrimaryParticleInfo.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4PrimaryParticle.hh"

////////////////////////////////////////////////////////////////////////////////
///
G4ThreadLocal G4Allocator<PrimaryParticleInfo>* aPrimaryParticleInfoAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
///
void PrimaryParticleInfo::Print() const { }

////////////////////////////////////////////////////////////////////////////////
///
void PrimaryParticleInfo::FillInfo(G4PrimaryParticle* pparticle){
    m_initial_total_energy = pparticle->GetTotalEnergy() / keV;
}



