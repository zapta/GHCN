"""Report annual hot days per station."""

import sys
import logging
import argparse
import pandas as pd
from pandas import DataFrame
import numpy as np
import matplotlib.pyplot as plt
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


STATIONS = [
    ("USC00043875", "Healdsburg, CA"),
]


def dump_df(df: DataFrame, msg: str):
    """Dump the head and tail of a dataframe."""
    print()
    print(msg)
    print("-" * len(msg))
    print(df.to_string(index=False, max_rows=10))
    print()


def main() -> int:
    """Main function."""

    ghcn_cache = GhcnCache(args.data_cache_dir, args.force_fetch)

    station = STATIONS[0]

    # df = ghcn_cache.fetch_station_raw_data(station_id)
    df = ghcn_cache.fetch_station_summary(station[0])
    df = df[df["year"].between(1900, 2025)]

    # print(df)

    temp_f = 100
    temp_c = (temp_f - 32) * 5 / 9
    df["is_hot"] = np.where(df["tmax"] >= temp_c, 1, 0)
    dump_df(df, "Station data")

    df2 = df.groupby(["year"])["is_hot"].sum().reset_index()
    df2 = df2.rename(columns={"is_hot": "hot_days"})
    dump_df(df2, "Hot days per year")

    df2.to_csv("_report.csv", sep=",", encoding="utf-8", index=False, header=True)

    ax = df2.plot(x="year", y="hot_days", kind="scatter", figsize=(12, 5))

    # ax.plot(df2["year"], df2["hot_days_trend"], color="red", linewidth=2, label="Trend")

    z2 = np.polyfit(df2["year"], df2["hot_days"], 2)
    ax.plot(df2["year"], np.poly1d(z2)(df2["year"]), "g--", label="Quadratic")

    plt.title(f"GHCN station {station[0]} ({station[1]})")
    plt.xlabel("Year")
    plt.ylabel(f"Number of hot days ({temp_f}F+)")
    plt.grid(True)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    sys.exit(main())
