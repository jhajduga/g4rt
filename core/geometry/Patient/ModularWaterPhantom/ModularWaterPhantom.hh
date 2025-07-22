#ifndef D3DF_MODULARWATERPHANTOM
#define D3DF_MODULARWATERPHANTOM

#include "IPhysicalVolume.hh"
#include "G4ThreeVector.hh"

class ModularWaterPhantom: public IPhysicalVolume {
    private:
        ///
        ModularWaterPhantom();

        ///
        ~ModularWaterPhantom() = default;

        /// Delete the copy and move constructors
        ModularWaterPhantom(const ModularWaterPhantom &) = delete;
        ModularWaterPhantom &operator=(const ModularWaterPhantom &) = delete;
        ModularWaterPhantom(ModularWaterPhantom &&) = delete;
        ModularWaterPhantom &operator=(ModularWaterPhantom &&) = delete;

    public:
        /// 
        static ModularWaterPhantom *GetInstance();

        ///
        void Construct(G4VPhysicalVolume *parentPV) override;

        ///
        void Destroy() override {}

        ///
        G4bool Update() override { return true; }

        ///
        void Reset() override {}

        ///
        void WriteInfo() override {};

        ///
        void SetRotation(G4RotationMatrix* rotation) { m_rotation = rotation; }

        ///
        G4RotationMatrix* GetRotation() const { return m_rotation; }

        ///
        G4RotationMatrix* m_rotation;

};


#endif //D3DF_MODULARWATERPHANTOM