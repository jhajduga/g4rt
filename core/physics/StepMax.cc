#include "StepMax.hh"



/**
 * @brief Constructs a StepMax process with the specified name and sets the maximum charged particle step length to the largest possible value.
 *
 * @param processName Name of the process.
 */
StepMax::StepMax(const G4String& processName)
: G4VDiscreteProcess(processName),MaxChargedStep(DBL_MAX){}



/**
 * @brief Destroys the StepMax process object.
 *
 * Default destructor with no additional cleanup.
 */
StepMax::~StepMax() {}



/**
 * @brief Determines whether this process applies to the given particle.
 *
 * @param particle Particle definition to test.
 * @return `true` if the particle has nonzero electric charge, `false` otherwise.
 */
G4bool StepMax::IsApplicable(const G4ParticleDefinition& particle) 
{ 
    return (particle.GetPDGCharge() != 0.);
}


    
/**
 * @brief Configure the maximum step length applied to charged particles.
 *
 * Sets the internal MaxChargedStep used by the process to limit proposed step lengths.
 *
 * @param step Maximum allowed step length for charged particles.
 */
void StepMax::SetMaxStep(G4double step) {MaxChargedStep = step;}



/**
 * @brief Provides the advisory maximum step length for charged particles.
 *
 * Sets *condition to NotForced and returns the configured maximum charged-particle step length.
 *
 * @param condition Output pointer set to NotForced.
 * @return The configured maximum step length for charged particles.
 */
G4double StepMax::PostStepGetPhysicalInteractionLength(const G4Track&,
                                                G4double,
                                                G4ForceCondition* condition )
{
    // condition is set to "Not Forced"
    *condition = NotForced;

    return MaxChargedStep;
}



/**
 * @brief Initialize and return the particle change for the current track without modifying the particle.
 *
 * Initializes the internal G4VParticleChange with the provided track and returns a pointer to it; no particle properties are altered.
 *
 * @return G4VParticleChange* Pointer to the initialized particle change object.
 */
G4VParticleChange* StepMax::PostStepDoIt(const G4Track& aTrack, const G4Step&)
{
    // do nothing
    aParticleChange.Initialize(aTrack);
    return &aParticleChange;
}