/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \author J.Hajduga (jhajduga@agh.edu.pl)
* \date 29.01.2025
*
*/

#ifndef D3DF_IbaImRT
#define D3DF_IbaImRT

#include "IPhysicalVolume.hh"
#include "G4ThreeVector.hh"

class IbaImRT: public IPhysicalVolume {
    private:
        ///
        IbaImRT();

        /**
 * @brief Destroys the IbaImRT singleton instance.
 */
        ~IbaImRT() = default;

        /**********************************************************************************
 * @brief Deleted copy constructor to prevent copying of the IbaImRT singleton.
 *********************************************************************************/
        IbaImRT(const IbaImRT &) = delete;
        /**
 * @brief Deleted copy assignment operator to prevent copying of the singleton instance.
 */
IbaImRT &operator=(const IbaImRT &) = delete;
        /**
 * @brief Move constructor is deleted to prevent moving of the singleton instance.
 */
IbaImRT(IbaImRT &&) = delete;
        /**
 * @brief Move assignment operator is deleted to enforce singleton behavior.
 *
 * Prevents moving assignment of the IbaImRT instance.
 */
IbaImRT &operator=(IbaImRT &&) = delete;

    public:
        /// 
        static IbaImRT *GetInstance();

        ///
        void Construct(G4VPhysicalVolume *parentPV) override;

        /**
 * @brief Destroys the physical volume.
 *
 * This implementation does nothing.
 */
        void Destroy() override {}

        /**
 * @brief Indicates that the IbaImRT volume is always up to date.
 *
 * @return G4bool Always returns true.
 */
        G4bool Update() override { return true; }

        /**
 * @brief Resets the state of the IbaImRT volume.
 *
 * This implementation performs no action.
 */
        void Reset() override {}

        ///
        void WriteInfo() override {};

        /// Static member variable for translation to Iba Origin
        static G4ThreeVector IbaToLocalTranslation;

};


#endif //D3DF_IbaImRT