import pandas as pd
import numpy as np
import wget
import os
import logging
import argparse
import time
from typing import Any, Tuple, List
import gzip
import shutil
import matplotlib.pyplot as plt

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

#BASE_STATION = ("ISE00105694", "San Jose") # Fremont station.

STATION_LIST = [
    #("ISE00105694", "Dagan"),
    ("IS000004640", "Kenan"),
]


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
    raw_df = fetch_station_raw_data(station_id)
    # print(df_raw)

    # Extract precipitation by date, in inches.
    prcp_df = raw_df.loc[raw_df['elem'] == "PRCP"]
    prcp_df = prcp_df[['ymd', 'value']].copy()
    # Convert tenth of mm to inches.
    prcp_df['value'] = prcp_df['value'] / 254.0
    prcp_df.rename(columns={"value": 'prcp'}, inplace=True)

    # Extract tMin by date, in C.
    min_df = raw_df.loc[raw_df['elem'] == "TMIN"]
    min_df = min_df[['ymd', 'value']].copy()
    # Convert tenth of C to C.
    min_df['value'] = min_df['value'] / 10.0
    min_df.rename(columns={"value": 'tmin'}, inplace=True)
    # print(min_df)

    # Extract tMax by date, in C.
    max_df = raw_df.loc[raw_df['elem'] == "TMAX"]
    max_df = max_df[['ymd', 'value']].copy()
    # Convert tenth of C to C.
    max_df['value'] = max_df['value'] / 10.0
    max_df.rename(columns={"value": 'tmax'}, inplace=True)
    # print(max_df)

    # Compute tAvg by date, in C.
    avg_df = pd.merge(min_df, max_df, on='ymd', how='inner')
    avg_df['tavg'] = avg_df[['tmin', 'tmax']].mean(1)
    avg_df.drop(['tmin', 'tmax'], axis=1, inplace=True)
    # print(avg_df)

    # Combine prcp, tmin, tmax, tavg by date
    summary_df = pd.merge(prcp_df, min_df, on='ymd', how='outer')
    summary_df = pd.merge(summary_df, max_df, on='ymd', how='outer')
    summary_df = pd.merge(summary_df, avg_df, on='ymd', how='outer')

    # Add specific columns for year and month
    summary_df['year'] = summary_df[['ymd']].floordiv(10000)
    summary_df['month'] = summary_df[['ymd']].mod(10000).floordiv(100)
    summary_df['day'] = summary_df[['ymd']].mod(100)
    summary_df = summary_df.reindex(['ymd', 'year', 'month', 'day', 'prcp', 'tmin', 'tmax', 'tavg'], axis=1)
    summary_df.sort_values(by=['ymd'], inplace=True)

    # summary_df.set_index('ymd', inplace=True)
    # print(summary_df)
    return summary_df

def get_station_hot_days_by_year(station_id: str) -> Any:
    df1 = fetch_station_summary(station_id)
    df1['is_hot'] = np.where(df1['tmax'] >= 35, 1, 0)
    print(df1)

    df2 = df1.groupby(['year'])['is_hot'].sum().reset_index()
    print(df2)
    df2.to_csv('_report.csv', sep=',', encoding='utf-8', index=False, header=True)
    exit(0)

#def get_station_averages_by_month(staion_id: str) -> Any:
#    """Return station averages by month."""
#    df1 = fetch_station_summary(staion_id)
#    # YEAR_MIN = 2015
#    YEAR_MIN = 2012
#    YEAR_MAX = 2022
#    NUM_YEARS = (YEAR_MAX - YEAR_MIN) + 1
#    df1 = df1.loc[(df1['year'] >= YEAR_MIN) & (df1['year'] <= YEAR_MAX)]
#    df1.drop(['ymd', 'year'], axis=1, inplace=True)
#    df1 = df1.groupby(['month']).agg({'prcp': 'sum',
#                                      'tmin': 'mean',
#                                      'tmax': 'mean',
#                                      'tavg': 'mean'})
#    # Average percipitation per month.
#    df1['prcp'] = df1['prcp'] / NUM_YEARS
#
#    return df1


def compare_stations_tavg(stations: List[Tuple]) -> Any:
    logger.info("Comparing temperatures.")

    #df0 = get_station_averages_by_month(stations[0])
    df0 = get_station_hot_days_by_year(stations[0][0])

    print(df0)
    exit(0)

    #df0.drop(['prcp', 'tmin', 'tmax'], axis=1, inplace=True)
    #df0['tavg'] = (df0['tavg'] * 9 / 5) + 32
    #merged = None
    #for station in stations:
    #    df1 = get_station_averages_by_month(station[0])
    #    df1.drop(['prcp', 'tmin', 'tmax'], axis=1, inplace=True)
    #    # print("************")
    #    # print(df1)
    #    df1['tavg'] = (df1['tavg'] * 9 / 5) + 32
    #    df1 = df1 - df0
    #    df1.rename(columns={"tavg": station[1]}, inplace=True)
    #    # print(df1)
    #    if merged is None:
    #        # print(df1)
    #        merged = df1.copy()
    #    else:
    #        merged = pd.merge(merged, df1, on='month', how='outer')
    return merged


fig, ax = plt.subplots(nrows=1, ncols=1, gridspec_kw={"hspace": 0.5})

# plt.subplots_adjust( hspace=0.2)


df = compare_stations_tavg(STATION_LIST)
# ax[0].hlines(y=0.0,  linewidth=1, color='gray')
df.plot(ax=ax[0], title=f"Temp [F] compared to station [{BASE_STATION[1]}] ").axhline(y=0.0, color='gray',
                                                                                      linestyle='--')

# plt.legend(fontsize = 7)

# ax.legend(fontsize=10)

#df = compare_stations_prcp(BASE_STATION, STATION_LIST)
#df.plot(ax=ax[1], title=f"Precipitation [Inch] compared to station [{BASE_STATION[1]}] ").axhline(y=0.0, color='gray', linestyle='--')

plt.show()
