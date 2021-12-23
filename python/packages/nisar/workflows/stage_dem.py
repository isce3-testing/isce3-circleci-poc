#!/usr/bin/env python3

# DEM staging

import argparse
import os

import numpy as np
import shapely.ops
import shapely.wkt
from osgeo import gdal, osr
from shapely.geometry import LinearRing, Point, Polygon, box


def cmdLineParse():
    """
    Command line parser
    """
    parser = argparse.ArgumentParser(
        description="""
                                     Stage and verify DEM for processing. """,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "-p",
        "--product",
        type=str,
        action="store",
        help="Input reference RSLC HDF5 product",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        action="store",
        default="dem.vrt",
        dest="outfile",
        help="Output DEM filepath (VRT format).",
    )
    parser.add_argument(
        "-f",
        "--path",
        type=str,
        action="store",
        dest="filepath",
        default="file",
        help="Filepath to user DEM.",
    )
    parser.add_argument(
        "-m",
        "--margin",
        type=int,
        action="store",
        default=5,
        help="Margin for DEM bounding box (km)",
    )
    parser.add_argument(
        "-b",
        "--bbox",
        type=float,
        action="store",
        dest="bbox",
        default=None,
        nargs="+",
        help="Spatial bounding box in latitude/longitude (WSEN, decimal degrees)",
    )
    return parser.parse_args()


def check_dateline(poly):
    """Split `poly` if it crosses the dateline.

    Parameters
    ----------
    poly : shapely.geometry.Polygon
        Input polygon.

    Returns
    -------
    polys : list of shapely.geometry.Polygon
         A list containing: the input polygon if it didn't cross
        the dateline, or two polygons otherwise (one on either
        side of the dateline).
    """

    xmin, _, xmax, _ = poly.bounds
    # Check dateline crossing
    if (xmax - xmin) > 180.0:
        dateline = shapely.wkt.loads("LINESTRING( 180.0 -90.0, 180.0 90.0)")

        # build new polygon with all longitudes between 0 and 360
        x, y = poly.exterior.coords.xy
        new_x = (k + (k <= 0.0) * 360 for k in x)
        new_ring = LinearRing(zip(new_x, y))

        # Split input polygon
        # (https://gis.stackexchange.com/questions/232771/splitting-polygon-by-linestring-in-geodjango_)
        merged_lines = shapely.ops.linemerge([dateline, new_ring])
        border_lines = shapely.ops.unary_union(merged_lines)
        decomp = shapely.ops.polygonize(border_lines)

        polys = list(decomp)
        assert len(polys) == 2
    else:
        # If dateline is not crossed, treat input poly as list
        polys = [poly]

    return polys


def determine_polygon(ref_slc, bbox=None):
    """Determine bounding polygon using RSLC radar grid/orbit
    or user-defined bounding box

    Parameters:
    ----------
    ref_slc: str
        Filepath to reference RSLC product
    bbox: list, float
        Bounding box with lat/lon coordinates (decimal degrees)
        in the form of [West, South, East, North]

    Returns:
    -------
    poly: shapely.Geometry.Polygon
        Bounding polygon corresponding to RSLC perimeter
        or bbox shape on the ground
    """
    if bbox is not None:
        print("Determine polygon from bounding box")
        poly = box(bbox[0], bbox[1], bbox[2], bbox[3])
    else:
        print("Determine polygon from RSLC radar grid and orbit")
        poly = get_geo_polygon(ref_slc)

    return poly


def point2epsg(lon, lat):
    """Return EPSG code based on point lat/lon

    Parameters:
    ----------
    lat: float
        Latitude coordinate of the point
    lon: float
        Longitude coordinate of the point

    Returns:
    -------
    epsg code corresponding to the point lat/lon coordinates
    """
    if lon >= 180.0:
        lon = lon - 360.0
    if lat >= 75.0:
        return 3413
    elif lat <= -75.0:
        return 3031
    elif lat > 0:
        return 32601 + int(np.round((lon + 177) / 6.0))
    elif lat < 0:
        return 32701 + int(np.round((lon + 177) / 6.0))
    else:
        raise ValueError("Could not determine projection for {0},{1}".format(lat, lon))


