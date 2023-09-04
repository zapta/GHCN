import pandas as pd
import wget
import os
import logging

# ghcn files formats
# https://docs.opendata.aws/noaa-ghcn-pds/readme.html


logging.basicConfig(
    level=logging.INFO,
    format="%(relativeCreated)07d %(levelname)-7s %(filename)-10s: %(message)s",
)
logger = logging.getLogger("main")









pd.set_option('display.max_columns', 15)
pd.set_option('display.width', 200)


tmp_stations_file = "_tmp_stations.txt"
if os.path.exists(tmp_stations_file):
    logger.info("Deleting the old GHCN stations file.")
    os.remove(tmp_stations_file)
logger.info("Loading GHCN stations file.")
stations_file_url = 'ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/ghcnd-stations.txt'
tmp_stations_file2 = wget.download(stations_file_url, tmp_stations_file)
assert tmp_stations_file2 == tmp_stations_file

# ID	1-11	Character	EI000003980
# LATITUDE	13-20	Real	55.3717
# LONGITUDE	22-30	Real	-7.3400
# ELEVATION	32-37	Real	21.0
# STATE	39-40	Character
# NAME	42-71	Character	MALIN HEAD
# GSN FLAG	73-75	Character	GSN
# HCN/CRN FLAG	77-79	Character
# WMO ID	81-85	Character	03980

column_specs = [(1,11), (13,20), (22,30), (32,37), (39,40), (42,71), (73,75), (77, 79), (81, 85)]
column_names = ['id', 'lat', 'lon', 'elev', 'state' , 'name', 'x1' , 'x2', 'x3']
logger.info("Reading GHCN stations file.")
# df = pd.read_fwf(tmp_stations_file , infer_nrows=1000,  names=['id', 'lat', 'lon', 'elevation', 'state' , 'name', 'x3' , 'x4'], header=None)
df = pd.read_fwf(tmp_stations_file , colspecs =column_specs,
                 names=column_names, header=None)
print(f"{df}")


