void drawGeoFromGDML(TGeoManager *geom, TString fname) {
  TGeoVolume *top = geom->GetTopVolume();
  TCanvas *c1 = new TCanvas("c1","canvas",700,700);
  top->Draw("ogl");
  c1->SaveAs(fname+".pdf");
}


int checkOverlaps(TGeoManager *geom, bool fullGeo) {
  cout << "\n\n== check overlap ==\n\n" << endl;
  geom->CheckOverlaps();
  TObjArray *overlapsList = geom->GetListOfOverlaps();
  int numberOfOverlaps = 0; 
  for (int numberOfOverlaps = 0; numberOfOverlaps < overlapsList->GetEntries(); ++numberOfOverlaps) {
    overlapsList->At(numberOfOverlaps)->Print();
    cout << "== ... ==" << endl;
  }
  cout << "\n\n== CheckGeometryFull() ==\n\n" << endl;
  if (fullGeo)
    geom->CheckGeometryFull();
  return overlapsList->GetEntries();
}


int testGeo(TString fname="WorldGeometry") {
  TGeoManager *geom;
  if (!gSystem->AccessPathName( fname+".gdml" ))
    geom = TGeoManager::Import(fname+".gdml");
  else {
    cout << fname+".gdml" + " does not exist" << endl;
    return 1;
  }
  drawGeoFromGDML(geom,fname);
  return checkOverlaps(geom,false);
}
