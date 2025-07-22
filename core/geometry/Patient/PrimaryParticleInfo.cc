#include "PrimaryParticleInfo.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4PrimaryParticle.hh"

////////////////////////////////////////////////////////////////////////////////
///
G4ThreadLocal G4Allocator<PrimaryParticleInfo>* aPrimaryParticleInfoAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Placeholder for printing primary particle information.
 *
 * Currently, this function does not perform any action.
 */
void PrimaryParticleInfo::Print() const { }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Stores the initial total energy of the given primary particle in keV.
 *
 * Extracts the total energy from the provided `G4PrimaryParticle` object, converts it to keV, and saves it in the member variable `m_initial_total_energy`.
 */
void PrimaryParticleInfo::FillInfo(G4PrimaryParticle* pparticle){
    m_initial_total_energy = pparticle->GetTotalEnergy() / keV;
}



