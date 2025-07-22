
#include "stdio.h"

void time_dose_accumulate(){

    Double_t gantryDoseDelivery = 0.0166; // Gy / s

    TFile *f = new TFile("run-0-analysis.root");
    TTree *tree = (TTree*)f->Get("WaterPhantomTree");

    Double_t evt_time, total_time, total_dose;
    std::vector<Double_t>* evt_voxel_dose = nullptr;
    Int_t evt_nHits;

    tree->SetBranchAddress("LocalTime",&evt_time);
    tree->SetBranchAddress("VoxelDose",&evt_voxel_dose);
    tree->SetBranchAddress("nHits",&evt_nHits);

    // Double_t mc_to_real_time_coeff = 1.;
    Double_t mc_to_real_time_coeff = 18.;


    total_time=0.;
    total_dose=0.;
    Long64_t nentries = tree->GetEntries();
    std::cout<< " Cummulated Dose | Evt no | Cummulated time " <<std::endl; //
    for (Long64_t i=0;i<nentries;i++) {
      tree->GetEntry(i);
      total_time+=evt_time * mc_to_real_time_coeff;
      Double_t evt_dose =0.;
      for(Int_t ih = 0; ih < evt_nHits; ++ih){
          evt_dose+=evt_voxel_dose->at(ih);
      }
      total_dose+=evt_dose;
      if(i%100000==0){
        std::cout<< total_dose << " " << i << " " << total_time <<std::endl; //

      }
    }

    std::cout<< "Total simualted evt no.: " << nentries << std::endl; //
    std::cout<< "Total simualted time:    " << total_time << " ns" << std::endl; //

}