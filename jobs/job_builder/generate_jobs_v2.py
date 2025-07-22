import os

output_dir = "generated_tomls"
os.makedirs(output_dir, exist_ok=True)

base_y = -8.675
step_mm = 2
num_steps = int(70 / step_mm) + 1 # inclusive

template = """[RunSvc]
JobName = "{job_name}"
BeamType = "IAEA"
PhspInputFileName = "/localfs/814439/TB16112024_PROF_Y/1/phsp"
PhspEvtVrtxMultiplicityTreshold = 10
PrintProgressFrequency = 0.05
Physics = "LowE_Penelope" # "LowE_Livermore" "LowE_Penelope" "emstandard_opt3"
GenerateCT = false
PrimariesAnalysis = false
NTupleAnalysis = false
RunAnalysis = true

[GeoSvc]
BuildLinac = true
BuildPatient = true

[PatientGeometry]
Type = "D3DDetector"
PatientIsocentreX = 0.0
PatientIsocentreY = {isocentre_y}
PatientIsocentreZ = 0.0
EnviromentSizeX = 680.0
EnviromentSizeY = 710.0
EnviromentSizeZ = 460.0
EnviromentMedium = "Usr_G4AIR20C"
EnviromentPatientEnvelop = "ModularWaterPhantom_3mf"
PatientDBPath = "d3df-patients/patient/phantoms/modular_water_phantom"

[GeometryBuilder]
ExcludeObjList = [
"WaterPhantom v8|Cap v3:5",
"WaterPhantom v8|Cap v3:4",
"WaterPhantom v8|Cap v3:3",
"WaterPhantom v8|Cap v3:2",
"WaterPhantom v8|Cap v3:1"
]
Position = [0.0, 0.0, 0.0]
Rotation = [0.0, 0.0, 0.0]

[D3DDetector_Cell]
Medium = "RMPS470"
Voxelization = [5,5,5]

[RunSvc_Plan]
BeamRotation = 0
nParticles = 10000000
PlanInputFile = [ "plan/custom/cp5x5.dat"]
"""

for i in range(num_steps):
    y_mm = i * step_mm
    y_value = round(base_y + y_mm, 3)
    job_name = f"profile_wp_3mf_y_{y_mm}mm"
    content = template.format(job_name=job_name, isocentre_y=y_value)
    filename = os.path.join(output_dir, f"{job_name}.toml")
with open(filename, "w") as f:
    f.write(content)

print(f"Wygenerowano {num_steps} plików TOML w katalogu: {output_dir}")