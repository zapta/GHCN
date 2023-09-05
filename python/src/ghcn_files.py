import pandas as pd
import wget
import os
import logging
import argparse
import time
import numpy as np
from typing import Any

# ghcn files formats
# https://docs.opendata.aws/noaa-ghcn-pds/readme.html


logging.basicConfig(
    level=logging.INFO,
    format="%(relativeCreated)07d %(levelname)-7s %(filename)-10s: %(message)s",
)
logger = logging.getLogger("main")

parser = argparse.ArgumentParser()
parser.add_argument("--data_cache_dir",
                    dest="data_cache_dir",
                    default="./_data_cache",
                    help="Path to local data cache directory.")
parser.add_argument('--force_fetch',
                    dest="force_fetch",
                    default=False,
                    action=argparse.BooleanOptionalAction,
                    help="If true, all files are fetched from NOAA, even if already cached..")

args = parser.parse_args()

pd.set_option('display.max_columns', 15)
pd.set_option('display.width', 200)


def local_file_path(local_file_name: str) -> str:
    return os.path.join(args.data_cache_dir, local_file_name)


def fetch_data_file(remote_file_path: str, local_file_name: str):
    """"""
    local_path = local_file_path(local_file_name)
    if os.path.exists(local_path):
        if not args.force_fetch:
            logger.info(f"File {local_file_name} already in cache.")
            return
        logger.info(f"Deleting the old cache file {local_file_name}.")
        os.remove(local_path)
    start_time = time.time()
    logger.info(f"Fetching {remote_file_path}.")
    remote_file_url = f"ftp://ftp.ncdc.noaa.gov/{remote_file_path}"
    local_file_path2 = wget.download(remote_file_url, local_path)
    elapsed_time = time.time() - start_time
    logger.info(f"File {local_file_name} fetched in {elapsed_time:.1f} secs.")
    assert local_file_path2 == local_path, f"{local_file_path2} vs {local_path}"


def fetch_stations_list() -> Any:
    """Returns a data frame with complete station list."""
    local_name = "_stations.txt"
    local_path = local_file_path(local_name)
    fetch_data_file("pub/data/ghcn/daily/ghcnd-stations.txt", local_name)
    # NOTE: Start column index is one less than in the GHCN spec.
    column_specs = [(0, 11), (12, 20), (21, 30), (31, 37), (38, 40), (41, 71), (72, 75), (76, 79), (80, 85)]
    column_names = ['station_id', 'lat', 'lon', 'elev', 'state', 'station_name', 'gsn_flag', 'hcn_crn', 'wmo_id']
    logger.info("Loading stations data frame from file.")
    df = pd.read_fwf(local_path, colspecs=column_specs, names=column_names, header=None)
    logger.info(f"Stations data frame has {len(df)} records.")
    df.set_index('station_id', inplace=True)
    return df


def fetch_station_inventory() -> Any:
    """Returns a data frame with stations inventory covering data type and time range."""
    local_name = "_inventory.txt"
    local_path = local_file_path(local_name)
    fetch_data_file("pub/data/ghcn/daily/ghcnd-inventory.txt", local_name)
    # NOTE: Start column index is one less than in the GHCN spec.
    column_specs = [(0, 11), (12, 20), (21, 30), (31, 35), (36, 40), (41, 45)]
    column_names = ['station_id', 'lat', 'lon', 'elem', 'first_year', 'last_year']
    logger.info("Loading inventory data frame from file.")
    df = pd.read_fwf(local_path, colspecs=column_specs, names=column_names, header=None)
    logger.info(f"Inventory data frame has {len(df)} records.")
    # df.set_index('station_id', inplace=True)
    return df


# From https://stackoverflow.com/questions/57294120/calculating-distance-between-latitude-and-longitude-in-python#57298327
def haversine(Olat, Olon, Dlat, Dlon):
    radius = 6371.  # Earth in km
    d_lat = np.radians(Dlat - Olat)
    d_lon = np.radians(Dlon - Olon)
    a = (np.sin(d_lat / 2.) * np.sin(d_lat / 2.) +
         np.cos(np.radians(Olat)) * np.cos(np.radians(Dlat)) *
         np.sin(d_lon / 2.) * np.sin(d_lon / 2.))
    c = 2. * np.arctan2(np.sqrt(a), np.sqrt(1. - a))
    d = radius * c
    return d


def inject_distance_to_point(station_list_df: Any, lat: float, lon: float) -> Any:
    """Given a station list dataframe and a point, insert a 'dist' column with distance in km to that point."""
    distances = haversine(lat, lon, station_list_df[["lat"]].to_numpy(), station_list_df[["lon"]].to_numpy())
    station_list_df.insert(len(station_list_df.columns), "dist", distances)


# def haversine2(Opt, Dpt):
#     radius = 6371.  # Earth in km
#     d_lat = np.radians(Dpt[0] - Opt[0])
#     d_lon = np.radians(Dpt[1] - Opt[1])
#     a = (np.sin(d_lat / 2.) * np.sin(d_lat / 2.) +
#          np.cos(np.radians(Opt[0])) * np.cos(np.radians(Dpt[0])) *
#          np.sin(d_lon / 2.) * np.sin(d_lon / 2.))
#     c = 2. * np.arctan2(np.sqrt(a), np.sqrt(1. - a))
#     d = radius * c
#     return d


df = fetch_stations_list()
inject_distance_to_point(df, 37.1427, -121.9725)
print(df)

df.to_csv(local_file_path("_distances.csv"), index=False, float_format="%.1f", header=True)

candidates = df.loc[(df['dist'] <= 50)]
print(candidates)



# print(df[["lat", "lon"]].to_numpy())

# print("------------ x1 ")
# x1 = haversine(17.1167, -61.7833, df[["lat"]].to_numpy(), df[["lon"]].to_numpy())
# print(x1)

# print (len(df.columns))
#
# df.insert(len(df.columns), "dist", x1)
# print(df)
#
# # print("------------ x2 ")

# x2 = haversine2((17.1167 ,-61.7833), df[["lat", "lon"]].to_numpy() )
# print(x2)

# print("------@@@@@@@@@@@@@")
# print(df[["lat", "lon"]].to_numpy())

# print(df[:, 0])

# df = fetch_station_inventory()
# print(df)
