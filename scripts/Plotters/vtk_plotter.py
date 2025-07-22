import pandas as pd
import numpy as np
import vtk
from vtk.util import numpy_support
import os


def create_grid_actor(bounds, spacing):

    grid_points = vtk.vtkPoints()
    for x in np.arange(bounds[0], bounds[1] + spacing, spacing):
        for y in np.arange(bounds[2], bounds[3] + spacing, spacing):
            grid_points.InsertNextPoint(x, y, bounds[4])
            grid_points.InsertNextPoint(x, y, bounds[5])
        for z in np.arange(bounds[4], bounds[5] + spacing, spacing):
            grid_points.InsertNextPoint(x, bounds[2], z)
            grid_points.InsertNextPoint(x, bounds[3], z)
    for y in np.arange(bounds[2], bounds[3] + spacing, spacing):
        for z in np.arange(bounds[4], bounds[5] + spacing, spacing):
            grid_points.InsertNextPoint(bounds[0], y, z)
            grid_points.InsertNextPoint(bounds[1], y, z)

    # Create the grid lines
    grid_lines = vtk.vtkCellArray()
    num_points = grid_points.GetNumberOfPoints()
    for i in range(0, num_points, 2):
        grid_lines.InsertNextCell(2)
        grid_lines.InsertCellPoint(i)
        grid_lines.InsertCellPoint(i + 1)

    # Create the grid polydata
    grid_polydata = vtk.vtkPolyData()
    grid_polydata.SetPoints(grid_points)
    grid_polydata.SetLines(grid_lines)

    # Create the grid mapper and actor
    grid_mapper = vtk.vtkPolyDataMapper()
    grid_mapper.SetInputData(grid_polydata)
    grid_actor = vtk.vtkActor()
    grid_actor.SetMapper(grid_mapper)
    grid_actor.GetProperty().SetColor(0.8, 0.8, 0.8)
    grid_actor.GetProperty().SetOpacity(0.5)
    
    return grid_actor

def create_vtk_image_data(cell_df, voxel_side_len):
    print("Creating vtkImageData object...")
    
    # Unique coordinates
    x_coords = np.unique(cell_df['X [mm]'])
    y_coords = np.unique(cell_df['Y [mm]'])
    z_coords = np.unique(cell_df['Z [mm]'])
    
    x_dim = len(x_coords)
    y_dim = len(y_coords)
    z_dim = len(z_coords)

    print(f"Dimensions: {x_dim}, {y_dim}, {z_dim}")

    # Create vtkImageData object
    imageData = vtk.vtkImageData()
    imageData.SetDimensions(x_dim, y_dim, z_dim)
    imageData.SetSpacing(voxel_side_len, voxel_side_len, voxel_side_len)
    imageData.SetOrigin(np.min(x_coords), np.min(y_coords), np.min(z_coords))

    imageData.AllocateScalars(vtk.VTK_FLOAT, 1)

    # Map coordinates to indices
    coord_to_index = {
        'x': {v: i for i, v in enumerate(x_coords)},
        'y': {v: i for i, v in enumerate(y_coords)},
        'z': {v: i for i, v in enumerate(z_coords)}
    }

    scalar_data = np.zeros((z_dim, y_dim, x_dim), dtype=np.float32)
    observable = 'Dose [Gy]'

    
    print("Filling vtkImageData with voxel values...")
    # Convert coordinate mappings to NumPy arrays for fast lookup
    x_indices = cell_df['X [mm]'].map(coord_to_index['x']).to_numpy()
    y_indices = cell_df['Y [mm]'].map(coord_to_index['y']).to_numpy()
    z_indices = cell_df['Z [mm]'].map(coord_to_index['z']).to_numpy()

    # Assign values using NumPy advanced indexing
    scalar_data[z_indices, y_indices, x_indices] = cell_df[observable].to_numpy()

    vtk_data_array = numpy_support.numpy_to_vtk(scalar_data.ravel(), deep=True, array_type=vtk.VTK_FLOAT)
    imageData.GetPointData().SetScalars(vtk_data_array)

    print("vtkImageData creation complete.")
    return imageData