def get_geo_polygon(ref_slc, min_height=-500.0, max_height=9000.0, pts_per_edge=5):
    """Create polygon (EPSG:4326) using RSLC radar grid and orbits

    Parameters:
    -----------
    ref_slc: str
        Path to RSLC product to stage the DEM for
    min_height: float
        Global minimum height (in m) for DEM interpolator
    max_height: float
        Global maximum height (in m) for DEM interpolator
    pts_per_edge: float
        Number of points per edge for min/max bounding box computation

    Returns:
    -------
    poly: shapely.Geometry.Polygon
        Bounding polygon corresponding to RSLC perimeter on the ground
    """
    from isce3.core import LUT2d
    from isce3.geometry import DEMInterpolator, get_geo_perimeter_wkt
    from nisar.products.readers import SLC

    # Prepare SLC dataset input
    productSlc = SLC(hdf5file=ref_slc)

    # Extract orbits, radar grid, and doppler for frequency A
    orbit = productSlc.getOrbit()
    radar_grid = productSlc.getRadarGrid(frequency="A")
    doppler = LUT2d()

    # Get min and max global height DEM interpolators
    dem_min = DEMInterpolator(height=min_height)
    dem_max = DEMInterpolator(height=max_height)

    # Get min and max bounding boxes
    box_min = get_geo_perimeter_wkt(radar_grid, orbit, doppler, dem_min, pts_per_edge)
    box_max = get_geo_perimeter_wkt(radar_grid, orbit, doppler, dem_max, pts_per_edge)

    # Determine minimum and maximum polygons
    poly_min = shapely.wkt.loads(box_min)
    poly_max = shapely.wkt.loads(box_max)

    # Get polygon from intersection of poly_min and poly_max
    poly = poly_min | poly_max

    return poly


def determine_projection(polys):
    """Determine EPSG code for each polygon in polys.
    EPSG is computed for a regular list of points. EPSG
    is assigned based on a majority criteria.

    Parameters:
    -----------
    polys: shapely.Geometry.Polygon
        List of shapely Polygons
    Returns:
    --------
    epsg:
        List of EPSG codes corresponding to elements in polys
    """

    epsg = []

    # Make a regular grid based on polys min/max latitude longitude
    for p in polys:
        xmin, ymin, xmax, ymax = p.bounds
        xx, yy = np.meshgrid(np.linspace(xmin, xmax, 250), np.linspace(ymin, ymax, 250))
        x = xx.flatten()
        y = yy.flatten()

        # Query to determine the zone
        zones = []
        for lx, ly in zip(x, y):
            # Create a point with grid coordinates
            pp = Point(lx, ly)
            # If Point is in polys, compute EPSG
            if pp.within(p):
                zones.append(point2epsg(lx, ly))

        # Count different EPSGs
        vals, counts = np.unique(zones, return_counts=True)
        # Get the ESPG for Polys
        epsg.append(vals[np.argmax(counts)])

    return epsg


def download_dem(polys, epsgs, margin, outfile):
    """Download DEM from nisar-dem bucket

    Parameters:
    ----------
    polys: shapely.geometry.Polygon
        List of shapely polygons
    epsg: str, list
        List of EPSG codes corresponding to polys
    margin: float
        Buffer margin (in km) applied for DEM download
    outfile:
        Path to the output DEM file to be staged
    """

    if 3031 in epsgs:
        epsgs = [3031] * len(epsgs)
        polys = transform_polygon_coords(polys, epsgs)
        # Need one EPSG as in polar stereo we have one big polygon
        epsgs = [3031]
        margin = margin * 1000
    elif 3413 in epsgs:
        epsgs = [3413] * len(epsgs)
        polys = transform_polygon_coords(polys, epsgs)
        # Need one EPSG as in polar stereo we have one big polygon
        epsgs = [3413]
        margin = margin * 1000
    else:
        # set epsg to 4326 for each element in the list
        epsgs = [4326] * len(epsgs)
        # convert margin to degree (approx formula)
        margin = margin / 40000 * 360

    # Download DEM for each polygon/epsg
    file_prefix = os.path.splitext(outfile)[0]
    dem_list = []
    for n, (epsg, poly) in enumerate(zip(epsgs, polys)):
        vrt_filename = f"/vsis3/nisar-dem/EPSG{epsg}/EPSG{epsg}.vrt"
        poly.buffer(margin)
        outpath = f"{file_prefix}_{n}.tiff"
        dem_list.append(outpath)
        xmin, ymin, xmax, ymax = poly.bounds
        ds = gdal.Open(vrt_filename, gdal.GA_ReadOnly)
        gdal.Translate(outpath, ds, format="GTiff", projWin=[xmin, ymax, xmax, ymin])

    # Build vrt with downloaded DEMs
    gdal.BuildVRT(outfile, dem_list)


