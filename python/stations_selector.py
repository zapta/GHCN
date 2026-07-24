"""A program to find stations id in a radius from a lat/lon."""

import sys
from typing import Any
import logging
import argparse
import pandas as pd
import numpy as np
from ghcn_cache import GhcnCache

# ghcn files formats
# https://docs.opendata.aws/noaa-ghcn-pds/readme.html


logging.basicConfig(
    level=logging.INFO,
    format="%(relativeCreated)07d %(levelname)-7s %(filename)-10s: %(message)s",
)
logger = logging.getLogger("main")

parser = argparse.ArgumentParser()
parser.add_argument(
    "--lat",
    type=float,
    dest="lat",
    default=37.256604,
    help="Latitude of center of search radius",
)
parser.add_argument(
    "--lon",
    type=float,
    dest="lon",
    default=-121.858915,
    help="Longitude of center of search radius",
)
parser.add_argument(
    "--radius-km",
    type=float,
    dest="radius_km",
    default=50.0,
    help="Radius of search circle, in km.",
)
parser.add_argument(
    "--min-year",
    type=int,
    dest="min_year",
    default=2015,
    help="First year of data of interest.",
)
parser.add_argument(
    "--max-year",
    type=int,
    dest="max_year",
    default=2022,
    help="Last year of data of interest.",
)

parser.add_argument(
    "--data_cache_dir",
    dest="data_cache_dir",
    default="./_data_cache",
    help="Path to local data cache directory.",
)

parser.add_argument(
    "--force_fetch",
    dest="force_fetch",
    default=False,
    action=argparse.BooleanOptionalAction,
    help="If true, all files are fetched from NOAA, even if already cached..",
)

args = parser.parse_args()

pd.set_option("display.max_columns", 15)
pd.set_option("display.width", 200)


# From https://stackoverflow.com/questions/57294120/calculating-distance-between-
#            latitude-and-longitude-in-python#57298327
def haversine(lat1, lon1, lat2, lon2):
    """Calculate the distance in Km between two lat/lon points."""
    radius = 6371.0  # Earth radius in km
    d_lat = np.radians(lat2 - lat1)
    d_lon = np.radians(lon2 - lon1)
    a = np.sin(d_lat / 2.0) * np.sin(d_lat / 2.0) + np.cos(np.radians(lat1)) * np.cos(
        np.radians(lat2)
    ) * np.sin(d_lon / 2.0) * np.sin(d_lon / 2.0)
    c = 2.0 * np.arctan2(np.sqrt(a), np.sqrt(1.0 - a))
    d = radius * c
    return d


def inject_distance_to_point(station_list_df: Any, lat: float, lon: float) -> Any:
    """Given a station list dataframe and a point, insert a 'dist' column
    with distance in km to that point."""
    distances = haversine(
        lat,
        lon,
        station_list_df[["lat"]].to_numpy(),
        station_list_df[["lon"]].to_numpy(),
    )
    station_list_df.insert(len(station_list_df.columns), "dist", distances)


# def get_stations_info():
#     """Returns a data frame with stations list and inventory info"""
#     file_name = "_stations_info.csv"
#     file_path = local_file_path(file_name)
#     if os.path.exists(file_path):
#         if not args.force_fetch:
#             logger.info(f"File {file_name} already in cache.")
#             info_df = pd.read_csv(file_path, delimiter=',')
#             return info_df
#         logger.info(f"Deleting the old cache file {file_name}.")
#         os.remove(file_path)
#     stations_df = fetch_stations_list()
#     inventory_df = fetch_station_inventory()
#     tmp_df = inventory_df.loc[inventory_df['elem'] == "TMIN"]
#     tmp_df = tmp_df[['station_id', 'first_year', 'last_year']]
#     info_df = pd.merge(stations_df, tmp_df, on='station_id', how='left')
#     logger.info(f"Writing stations info cache file {file_name}")
#     info_df.to_csv(file_path, index=False, header=True)
#     return info_df


def get_matching_stations(ghcn_cache: GhcnCache) -> Any:
    """Return data frame with info of matching stations"""
    df1 = ghcn_cache.get_stations_info()
    inject_distance_to_point(df1, args.lat, args.lon)
    result = df1.loc[
        (df1["dist"] <= args.radius_km)
        & (df1["first_year"] <= args.min_year)
        & (df1["last_year"] >= args.max_year)
    ]
    result.sort_values(by=["dist"], inplace=True)
    result.set_index("station_id", inplace=True)
    result.drop(["gsn_flag", "hcn_crn", "wmo_id"], axis=1, inplace=True)
    return result


def main() -> int:
    """Main function"""
    ghcn_cache = GhcnCache(args.data_cache_dir, args.force_fetch)
    df = get_matching_stations(ghcn_cache)
    print(df)


if __name__ == "__main__":
    sys.exit(main())
