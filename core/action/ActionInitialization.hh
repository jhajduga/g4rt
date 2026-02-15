/**
*
* \author B. Rachwal
* \date 05.05.2020
*
*/

#ifndef Dose3D_ACTIONINITIALIZATION_HH
#define Dose3D_ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

///\class ActionInitialization
class ActionInitialization : public G4VUserActionInitialization {
  public:
  /**
 * @brief Default constructor for ActionInitialization.
 *
 * Creates an ActionInitialization instance with the class's default settings.
 * No runtime initialization is performed here; action setup occurs in Build()/BuildForMaster().
 */
  ActionInitialization() = default;

  /**
 * @brief Destroys the ActionInitialization instance.
 *
 * Default virtual destructor for proper cleanup in derived classes.
 */
  virtual ~ActionInitialization() = default;

  ///
  void BuildForMaster() const override;

  ///
  void Build() const override;
};

#endif // Dose3D_ACTIONINITIALIZATION_HH
