#include "D3DMLayer.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "Services.hh"

G4double D3DMLayer::COVER_WIDTH = 1.00 * mm;

////////////////////////////////////////////////////////////////////////////////
///
D3DMLayer::D3DMLayer(const G4String& label): D3DMLayer(label,"None",0){}

////////////////////////////////////////////////////////////////////////////////
///
D3DMLayer::D3DMLayer(const G4String& label, G4String cellMediumName ,G4bool shiftZ)
: VPatient(label), m_label(label), m_cell_medium_name(cellMediumName), m_mlayer_shift(shiftZ){}

////////////////////////////////////////////////////////////////////////////////
///
D3DMLayer::D3DMLayer(const G4String& label,G4String cellMediumName, const std::vector<G4ThreeVector>& cellsInLayer)
: VPatient(label), m_label(label), m_cell_medium_name(cellMediumName), m_cells_in_layer_positioning(cellsInLayer)
{
  SetNCells();
}

////////////////////////////////////////////////////////////////////////////////
///
D3DMLayer::~D3DMLayer() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::WriteInfo() {
  // TODO implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::Destroy() {
  LOGSVC_INFO("Destroing the D3DMLayer {} volume", GetName());
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DMLayer::LoadParameterization(){

    for(int iz = 0; iz < m_n_cells_in_layer_z; ++iz ){
      std::vector<G4int> column_mapping;
      for(int ix = 0; ix < m_n_cells_in_layer_x; ++ix ){
        column_mapping.push_back(1);  
      }
      m_layer_mapping.push_back(column_mapping); // row 0
    }
  return true;
}



////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::SetPosition(char axis, double pos){
  switch(std::tolower(axis)) {
    case 'x':
      m_init_x = pos;
      break;
    case 'y':
      m_init_y = pos;
      break;
    case 'z':
      m_init_z = pos;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::SetNCells(char axis, int nc){
  switch(std::tolower(axis)) {
    case 'x':
      m_n_cells_in_layer_x = nc;
      break;
    case 'z':
      m_n_cells_in_layer_z = nc;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::SetNCells(){
  m_n_cells_in_layer_x = 2;
  m_n_cells_in_layer_z = 8;
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::SetCellNVoxels(char axis, int nv){
  switch(std::tolower(axis)) {
    case 'x':
      m_cell_voxelization_x = nv;
      break;
    case 'y':
      m_cell_voxelization_y = nv;
      break;
    case 'z':
      m_cell_voxelization_z = nv;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::Construct(G4VPhysicalVolume *parentWorld) {
  // G4cout << "[INFO]:: D3DMLayer construction... " << G4endl;
  if(m_cells_in_layer_positioning.size()==0){
    LoadParameterization();
    if (m_init_x!=-999&&m_init_y!=-999&&m_init_z!=-999){
      G4cout << "D3DMLayer: \""<< GetName() << "\" translation: " << G4ThreeVector(m_init_x,m_init_y,m_init_z) << G4endl;
    } else { // 
      G4String msg = "Initial position of the layer hasn't been set properly";
      LOGSVC_CRITICAL(msg.data());
      G4Exception("D3DMLayer", "Construct", FatalErrorInArgument, msg);
    }
    G4double layer_width = D3DCell::SIZE + 2*D3DMLayer::COVER_WIDTH;
    for(int iz = 0; iz < m_n_cells_in_layer_z; ++iz ){
      auto const& row = m_layer_mapping.at(iz);
      G4double current_z = m_init_z + iz * (layer_width);
      // std::cout << "current_z = " << current_z << std::endl;
      for(int ix = 0; ix < m_n_cells_in_layer_x; ++ix ){
        // std::cout << "ix = " << ix << std::endl;
        G4double current_x = m_init_x + ix * (layer_width);
        if(m_mlayer_shift&&iz%2==1) // Assuming that the is no first level of two connection cells, TODO: otherwise if (iz%2==0)
          current_x+=(layer_width)/2.;
        G4double current_y = m_init_y;
        G4int iy = m_id;
        if (row.at(ix)){
          auto current_centre = G4ThreeVector(current_x, current_y, current_z);
          auto label = m_label+"_Cell_"+std::to_string(ix)+"_"+std::to_string(iy)+"_"+std::to_string(iz);
          // std::cout << "label = " << label << std::endl;
          m_d3d_cells.push_back(new D3DCell(label,current_centre,m_cell_medium_name));
          m_d3d_cells.back()->SetIDs(ix,iy,iz);
          m_d3d_cells.back()->SetNVoxels('x',m_cell_voxelization_x);
          m_d3d_cells.back()->SetNVoxels('y',m_cell_voxelization_y);
          m_d3d_cells.back()->SetNVoxels('z',m_cell_voxelization_z);
          // std::cout << "Cell Voxelization: " << m_cell_voxelization_x << " " << m_cell_voxelization_y << " " << m_cell_voxelization_z << std::endl;
          m_d3d_cells.back()->SetTracksAnalysis(m_tracks_analysis);
          // std::cout << "Before construct" << std::endl;
          m_d3d_cells.back()->IPhysicalVolume::Construct(this);
          // std::cout << "After construct" << std::endl;

        }
      }
    }
  }
  else{
    G4cout << "D3DMLayer:: \"" << GetName() << "\" initialization based on CSV positioning..." << G4endl;
    int idx,idy,idz;
    idy = m_id;
    idx = 0;
    idz = 0;
    for(const auto& cell_positioning : m_cells_in_layer_positioning){
      auto label = m_label+"_Cell_"+std::to_string(idx)+"_"+std::to_string(idy)+"_"+std::to_string(idz);
      auto cell_position = cell_positioning+m_init_possition;
      // G4cout << "[DEBUG]:: D3DMLayer:: creating cell: " << label << " with position: " << cell_position << G4endl;
      m_d3d_cells.push_back(new D3DCell(label,cell_position,m_cell_medium_name));
      m_d3d_cells.back()->SetIDs(idx++,idy,idz);
      m_d3d_cells.back()->SetNVoxels('x',m_cell_voxelization_x);
      m_d3d_cells.back()->SetNVoxels('y',m_cell_voxelization_y);
      m_d3d_cells.back()->SetNVoxels('z',m_cell_voxelization_z);
      m_d3d_cells.back()->SetTracksAnalysis(m_tracks_analysis);
      m_d3d_cells.back()->IPhysicalVolume::Construct(this);
      if(idx >= m_n_cells_in_layer_x) {
        idx = 0;
        idz++;
        }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DMLayer::Update() {
  // TODO:: implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DMLayer::DefineSensitiveDetector(){
  for (auto& cell : m_d3d_cells)
    cell->DefineSensitiveDetector();
}

