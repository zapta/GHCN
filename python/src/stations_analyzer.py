import pandas as pd
import wget
import os
import logging
import argparse
import time
from typing import Any
import gzip
import shutil

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


def gunzip_shutil(source_filepath, dest_filepath, block_size=65536):
    with gzip.open(source_filepath, 'rb') as s_file, \
            open(dest_filepath, 'wb') as d_file:
        shutil.copyfileobj(s_file, d_file, block_size)


def fetch_station_raw_data(station_id: str) -> Any:
    """Returns a data frame with complete station list."""
    local_raw_csv_name = f"_station_{station_id}_raw.csv"
    local_raw_csv_path = local_file_path(local_raw_csv_name)
    col_types = {'m_flag': str, 'q_flag': str, 's_flag': str, 'obs': str}
    if os.path.exists(local_raw_csv_path):
        if not args.force_fetch:
            logger.info(f"File {local_raw_csv_name} already in cache.")
            raw_station_df = pd.read_csv(local_raw_csv_path, dtype=col_types, delimiter=',')
            return raw_station_df
        logger.info(f"Deleting the old cache file {local_raw_csv_name}.")
        os.remove(local_raw_csv_path)
    local_gz_name = f"_station_{station_id}.csv.gz"
    local_gz_path = local_file_path(local_gz_name)
    remote_path = f"pub/data/ghcn/daily/by_station/{station_id}.csv.gz"
    fetch_data_file(remote_path, local_gz_name)
    local_tmp_csv_name = f"_station_{station_id}_tmp.csv"
    local_tmp_csv_path = local_file_path(local_tmp_csv_name)
    gunzip_shutil(local_gz_path, local_tmp_csv_path)
    col_names = ['station_id', 'ymd', 'elem', 'value', 'm_flag', 'q_flag', 's_flag', 'obs']
    # col_types = {'station_id' :str, 'ymd':int, 'elem':str, 'value':int, 'm_flag':str, 'q_flag':str, 's_flag':str, 'obs':str}
    raw_station_df = pd.read_csv(local_tmp_csv_path, names=col_names, dtype=col_types, delimiter=',')
    raw_station_df.to_csv(local_raw_csv_path, index=False, header=True)
    return raw_station_df


def fetch_station_summary(station_id: str) -> Any:
    """Returns the station summary data """
    # Lookup in cache
    local_csv_name = f"_station_{station_id}_summary.csv"
    local_csv_path = local_file_path(local_csv_name)
    # col_types = {'m_flag': str, 'q_flag': str, 's_flag': str, 'obs': str}
    if os.path.exists(local_csv_path):
        if not args.force_fetch:
            logger.info(f"File {local_csv_name} already in cache.")
            summary_df = pd.read_csv(local_csv_path, delimiter=',')
            return summary_df
        logger.info(f"Deleting the old cache file {local_csv_name}.")
        os.remove(local_csv_path)

    # Fetch station raw data
    raw_df = fetch_station_raw_data("USC00047965")
    # print(df_raw)

    # Extract tmin by date
    min_df = raw_df.loc[raw_df['elem'] == "TMIN"]
    min_df = min_df[['ymd', 'value']].copy()
    min_df['value'] = min_df['value'] / 10.0
    min_df.rename(columns={"value": 'tmin'}, inplace=True)
    # print(min_df)

    # Extract tmax by date
    max_df = raw_df.loc[raw_df['elem'] == "TMAX"]
    max_df = max_df[['ymd', 'value']].copy()
    max_df['value'] = max_df['value'] / 10.0
    max_df.rename(columns={"value": 'tmax'}, inplace=True)
    # print(max_df)

    # Compute tavg by date
    avg_df = pd.merge(min_df, max_df, on='ymd', how='inner')
    avg_df['tavg'] = avg_df[['tmin', 'tmax']].mean(1)
    avg_df.drop(['tmin', 'tmax'], axis=1, inplace=True)
    # print(avg_df)

    # Combine tmin, tmax, tavg by date
    summary_df = pd.merge(min_df, max_df, on='ymd', how='outer')
    summary_df = pd.merge(summary_df, avg_df, on='ymd', how='outer')

    # Add specific columns for year and month
    summary_df['year'] = summary_df[['ymd']].floordiv(10000)
    summary_df['month'] = summary_df[['ymd']].mod(10000).floordiv(100)
    summary_df = summary_df.reindex(['ymd', 'year', 'month', 'tmin', 'tmax', 'tavg'], axis=1)
    summary_df.sort_values(by=['ymd'], inplace=True)

    summary_df.set_index('ymd', inplace=True)
    return summary_df


df = fetch_station_summary("USC00047965")
print(df)
