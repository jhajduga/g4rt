import numpy as np
from root_numpy import root2array, tree2array
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm

# read input ROOT file
data = root2array("phasespaces.root")

# data type needed to cast the contents into numpy array
in_dtype = [('x', '<f4'), ('y', '<f4'), ('z', '<f4'), ('u', '<f4'), ('v', '<f4'), ('w', '<f4'), ('E', '<f4'),
            ('type', '<i4'), ('index', '<f4')]

# data type and column order compatible with output binary phase space format
out_dtype = [('type', '<i4'), ('E', '<f4'), ('index', '<f4'), ('x', '<f4'), ('y', '<f4'), ('z', '<f4'), ('u', '<f4'), ('v', '<f4'), ('w', '<f4')]
new_column_order = [it[0] for it in out_dtype]

# loop over all phase space planes
for ind in np.unique(data['index']):
    single_plane = data[data['index'] == ind].astype(in_dtype).copy()
    single_plane['index'] = 1.0  # field not used, set to one
    filename = "phsp_{:d}.dat".format(ind)
    out_array = np.zeros(dtype=out_dtype, shape=single_plane.shape)
    for col in new_column_order:
        out_array[col] = single_plane[col]
    out_array.tofile(filename)
    print("Saved {:d} particles in {:s}.".format(out_array.size, filename))

    mean_z = np.mean(out_array['z'])

    # save debugging plots
    fig, ax = plt.subplots(3,1)
    for i,var in enumerate(['x', 'y', 'z']):
        ax[i].hist(out_array[var], bins=100)
        ax[i].set_xlabel(var + " [cm]")
    fig.subplots_adjust(hspace=0.5)
    fig.savefig("phsp_{:d}_xyz.png".format(ind), dpi=300)
    for a in ax:
        a.set_yscale('log')
    fig.savefig("phsp_{:d}_xyz_log.png".format(ind), dpi=300)
    plt.close(fig)

    try:
        fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
        _,_,_, im = ax[0].hist2d((out_array['y']**2 + out_array['x']**2)**0.5, out_array['z'], bins=100)
        fig.colorbar(im, ax=ax[0])
        _,_,_, im = ax[1].hist2d((out_array['y']**2 + out_array['x']**2)**0.5, out_array['z'], bins=100, norm=LogNorm())
        fig.colorbar(im, ax=ax[1])
        for a in ax:
            a.set_xlabel('r [cm]')
            a.set_ylabel('z [cm]')
        plt.savefig("phsp_{:d}_rz.png".format(ind), dpi=300)
        plt.close(fig)
    except ValueError:
        pass

    fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
    _,_,_, im = ax[0].hist2d(out_array['x'], out_array['y'], bins=100)
    fig.colorbar(im, ax=ax[0])
    _,_,_, im = ax[1].hist2d(out_array['x'], out_array['y'], bins=100, norm=LogNorm())
    fig.colorbar(im, ax=ax[1])
    for a in ax:
        a.set_xlabel('x [cm]')
        a.set_ylabel('y [cm]')
    ax[0].set_title("Full plane Z = {:g} cm: {:d} particles ({:d} photons ({:3.3f} %)))".format(
        mean_z,
        out_array['x'].size,
        out_array['x'][out_array['type'] == 1].size,
        100.0 * float(out_array['x'][out_array['type'] == 1].size) / out_array['x'].size
    )
    )
    plt.savefig("phsp_{:d}_xy.png".format(ind), dpi=300)

    fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
    _,_,_, im = ax[0].hist2d(out_array['x'][out_array['type'] == 1], out_array['y'][out_array['type'] == 1], bins=100)
    fig.colorbar(im, ax=ax[0])
    _,_,_, im = ax[1].hist2d(out_array['x'][out_array['type'] == 1], out_array['y'][out_array['type'] == 1], bins=100, norm=LogNorm())
    fig.colorbar(im, ax=ax[1])
    for a in ax:
        a.set_xlabel('x [cm]')
        a.set_ylabel('y [cm]')
    ax[0].set_title("Full plane Z = {:g} cm: {:d} photons)".format(mean_z, out_array['x'][out_array['type'] == 1].size))
    plt.savefig("phsp_{:d}_xy_photons.png".format(ind), dpi=300)

    max_field_radius = 40.0 * (2.0**0.5) * 0.5
    x_range = 1.2 * max_field_radius * ((100.0 - mean_z) / 100.0)
    y_range = x_range
    fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
    filter_cond = (np.abs(out_array['x']) < x_range) & (np.abs(out_array['y']) < y_range)
    _,_,_, im = ax[0].hist2d(out_array['x'][filter_cond], out_array['y'][filter_cond], bins=100)
    fig.colorbar(im, ax=ax[0])
    _,_,_, im = ax[1].hist2d(out_array['x'][filter_cond], out_array['y'][filter_cond], bins=100, norm=LogNorm())
    fig.colorbar(im, ax=ax[1])
    for a in ax:
        a.set_xlabel('x [cm]')
        a.set_ylabel('y [cm]')
    ax[0].set_title("Zoom region, {:3.4f}% particles from the full plane ({:d} / {:d})".format(
        100.0 * out_array['x'][filter_cond].size / float(out_array['x'].size), out_array['x'][filter_cond].size, out_array['x'].size))
    plt.savefig("phsp_{:d}_xy_zoom.png".format(ind), dpi=300)
    plt.close(fig)

    fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
    filter_cond = (np.abs(out_array['x']) < x_range) & (np.abs(out_array['y']) < y_range) & (out_array['type'] == 1)
    _,_,_, im = ax[0].hist2d(out_array['x'][filter_cond], out_array['y'][filter_cond], bins=100)
    fig.colorbar(im, ax=ax[0])
    _,_,_, im = ax[1].hist2d(out_array['x'][filter_cond], out_array['y'][filter_cond], bins=100, norm=LogNorm())
    fig.colorbar(im, ax=ax[1])
    for a in ax:
        a.set_xlabel('x [cm]')
        a.set_ylabel('y [cm]')
    ax[0].set_title("Zoom region, {:3.4f}% photons from the full plane ({:d} / {:d})".format(
        100.0 * out_array['x'][filter_cond].size / float(out_array['x'][out_array['type'] == 1].size), out_array['x'][filter_cond].size, out_array['x'][out_array['type'] == 1].size))
    plt.savefig("phsp_{:d}_xy_photons_zoom.png".format(ind), dpi=300)
    plt.close(fig)

    fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
    _,_,_, im = ax[0].hist2d(out_array['u'], out_array['v'], bins=100)
    fig.colorbar(im, ax=ax[0])
    _,_,_, im = ax[1].hist2d(out_array['u'], out_array['v'], bins=100, norm=LogNorm())
    fig.colorbar(im, ax=ax[1])
    for a in ax:
        a.set_xlabel('u')
        a.set_ylabel('v')
    plt.savefig("phsp_{:d}_uv.png".format(ind), dpi=300)
    plt.close(fig)

    fig, ax = plt.subplots(2,1, figsize=(8,10), sharex=True, sharey=True, tight_layout=True)
    _,_,_, im = ax[0].hist2d(out_array['u'], out_array['x'], bins=100)
    fig.colorbar(im, ax=ax[0])
    _,_,_, im = ax[1].hist2d(out_array['u'], out_array['x'], bins=100, norm=LogNorm())
    fig.colorbar(im, ax=ax[1])
    for a in ax:
        a.set_xlabel('u')
        a.set_ylabel('x [cm]')
    plt.savefig("phsp_{:d}_ux.png".format(ind), dpi=300)
    plt.close(fig)