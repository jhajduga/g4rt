#include "PrimaryParticleInfo.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4PrimaryParticle.hh"

////////////////////////////////////////////////////////////////////////////////
///
G4ThreadLocal G4Allocator<PrimaryParticleInfo>* aPrimaryParticleInfoAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Print primary particle information.
 *
 * @note This function is currently a no-op (placeholder) and does not produce output.
 */
void PrimaryParticleInfo::Print() const { }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Record the primary particle's initial total energy (in keV).
 *
 * Retrieves the particle's total energy from the given G4PrimaryParticle, converts it to kiloelectronvolts
 * (by dividing by the `keV` unit), and stores the result in the member variable `m_initial_total_energy`.
 *
 * @param pparticle Pointer to the primary particle whose total energy will be recorded.
 *                  Must be non-null; the function does not perform a null-pointer check.
 */
void PrimaryParticleInfo::FillInfo(G4PrimaryParticle* pparticle){
    m_initial_total_energy = pparticle->GetTotalEnergy() / keV;
}