def transform_polygon_coords(polys, epsgs):
    """Transform coordinates of polys (list of polygons)
       to target epsgs (list of EPSG codes)

    Parameters:
    ----------
    polys: shapely.Geometry.Polygon
        List of shapely polygons
    epsg: list, str
        List of EPSG codes corresponding to
        elements in polys
    """

    # Assert validity of inputs
    assert len(polys) == len(epsgs)

    # Transform each point of the perimeter in target EPSG coordinates
    llh = osr.SpatialReference()
    llh.ImportFromEPSG(4326)
    tgt = osr.SpatialReference()

    xmin, ymin, xmax, ymax = [], [], [], []
    tgt_x, tgt_y = [], []
    for poly, epsg in zip(polys, epsgs):
        x, y = poly.exterior.coords.xy
        tgt.ImportFromEPSG(int(epsg))
        trans = osr.CoordinateTransformation(llh, tgt)
        for lx, ly in zip(x, y):
            dummy_x, dummy_y, dummy_z = trans.TransformPoint(ly, lx, 0)
            tgt_x.append(dummy_x)
            tgt_y.append(dummy_y)
        xmin.append(min(tgt_x))
        ymin.append(min(tgt_y))
        xmax.append(max(tgt_x))
        ymax.append(max(tgt_y))
    # return a polygon
    poly = [
        Polygon(
            [
                (min(xmin), min(ymin)),
                (min(xmin), max(ymax)),
                (max(xmax), max(ymax)),
                (max(xmax), min(ymin)),
            ]
        )
    ]

    return poly


def check_dem_overlap(DEMFilepath, polys):
    """Evaluate overlap between user-provided DEM
       and DEM that stage_dem.py would download
       based on RSLC or bbox provided information

    Parameters:
    ----------
    DEMFilepath: str
        Filepath to the user-provided DEM
    polys: shapely.geometry.Polygon
        List of polygons computed from RSLC or bbox

    Returns:
    -------
    perc_area: float
        Area (in percentage) covered by the intersection between the
        user-provided dem and the one downloadable by stage_dem.py
    """
    from isce3.io import Raster

    # Get local DEM edge coordinates
    DEM = Raster(DEMFilepath)
    ulx, xres, xskew, uly, yskew, yres = DEM.get_geotransform()
    lrx = ulx + (DEM.width * xres)
    lry = uly + (DEM.length * yres)
    poly_dem = Polygon([(ulx, uly), (ulx, lry), (lrx, lry), (lrx, uly)])

    # Initialize epsg
    epsg = [DEM.get_epsg()] * len(polys)

    if DEM.get_epsg() != 4326:
        polys = transform_polygon_coords(polys, epsg)

    perc_area = 0
    for poly in polys:
        perc_area += (poly.intersection(poly_dem).area / poly.area) * 100
    return perc_area


def check_aws_connection():
    """Check connection to AWS s3://nisar-dem bucket
    Throw exception if no connection is established
    """
    import boto3

    s3 = boto3.resource("s3")
    obj = s3.Object("nisar-dem", "EPSG3031/EPSG3031.vrt")
    try:
        body = obj.get()["Body"].read()
    except:
        errmsg = (
            "No access to nisar-dem s3 bucket. Check your AWS credentials"
            "and re-run the code"
        )
        raise ValueError(errmsg)


def main(opts):
    """Main script to execute dem staging

    Parameters:
    ----------
    opts : argparse.ArgumentParser
        Argument parser
    """

    # Check if RSLC or bbox are provided
    if (opts.product is None) & (opts.bbox is None):
        errmsg = (
            "Need to provide reference RSLC HDF5 or bounding box. "
            "Cannot download DEM"
        )
        raise ValueError(errmsg)

    # Make sure that output file has VRT extension
    if not opts.outfile.lower().endswith(".vrt"):
        err_msg = "DEM output filename extension is not .vrt"
        raise ValueError(err_msg)

    # Determine polygon based on RSLC info or bbox
    poly = determine_polygon(opts.product, opts.bbox)

    # Check dateline crossing. Returns list of polygons
    polys = check_dateline(poly)

    if os.path.isfile(opts.filepath):
        print("Check overlap with user-provided DEM")
        overlap = check_dem_overlap(opts.filepath, polys)
        if overlap < 75.0:
            print("Insufficient DEM coverage. Errors might occur")
        print(f"DEM coverage is {overlap} %")
    else:
        # Check connection to AWS s3 nisar-dem bucket
        try:
            check_aws_connection()
        except ImportError:
            import warnings

            warnings.warn(
                "boto3 is require to verify AWS connection"
                "proceeding without verifying connection"
            )
        # Determine EPSG code
        epsg = determine_projection(polys)
        # Download DEM
        download_dem(polys, epsg, opts.margin, opts.outfile)
        print("Done, DEM store locally")


if __name__ == "__main__":
    opts = cmdLineParse()
    main(opts)
