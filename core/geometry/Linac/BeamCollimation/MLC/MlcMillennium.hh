//
// Created by brachwal on 25.08.2020.
//

#ifndef DOSE3D_VARIANMLCMILLENIUM_HH
#define DOSE3D_VARIANMLCMILLENIUM_HH

class MlcMillennium {
  private:

  public:
      /**
 * @brief Default constructor is deleted to prevent instantiation without a parent physical volume.
 */
      MlcMillennium() = delete;

      /**
 * @brief Construct an MlcMillennium associated with a parent physical volume.
 *
 * Constructs an MlcMillennium instance tied to the specified parent G4VPhysicalVolume.
 *
 * @param parentPV Pointer to the parent G4VPhysicalVolume with which this object is associated.
 */
      MlcMillennium(G4VPhysicalVolume* parentPV){};

      /**
 * @brief Destroys the MlcMillennium instance.
 */
      ~MlcMillennium() = default;

};
#endif //DOSE3D_VARIANMLCMILLENIUM_HH