def main():
    # csv_path = '/home/geant4/workspace/github/g4rt/output/test_srunet3d_4x4x2_64x64x64_7/sim/cp10x10/cp10x10_ct_dose_cell.csv'
    csv_path = '/home/geant4/workspace/github/g4rt/output/imrt_test/sim/cp10x10/cp10x10_ct_dose_cell.csv'
    if not os.path.exists(csv_path):
        print(f"CSV file not found at {csv_path}")
        return
    
    print("Reading CSV file...")
    cell_df = pd.read_csv(csv_path)
    cell_df = cell_df.sort_values(by=['X [mm]', 'Y [mm]', 'Z [mm]'])
    cell_df["Dose [Gy]"] = cell_df["Dose [Gy]"]/cell_df["Dose [Gy]"].max()
    print(cell_df[cell_df["Dose [Gy]"] > 0])

    if cell_df.empty:
        print("DataFrame is empty. Exiting...")
        return

    voxel_side_len = 10

    print("Creating vtkImageData from DataFrame...")
    imageData = create_vtk_image_data(cell_df, voxel_side_len)

    print("Creating mapper...")
    mapper = vtk.vtkSmartVolumeMapper()
    mapper.SetInputData(imageData)

    print("Creating actor...")
    actor = vtk.vtkVolume()
    actor.SetMapper(mapper)

    print("Creating color and opacity transfer functions...")
    colorFunc = vtk.vtkColorTransferFunction()
    opacityFunc = vtk.vtkPiecewiseFunction()

    # Define a rainbow gradient for the color transfer function
    colorFunc.AddRGBPoint(0.0, 0.0, 0.0, 1.0)  # Blue at the minimum value
    colorFunc.AddRGBPoint(0.25, 0.0, 1.0, 1.0) # Cyan
    colorFunc.AddRGBPoint(0.5, 0.0, 1.0, 0.0)  # Green
    colorFunc.AddRGBPoint(0.75, 1.0, 1.0, 0.0) # Yellow
    colorFunc.AddRGBPoint(1.0, 1.0, 0.0, 0.0)  # Red at the maximum value

    opacityFunc.AddPoint(0.0, 0.0)
    opacityFunc.AddPoint(1.0, 1.0)

    actor.GetProperty().SetColor(colorFunc)
    actor.GetProperty().SetScalarOpacity(opacityFunc)
    actor.GetProperty().SetInterpolationTypeToNearest()

    print("Creating renderer and adding actor...")
    renderer = vtk.vtkRenderer()
    renderer.AddVolume(actor)
    renderer.SetBackground(1, 1, 1)

    # Add axes
    print("Adding axes...")
    axes = vtk.vtkAxesActor()
    axes.SetTotalLength(25, 25, 25)
    axes.SetShaftTypeToLine()
    axes.SetTipTypeToCone()
    axes.SetConeRadius(0.2)
    axes.SetXAxisLabelText("X")
    axes.SetYAxisLabelText("Y")
    axes.SetZAxisLabelText("Z")
    renderer.AddActor(axes)

    # Add scalar bar
    print("Adding scalar bar...")
    scalar_bar = vtk.vtkScalarBarActor()
    scalar_bar.SetLookupTable(colorFunc)
    scalar_bar.SetTitle("Dose [Gy]")
    scalar_bar.GetLabelTextProperty().SetColor(0, 0, 0)
    scalar_bar.GetTitleTextProperty().SetColor(0, 0, 0)
    renderer.AddActor2D(scalar_bar)

    # Add grid
    print("Adding grid...")
    bounds = imageData.GetBounds()
    grid_actor = create_grid_actor(bounds, 10.0)
    renderer.AddActor(grid_actor)

    print("Creating render window and adding renderer...")
    renderWindow = vtk.vtkRenderWindow()
    renderWindow.AddRenderer(renderer)

    print("Creating render window interactor...")
    renderWindowInteractor = vtk.vtkRenderWindowInteractor()
    renderWindowInteractor.SetRenderWindow(renderWindow)

    print("Checking renderer setup...")
    if renderer.GetVolumes().GetNumberOfItems() == 0:
        print("Warning: No volumes added to renderer.")
    else:
        print("Volume successfully added to renderer.")

    interactor_style = vtk.vtkInteractorStyleTrackballCamera()
    renderWindowInteractor.SetInteractorStyle(interactor_style)
    print("Starting visualization...")
    renderWindow.Render()
    renderWindowInteractor.Start()
    print("Visualization should be running...")

if __name__ == "__main__":
    main()
