#include "GeometryDBReader.hh"
#include "Services.hh"

#include <stdexcept>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <G4SystemOfUnits.hh>

namespace py = pybind11;

std::vector<GeometryDBReader::CellInfo> GeometryDBReader::m_db_cells_positioning;

GeometryDBReader::GeometryDBReader()
  : m_parser(py::module::import("xmlx_geometry_parser"))
{}

void GeometryDBReader::Finalize() {
    std::cout << "[DEBUG]:: GeometryDBReader manual finalizer\n";
    
}

GeometryDBReader& GeometryDBReader::Instance() {
    static GeometryDBReader instance;
    return instance;
}

// Load and parse geometry data from Excel/CSV
void GeometryDBReader::LoadDataBase(const std::string& path)
{
    std::cout << "Loading geoemtry data from: "<< path << std::endl;

    db_filename = path + "/D3DF_bodies.xlsx";
    csv_filename = path + "/D3DF_bodies.csv";
    sheet = "scintillator_mapping_db";
    

    py::object sheet_arg = sheet.empty()
    ? py::object(py::none())
    : py::object(py::str(sheet));
    py::object py_list = m_parser.attr("get_geometries")(db_filename, csv_filename, sheet_arg);
    auto vec = py_list.cast<std::vector<py::dict>>();

    geoms_.clear();
    geoms_.reserve(vec.size());

    for (auto &d : vec) {
        GeometryData gd;

        // Component name
        gd.component = py::cast<std::string>(d["component"]);
        
        // Body name / identifier
        gd.body = py::cast<std::string>(d["body"]);

        // Center of mass 
        auto com_py = d["com"].cast<std::vector<double>>();
        G4ThreeVector tranlation;
        if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "IbaImRT_3mf"){
            tranlation = G4ThreeVector(-95.0,90.0,90.0);
        }
        else if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "ModularWaterPhantom_3mf"){
            tranlation = G4ThreeVector(-271.0,275.0,225.0);
        }
        // gd.com = G4ThreeVector( com_py[0]*mm -95.0*mm , com_py[1]*mm -90.0*mm, com_py[2]*mm -90.0*mm );

        auto temp_vec = G4ThreeVector( com_py[0]*mm, com_py[1]*mm, com_py[2]*mm);
        temp_vec.rotate(180.0*deg, G4ThreeVector(1.0,0.0,0.0));
        gd.com = temp_vec+ tranlation;

        // Material name
        gd.mat = py::cast<std::string>(d["material"]);

        // Scintillator ID
        gd.sc_id = py::cast<std::string>(d["sc_id"]);
        if (!(gd.sc_id == "nan" || gd.sc_id.empty() || gd.sc_id == "")){
            m_db_cells_positioning.emplace_back(gd.sc_id, gd.com);
        }

        // Nodes
        gd.nodes = d["nodes"].cast<std::vector<std::array<int,3>>>();

        // Vertices
        {
            auto verts_py = d["vertices"].cast<std::vector<std::array<double,3>>>();
            gd.vertices.reserve(verts_py.size());
            for (auto &v : verts_py)
                gd.vertices.emplace_back(v[0]*mm, v[1]*mm, v[2]*mm);
        }

        // Normals
        {
            auto norms_py = d["normals"].cast<std::vector<std::array<double,3>>>();
            gd.normals.reserve(norms_py.size());
            for (auto &n : norms_py)
                gd.normals.emplace_back(n[0], n[1], n[2]);
        }

        geoms_.push_back(std::move(gd));
    }
    m_parser = py::object(); 

    std::cout<< "Got #" << m_db_cells_positioning.size() << " cell entries." << std::endl;
}

