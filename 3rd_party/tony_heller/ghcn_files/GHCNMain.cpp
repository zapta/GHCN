//--------------------------------------------------------------------------------------
// Main.cpp
// Code for parsing and processing daily GHCN data from
// ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/ghcnd_hcn.tar.gz
// Written by Steve Goddard
// If you modify it and mess it up,don't blame it on me

// The algorthim for locating records has no time bias.
// All years which share in the min or max record get counted.
// You can run the algorithm forwards,backwards or randomly
// and you will always get the same result.

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <math.h>

#include "GHCN.h"

static const int MAX_DATE_DUMP_YEARS = 6;

std::map<size_t,bool> months_under_test_map;
std::map<size_t,bool> days_under_test_map;
size_t most_recent_year = 0;

size_t baseline_start_year = 1840;
size_t baseline_end_year = 2100;

float g_temperature_threshold_high = 37.8f;
float g_temperature_threshold_low = -18;
int g_first_year = 1850;
int g_last_year = 2017;
float g_precipitation_target = 1.0f;
float g_snow_target = 1.0f;
size_t g_month_to_dump = 0;
size_t g_day_to_dump = 0;


GRAPHING_OPTIONS g_current_graphing_option = GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY;
std::pair<size_t,float> g_result_pair[NUMBER_OF_GRAPHING_OPTIONS][MAX_NUMBER_OF_DATA_POINTS];
std::pair<size_t,float> g_result_pair_2[NUMBER_OF_GRAPHING_OPTIONS][MAX_NUMBER_OF_DATA_POINTS];

size_t dump_year = 0;
size_t dump_year_2 = 0;

float cmToIn(float cm)
{
    return cm / 2.54;
}

float InToCm(float in)
{
    return in * 2.54;
}

float cToF(float degrees_c)
{
    if (degrees_c == UNKNOWN_TEMPERATURE)
    {
        return UNKNOWN_TEMPERATURE;
    }
    return (degrees_c * 1.8f) + 32.0f;
}

float fToC(float degrees_f)
{
    if (degrees_f == UNKNOWN_TEMPERATURE)
    {
        return UNKNOWN_TEMPERATURE;
    }
    return (degrees_f - 32.0f) / 1.8f;
}

size_t getNumberOfDaysInAMonth(size_t month_number)
{
    if (
            month_number == 1
         || month_number == 3
         || month_number == 5
         || month_number == 7
         || month_number == 8
         || month_number == 10
         || month_number == 12
       )
    {
        return 31;
    }
    else if (month_number == 2)
    {
        return 29;
    }
    else
    {
        return 30;
    }
}

bool stationActiveForTheEntireDateRange(int start_date_for_records, int end_date_for_records, Station station)
{

    if (start_date_for_records == 0)
    {
        return true;
    }

    if (station.getYearContainsTemperatureDataMap()[start_date_for_records] &&
            station.getYearContainsTemperatureDataMap()[end_date_for_records])
    {

        return true;
    }

    std::map<size_t,bool>::iterator it = station.getYearContainsTemperatureDataMap().begin();

//    for (; it != station.getYearContainsTemperatureDataMap().end(); it++)
//    {
//        std::cerr << it->first << "," << it->second << std::endl;
//    }

    return false;
}

size_t getNumberOfDaysUnderTest(int year)
{
    bool leap_year = ( (year % 400) == 0 ) ? true : ( (year % 100) == 0 ) ? false : (year % 4) ? false : true;

    if (days_under_test_map.size() != 0)
    {
        return days_under_test_map.size();
    }

    if (months_under_test_map.size() == 0)
    {
        if (leap_year)
        {
            return 366;
        }
        else
        {
            return 365;
        }
    }
    else
    {
        int total_number_of_days = 0;

        for (unsigned int month = 1; month <= NUMBER_OF_MONTHS_PER_YEAR; month++)
        {            if (months_under_test_map[month])
            {
                total_number_of_days += NUMBER_OF_DAYS_PER_MONTH[month - 1];

                if (month == 2 && !leap_year)
                {
                    total_number_of_days--;
                }
            }
        }

        return total_number_of_days;
    }
}

int ghcnmain(std::vector<std::string> argv)
{
    int argc = (int)argv.size();

    months_under_test_map.clear();
    days_under_test_map.clear();

    if (argc < 2)
    {
        std::cerr << "Usage : ghcn.exe GHCN_DATA_FILE_NAME [options]" << std::endl;
        std::cerr << "Options :" << std::endl;
        std::cerr << "           year=YYYY : Year under test" << std::endl;
        std::cerr << "           month=MM  : Month under test" << std::endl;
        std::cerr << "           months=[0-12] : Number of months under test" << std::endl;
        std::cerr << "           state=[AL-WY] : Abbreviation of US state under test" << std::endl;
        std::cerr << "           station=[6 digit HCN number] : US station under test" << std::endl;
        std::cerr << "           threshold_high=[integer] : target maximum temperature" << std::endl;
        std::cerr << "           threshold_low=[integer] : target minimum temperature" << std::endl;
        std::cerr << "           start=[YYYY} : Only use stations with records from YYYY and the current year" << std::endl;
        std::cerr << "           date=[MMDD] : Collect statistics from day MMDD for each year" << std::endl;
        std::cerr << "           date=[MMDDYYYY] : Collect statistics from day MMDD for year YYYY" << std::endl;
        std::cerr << "           through=[MMDD] : Collect stats from Jan 1 through MMDD" << std::endl;
        return (1);
    }

    std::string input_file_name_string = argv[1];

    size_t year_under_test = 0;
    size_t month_under_test = 0;
    size_t day_under_test = 0;
    size_t months_under_test = 1;
    size_t year_to_dump = 0;
    size_t year_for_stats = 0;
    size_t through_month = 13;
    size_t through_day = 35;
    std::string state_under_test = "";
    int number_of_months_under_test = 12;
    int stride = 1;
    size_t seed;
    std::string station_under_test = "";
    int start_date_for_records = 0;
    int end_date_for_records = 0;
    bool test_only_airports = false;
    bool test_only_non_airports = false;
    size_t number_of_days_with_valid_maximum_temperatures = 0;
    std::string arguments_string;

    for (int i = 2; i < argc; i++)
    {
        std::string argument_string = argv[i];
        arguments_string += ("_" + argument_string);
        //std::cerr << argument_string << std::endl;

        if ( argument_string.substr(0,5).find("year=") != std::string::npos )
        {
            std::string year_string = argument_string.substr(5,4);
            year_under_test = (size_t)strtol(year_string.c_str(),NULL,10);
            std::cout << year_string << std::endl;
            std::cerr << year_string << std::endl;
        }
        else if ( argument_string.find("stats_year=") != std::string::npos )
        {
            //std::cerr << argument_string << std::endl;
            std::string year_string = argument_string.substr(11,4);
            year_for_stats = (size_t)strtol(year_string.c_str(),NULL,10);
            std::cout << "Year for stats " << year_for_stats << std::endl;
            std::cerr << "Year for stats " << year_for_stats << std::endl;
        }
        else if ( argument_string.find("dump_year=") != std::string::npos )
        {
            //std::cerr << argument_string << std::endl;
            std::string year_string = argument_string.substr(10,4);
            dump_year = (size_t)strtol(year_string.c_str(),NULL,10);
            std::cout << "Dump year " << dump_year << std::endl;
            std::cerr << "Dump year " << dump_year << std::endl;
        }
        else if ( argument_string.find("month=") != std::string::npos )
        {
            std::string month_string = argument_string.substr(6,2);
            month_under_test = (size_t)strtol(month_string.c_str(),NULL,10);
            months_under_test_map[month_under_test] = true;
            std::cout << "Month " << month_string << std::endl;
            std::cerr << "Month " << month_string << std::endl;
        }
        else if ( argument_string.find("months=") != std::string::npos )
        {
            std::string months_string = argument_string.substr(7,2);
            months_under_test = (size_t)strtol(months_string.c_str(),NULL,10);
            std::cout << "Number of months " << months_under_test << std::endl;
            std::cerr << "Number of months " << months_under_test << std::endl;

            for (size_t i = 0; i < months_under_test; i++)
            {
                size_t valid_month = ( month_under_test + i ) % 12;
                valid_month = (valid_month == 0) ? 12 : valid_month;
                months_under_test_map[valid_month] = true;
                std::cerr << "Month under test " << valid_month << std::endl;
            }
        }
        else if ( argument_string.find("spring") != std::string::npos )
        {
            month_under_test = 3;
            months_under_test_map[3] = true;
            months_under_test_map[4] = true;
            months_under_test_map[5] = true;
            std::cerr << "spring" << std::endl;
        }
        else if ( argument_string.find("summer") != std::string::npos )
        {
            month_under_test = 6;
            months_under_test_map[6] = true;
            months_under_test_map[7] = true;
            months_under_test_map[8] = true;
            std::cerr << "summer" << std::endl;
        }
        else if ( argument_string.find("fall") != std::string::npos )
        {
            month_under_test = 9;
            months_under_test_map[0] = true;
            months_under_test_map[10] = true;
            months_under_test_map[11] = true;
            std::cerr << "fall" << std::endl;
        }
        else if ( argument_string.find("day=") != std::string::npos )
        {
            std::string day_string = argument_string.substr(4,2);
            day_under_test = (size_t)strtol(day_string.c_str(),NULL,10);
            days_under_test_map[day_under_test] = true;
            std::cout << "day = " << day_string << std::endl;
            std::cerr << "day = " << day_string << std::endl;
        }
        else if ( argument_string.find("days=") != std::string::npos )
        {
            std::string days_string = argument_string.substr(5,2);
            int days_under_test = (size_t)strtol(days_string.c_str(),NULL,10);
            std::cout << "Number of days " << days_under_test << std::endl;
            std::cerr << "Number of days " << days_under_test << std::endl;

            for (size_t i = 0; i < days_under_test; i++)
            {
                days_under_test_map[day_under_test + i] = true;
                std::cerr << "Days under test " << day_under_test + i << std::endl;
            }
        }
        else if ( argument_string.find("state=") != std::string::npos )
        {
            state_under_test = argument_string.substr(6,2);
        }
        else if ( argument_string.find("station=") != std::string::npos )
        {
            station_under_test = argument_string.substr(8,6);
            std::cout << "Station under test = " << station_under_test << std::endl;
            std::cerr << "Station under test = " << station_under_test << std::endl;
        }
        else if ( argument_string.find("period=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string period_string = argument_string.substr(7,length - 7);
            number_of_months_under_test = (int)strtol(period_string.c_str(),NULL,10);
            std::cout << period_string << std::endl;
            std::cerr << period_string << std::endl;
        }
        else if ( argument_string.find("seed=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string seed_string = argument_string.substr(5,length - 5);
            seed = (int)strtol(seed_string.c_str(),NULL,10);
            std::cout << "Seed = " << seed_string << std::endl;
            std::cerr << "Seed = " << seed_string << std::endl;
            srand((unsigned int)seed);
        }
        else if ( argument_string.find("stride=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string stride_string = argument_string.substr(7,length - 7);
            stride = (int)strtol(stride_string.c_str(),NULL,10);
            std::cout << "Stride = " << stride_string << std::endl;
            std::cerr << "Stride = " << stride_string << std::endl;
        }
        else if ( argument_string.find("first_year=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string first_year_string = argument_string.substr(11, length - 11);
            g_first_year = (int)strtol(first_year_string.c_str(),NULL,10);
            std::cout << "First year = " << g_first_year << std::endl;
            std::cerr << "First year = " << g_first_year << std::endl;
        }
        else if ( argument_string.find("last_year=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string last_year_string = argument_string.substr(10, length - 10);
            g_last_year = (int)strtol(last_year_string.c_str(),NULL,10);
            std::cout << "Last year = " << g_last_year << std::endl;
            std::cerr << "Last year = " << g_last_year << std::endl;
        }
        else if ( argument_string.find("threshold_high=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string threshold_string = argument_string.substr(15, length - 15);
            g_temperature_threshold_high = (float)strtof( threshold_string.c_str(), NULL );
            std::cout << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
            std::cerr << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
        }
        else if ( argument_string.find("threshold_low=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string threshold_string = argument_string.substr(14,length - 14);
            g_temperature_threshold_low = (float)strtof(threshold_string.c_str(), NULL);
            std::cout << "Temperature threshold low = " << g_temperature_threshold_low << std::endl;
            std::cerr << "Temperature threshold low = " << g_temperature_threshold_low << std::endl;
        }
        else if ( argument_string.find("thresh_high=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string threshold_string = argument_string.substr(12,length - 12);
            g_temperature_threshold_high = (float)strtof(threshold_string.c_str(), NULL);
            std::cout << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
            std::cerr << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
        }
        else if ( argument_string.find("thresh=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string threshold_string = argument_string.substr(7,length - 7);
            g_temperature_threshold_high = (float)strtof(threshold_string.c_str(), NULL);
            std::cout << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
            std::cerr << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
        }
        else if ( argument_string.find("threshf=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string threshold_string = argument_string.substr(8,length - 8);
            g_temperature_threshold_high = (float)strtof(threshold_string.c_str(), NULL);
            g_temperature_threshold_high = fToC(g_temperature_threshold_high);
            std::cout << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
            std::cerr << "Temperature threshold high = " << g_temperature_threshold_high << std::endl;
        }
        else if (
                 argument_string.find("thresh_low=") != std::string::npos
                 || argument_string.find("thresh_min=") != std::string::npos
                 )
        {
            size_t length = argument_string.size();
            std::string threshold_string = argument_string.substr(11,length - 11);
            g_temperature_threshold_low = (float)strtof(threshold_string.c_str(), NULL);
            std::cout << "Temperature threshold low = " << g_temperature_threshold_low << std::endl;
            std::cerr << "Temperature threshold low = " << g_temperature_threshold_low << std::endl;
        }
        else if ( argument_string.find("start=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string start_string = argument_string.substr(6,length - 6);
            start_date_for_records = (int)strtol(start_string.c_str(),NULL,10);
            std::cout << "Records must include " << start_string << std::endl;
            std::cerr << "Records must include " << start_string << std::endl;
        }
        else if ( argument_string.find("begin=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string start_string = argument_string.substr(6,length - 6);
            start_date_for_records = (int)strtol(start_string.c_str(),NULL,10);
            std::cout << "Records must include " << start_string << std::endl;
            std::cerr << "Records must include " << start_string << std::endl;
        }
        else if ( argument_string.find("end=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string end_string = argument_string.substr(4,length - 4);
            end_date_for_records = (int)strtol(end_string.c_str(),NULL,10);
            std::cout << "Records must include " << end_string << std::endl;
            std::cerr << "Records must include " << end_string << std::endl;
        }
        else if ( argument_string.find("stop=") != std::string::npos )
        {
            size_t key_length = strlen("stop=");
            size_t length = argument_string.size();
            std::string end_string = argument_string.substr(key_length,length - key_length);
            end_date_for_records = (int)strtol(end_string.c_str(),NULL,10);
            std::cout << "Records must include " << end_string << std::endl;
            std::cerr << "Records must include " << end_string << std::endl;
        }
        else if ( argument_string.find("baseline=") != std::string::npos )
        {
            std::string begin_string = argument_string.substr(9, 4);
            baseline_start_year = (int)strtol(begin_string.c_str(),NULL,10);
            std::string end_string = argument_string.substr(13, 4);
            baseline_end_year = (int)strtol(end_string.c_str(),NULL,10);
            std::cout << "Baseline " << begin_string << " to " << end_string << std::endl;
            std::cerr << "Baseline " << begin_string << " to " << end_string << std::endl;
        }
        else if ( argument_string.find("airports=") != std::string::npos )
        {
            std::string yes_string = argument_string.substr(9,1);
            
            if ( yes_string == "y" )
            {
                test_only_airports = true;
                std::cout << "Airports only" << std::endl;
                std::cerr << "Airports only" << std::endl;
            }
            
            if ( yes_string == "n" )
            {
                test_only_non_airports = true;
                std::cout << "No airports" << std::endl;
                std::cerr << "No airports" << std::endl;
            }
        }
        else if ( argument_string.find("date=") != std::string::npos )
        {
            std::string dump_date_string = argument_string.substr(5,4);
            g_month_to_dump = (size_t)strtol(dump_date_string.substr(0,2).c_str(),NULL,10);
            g_day_to_dump = (size_t)strtol(dump_date_string.substr(2,2).c_str(),NULL,10);
            std::cout << "Month = " << g_month_to_dump << " Day = " << g_day_to_dump;
            std::cerr << "Month = " << g_month_to_dump << " Day = " << g_day_to_dump;
            month_under_test = g_month_to_dump;
            months_under_test_map[month_under_test] = true;
            day_under_test = g_day_to_dump;
            days_under_test_map[day_under_test] = true;

            if ( argument_string.size() == 13 )
            {
                year_to_dump = (size_t)strtol(argument_string.substr(9,4).c_str(),NULL,10);
                year_for_stats = year_to_dump;
                year_under_test = year_to_dump;
                std::cout << " Year = " << year_to_dump;
                std::cerr << " Year = " << year_to_dump;
            }

            std::cout << std::endl;
            std::cerr << std::endl;
        }
        else if ( argument_string.find("through=") != std::string::npos )
        {
            std::string through_date_string = argument_string.substr(8,4);
            through_month = (size_t)strtol(through_date_string.substr(0,2).c_str(),NULL,10);
            through_day = (size_t)strtol(through_date_string.substr(2,2).c_str(),NULL,10);
            std::cout << "Through month = " << through_month << " Day = " << through_day << std::endl;
            std::cerr << "Through month = " << through_month << " Day = " << through_day << std::endl;
        }
        else if ( argument_string.find("precip=") != std::string::npos )
        {
            size_t length = argument_string.size();
            std::string precip_string = argument_string.substr(7, length - 7);
            g_precipitation_target = (float)atof( precip_string.c_str() );
            std::cout << "Precipitation target = " << g_precipitation_target << std::endl;
            std::cerr << "Precipitation target = " << g_precipitation_target << std::endl;
        }
        else if ( argument_string.find("snow=") != std::string::npos )
        {
            size_t length = argument_string.size();
            size_t arg_length = strlen("snow=");
            std::string precip_string = argument_string.substr(arg_length, length - arg_length);
            g_snow_target = (float)atof( precip_string.c_str() );
            std::cout << "Snow target = " << g_snow_target << std::endl;
            std::cerr << "Snow target = " << g_snow_target << std::endl;
        }
    }

    replaceString(arguments_string, "=", "_");

    size_t number_of_countries = sizeof(COUNTRY_NAMES) / (MAX_STATE_NAME_LENGTH * 2);

    std::map<std::string,std::string> country_name_map;

    for (size_t i = 0; i < number_of_countries; i++)
    {
        country_name_map[ COUNTRY_NAMES[i][0] ] = COUNTRY_NAMES[i][1];
    }

    // Read in the station information
    std::map<std::string,std::string> station_name_map;
    std::map<std::string,std::string> state_name_map;
    std::map<std::string,int> state_number_map;
    std::map<std::string,std::string> latitude_map;
    std::map<std::string,std::string> longitude_map;
    std::string record_string;
    // ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/ghcnd-stations.txt
    std::ifstream ghcn_station_file("ghcnd-stations.txt");

    for (int i = 0; i < NUMBER_OF_STATES; i++)
    {
        state_number_map[std::string(STATE_ABREVIATIONS[i])] = i;
    }

    if ( ghcn_station_file.is_open() )
    {
        while ( ghcn_station_file.good() )
        {
            getline(ghcn_station_file,record_string);

            if (record_string.length() < 85)
            {
                continue;
            }

            std::string station_number = record_string.substr(0,11);
            std::string latitude = record_string.substr(12, 8);
            latitude_map[station_number] = latitude;
            std::string longitude = record_string.substr(21, 8);
            longitude_map[station_number] = longitude;
            std::string state_name = record_string.substr(38,2);
            std::string station_name = record_string.substr(41,35);
            std::replace(station_name.begin(), station_name.end(), '&', ' ');
            station_name_map[station_number] = station_name;

            //std::cout << station_number << "," << station_name << "," << state_name << std::endl;

            if ( station_number.substr(0,2) == "US")
            {
                state_name_map[station_number] = state_name;
                //std::cout << state_name << std::endl;
            }
        }

        ghcn_station_file.close();
    }
    else
    {
        std::cout << "Unable to open ghcnd-stations.txt" << std::endl;
    }

    // read in the station data
    // http://cdiac.ornl.gov/ftp/ushcn_daily/
    std::ifstream ghcn_data_file( input_file_name_string.c_str() );

    Country country;
    std::string current_station_number = "";
    unsigned int current_state_number = UINT_MAX;
    unsigned int current_year_number = 0;
    bool check_ghcn = true;
    size_t station_index = 1;
    size_t record_number = 0;

    // Read in the temperature database
    if ( ghcn_data_file.is_open() )
    {
        Station* new_station = new Station;
        bool first_station = true;
        size_t number_of_consecutive_days_above_max_threshold = 0;
        size_t number_of_consecutive_days_below_max_threshold = 0;
        size_t number_of_consecutive_days_below_min_threshold = 0;
        size_t number_of_consecutive_days_above_min_threshold = 0;

        while ( ghcn_data_file.good() )
        {
            getline(ghcn_data_file,record_string);
            //std::cout << record_string << std::endl;
            record_number++;

            char* record_char_string = (char*)record_string.c_str();

            if (check_ghcn && (record_char_string[0] < 'A' || record_char_string[0] > 'Z') )
            {
                continue;
            }

            DataRecord record;


            record.parseTemperatureRecord(new_station, record_string);

#if 0
            std::cerr << new_station->getStationName();

            for (size_t i = 0; i < NUMBER_OF_MONTHS_PER_YEAR; i++)
            {
                std::cerr << " " << new_station->getMinBaselineTotalPerMonthVector()[i];
            }

            std::cerr << std::endl;
#endif

            if (year_under_test && record.getYear() != year_under_test)
            {
                continue;
            }

            if (record.getYear() < (unsigned int)g_first_year &&
                    g_current_graphing_option != GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_PRECIPITATION
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_SNOWFALL)
            {
                continue;
            }

            if (record.getYear() > (unsigned int)g_last_year && g_last_year != 0 &&
                    g_current_graphing_option != GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_PRECIPITATION
                    && g_current_graphing_option != GRAPHING_OPTION_DAILY_SNOWFALL
                    )
            {
                continue;
            }

            if ((  month_under_test && !months_under_test_map[ record.getMonth() ] ) || ( record.getMonth() > through_month) )
            {
                continue;
            }

            // Uncomment this if you want to see the station info printed as the file is parsed
#if 0
            if ( record.getYear() == most_recent_year )
            {
                std::cout << record.getStateName() << " " << record.getStationNumber() << " ";
                std::cout << station_name_map[ record.getStationNumber() ];
                std::cout << " " << record.getRecordTypeString();
                std::cout << " " << record.getMonth();
                std::cout << " " << record.getYear();
                std::cout << std::endl;
            }
#endif
            // Build the database

            bool state_changed = false;

            // Look for a new state
            if ( record.getStateNumber() != current_state_number )
            {
                state_changed = true;
                current_state_number = record.getStateNumber();
                std::cerr << country_name_map[ record.getCountryAbbreviation() ] << std::endl;
                std::cout << country_name_map[ record.getCountryAbbreviation() ] << std::endl;
                country.getStateVector().at(current_state_number - 1).setStateNumber(current_state_number);
            }

            bool station_changed = false;
            bool use_station = true;

            // Look for a new station
            if ( record.getStationNumber() != current_station_number || state_changed)
            {
                if (!first_station)
                {
                    new_station = new Station;
                }

                first_station = false;

                std::string old_station_number = current_station_number;
                station_changed = true;
                number_of_consecutive_days_above_max_threshold = 0;
                number_of_consecutive_days_below_max_threshold = 0;
                number_of_consecutive_days_below_min_threshold = 0;
                number_of_consecutive_days_above_min_threshold = 0;
                srand((unsigned int)(seed + record_number));
                //station_index = rand();
                station_index++;

                current_station_number = record.getStationNumber();

                if ( (station_index % stride) != 0)
                {
                    use_station = false;
                    std::cerr << "Skipping station " << station_name_map[current_station_number]  << "  " << state_name_map[current_station_number] << "  " << current_station_number << std::endl;
                }

                if ( station_under_test != "" )
                {
                    if ( current_station_number.find(station_under_test) == std::string::npos )
                    {
                        current_station_number = old_station_number;
                        continue;
                    }
                }

                if ( state_under_test != "" )
                {
                    if ( state_name_map[current_station_number] != state_under_test )
                    {
                        current_station_number = old_station_number;
                        continue;
                    }
                }

#if 0
                if (state_under_test != "AK" && state_under_test != "HI")
                {
                    if ( state_name_map.count(current_station_number) )
                    {
                        if ( state_name_map[current_station_number] == "AK" || state_name_map[current_station_number] == "HI" )
                        {
                            current_station_number = old_station_number;
                            continue;
                        }
                    }
                }
#endif


                new_station->setStationNumber(current_station_number);
                new_station->setStateName( state_name_map[current_station_number] );
                new_station->setStationName( station_name_map[current_station_number] );
                new_station->setLatitude( latitude_map[current_station_number] );
                new_station->setLongitude( longitude_map[current_station_number] );

                if (test_only_airports || test_only_non_airports)
                {
                    if (   new_station->getStationName().find(" AP") != std::string::npos
                           || new_station->getStationName().find(" AF") != std::string::npos
                           || new_station->getStationName().find(" FLD") != std::string::npos
                           || new_station->getStationName().find(" AFB") != std::string::npos
                           || new_station->getStationName().find(" ASC") != std::string::npos
                           || new_station->getStationName().find(" AAF") != std::string::npos
                           || new_station->getStationName().find(" NAF") != std::string::npos
                           || new_station->getStationName().find(" NAS") != std::string::npos
                           || new_station->getStationName().find(" NAAS") != std::string::npos
                           || new_station->getStationName().find(" NAAF") != std::string::npos
                           || new_station->getStationName().find(" BASE") != std::string::npos
                           || new_station->getStationName().find(" CAA") != std::string::npos
                           || new_station->getStationName().find("RGNL A") != std::string::npos
                           || new_station->getStationName().find("AIRFIELD") != std::string::npos
                           || new_station->getStationName().find("AIRPORT") != std::string::npos
                           || new_station->getStationName().find("INTL") != std::string::npos
                           || new_station->getStationName().find("PILOT") != std::string::npos
                           || new_station->getStationName().find("AIR PK") != std::string::npos
                           )

                    {
                        if (test_only_non_airports)
                        {
                            current_station_number = old_station_number;
                            continue;
                        }
                    }
                    else
                    {
                        if (test_only_airports)
                        {
                            current_station_number = old_station_number;
                            continue;
                        }
                    }
                }

                if (use_station)
                {
                    std::cerr << new_station->getStationName() << "  " << state_name_map[current_station_number] << "  " << current_station_number << std::endl;
                    country.getStateVector().at(current_state_number - 1).getStationVector().push_back(new_station);
                }
            }

            if ( (station_index % stride) != 0)
            {
                use_station = false;
            }

            bool year_changed = false;

            if ( !use_station || country.getStateVector().at(current_state_number - 1).getStationVector().size() == 0 )
            {
                continue;
            }

            // Look for a new year
            if ( record.getYear() != current_year_number || state_changed || station_changed )
            {
                year_changed = true;
                current_year_number = record.getYear();
                Year new_year;
                new_year.setYear(current_year_number);
                number_of_consecutive_days_above_max_threshold = 0;
                number_of_consecutive_days_below_max_threshold = 0;
                number_of_consecutive_days_below_min_threshold = 0;
                number_of_consecutive_days_above_min_threshold = 0;


                if ( country.getStateVector().at(current_state_number - 1).getStationVector().size() )
                {
                    country.getStateVector().at(current_state_number - 1).getStationVector().back()->getYearVector().push_back(new_year);
                }
            }

            State& current_state = country.getStateVector().at(current_state_number - 1);
            Station& current_station = *(current_state.getStationVector().back());
            Year& current_year = current_station.getYearVector().back();
            Month& current_month = current_year.getMonthVector().at( record.getMonth() - 1 );
            current_month.setMonthNumber( record.getMonth() );

            // read in the records for each day of the month
            if (   use_station &&
                   (
                       record.getRecordTypeString() == "TMAX"
                       || record.getRecordTypeString() == "TMIN"
                       || record.getRecordTypeString() == "SNOW"
                       || record.getRecordTypeString() == "PRCP"
                       )
                   )
            {
                //std::cerr << current_station.getStationName() << std::endl;
                for (size_t day_number = 0; day_number < MAX_DAYS_IN_MONTH; day_number++)
                {
                    float high_temperature = record.getHighTemperature((unsigned int)day_number);
                    float low_temperature = record.getLowTemperature((unsigned int)day_number);
                    float snow = record.getSnowfall((unsigned int)day_number);
                    float precipitation = record.getSnowfall((unsigned int)day_number);
                    Day& day = current_month.getDayVector().at(day_number);
                    size_t day_of_year = record.getDayOfYearForStartOfRecord() + day_number;
                    day.setDayOfYear(day_of_year);
                    day.setStationName( current_station.getStationName() );

                    if (  day_under_test != 0 && !days_under_test_map[ day_number + 1 ] )
                    {
                        continue;
                    }


                    if (   ( (record.getMonth() == through_month) && (day_number > through_day) )
                           || ( record.getMonth() > through_month )
                           )
                    {
                        continue;
                    }

                    if ( record.getRecordTypeString() == "TMAX" &&
                         high_temperature != UNKNOWN_TEMPERATURE &&
                         high_temperature < UNREASONABLE_HIGH_TEMPERATURE )
                    {
                        number_of_days_with_valid_maximum_temperatures++;
                        current_station.getYearContainsTemperatureDataMap()[current_year_number] = true;
                        day.setMaxTemperature(high_temperature);
                        //std::cout << high_temperature << std::endl;

                        if (high_temperature < g_temperature_threshold_high)
                        {
                            current_year.incrementNumberOfDaysBelowMaxThreshold();
                            number_of_consecutive_days_above_max_threshold = 0;
                            number_of_consecutive_days_below_max_threshold++;

                            if (number_of_consecutive_days_below_max_threshold >
                                    current_year.getNumberOfConsecutiveDaysBelowMaxThreshold() )
                            {
                                //                    			std::cerr << " Setting max days below to " << number_of_consecutive_days_below_max_threshold;
                                //                    			std::cerr << " for station " << current_station.getStationName();
                                //                    			std::cerr << " high temperature "<< high_temperature << " threshold " << g_temperature_threshold_high << std::endl;
                                current_year.setNumberOfConsecutiveDaysBelowMaxThreshold(number_of_consecutive_days_below_max_threshold);
                            }
                        }
                        else
                        {
                            current_year.incrementNumberOfDaysAboveMaxThreshold();
                            number_of_consecutive_days_below_max_threshold = 0;
                            number_of_consecutive_days_above_max_threshold++;

                            if (number_of_consecutive_days_above_max_threshold >
                                    current_year.getNumberOfConsecutiveDaysAboveMaxThreshold() )
                            {
                                current_year.setNumberOfConsecutiveDaysAboveMaxThreshold(number_of_consecutive_days_above_max_threshold);
                            }
                        }

                        if ( high_temperature > country.getRecordMaxTemperature() )
                        {
                            country.setRecordMaxTemperature( high_temperature );
                            country.setRecordMaxYear( current_year_number );
                        }

                        if ( high_temperature > current_state.getRecordMaxTemperature() )
                        {
                            current_state.setRecordMaxTemperature( high_temperature );
                            current_state.setRecordMaxYear( current_year_number );
                        }

                        if ( high_temperature > current_station.getRecordMaxTemperature() )
                        {
                            current_station.setRecordMaxTemperature( high_temperature );
                            current_station.setRecordMaxYear( current_year_number );
                            current_station.setRecordMaxDayOfYear( (unsigned int)day_of_year );
                            current_station.setRecordMaxMonth( record.getMonth() );
                            current_station.setRecordMaxDayOfMonth( (unsigned int)day_number );
                        }

                        if ( high_temperature > current_year.getRecordMaxTemperature() )
                        {
                            current_year.setRecordMaxTemperature( high_temperature );
                            current_year.setRecordMaxMonth( record.getMonth() );
                        }

                        if ( high_temperature > current_month.getRecordMaxTemperature() )
                        {
                            current_month.setRecordMaxTemperature( high_temperature );
                            current_month.setRecordMaxDay( (unsigned int)day_number + 1 );
                        }
                    }
                    else if ( record.getRecordTypeString() == "TMIN" &&
                              low_temperature != UNKNOWN_TEMPERATURE  &&
                              low_temperature > UNREASONABLE_LOW_TEMPERATURE )
                    {
                        current_station.getYearContainsTemperatureDataMap()[current_year_number] = true;
                        day.setMinTemperature(low_temperature);

                        if (low_temperature > g_temperature_threshold_low)
                        {
                            current_year.incrementNumberOfDaysAboveMinThreshold();
                            number_of_consecutive_days_below_min_threshold = 0;
                            number_of_consecutive_days_above_min_threshold++;

                            if (number_of_consecutive_days_above_min_threshold >
                                    current_year.getNumberOfConsecutiveDaysAboveMinThreshold() )
                            {
                                current_year.setNumberOfConsecutiveDaysAboveMinThreshold(number_of_consecutive_days_above_min_threshold);
                            }
                        }
                        else
                        {
                            number_of_consecutive_days_below_min_threshold++;
                            number_of_consecutive_days_above_min_threshold = 0;

                            if (number_of_consecutive_days_below_min_threshold >
                                    current_year.getNumberOfConsecutiveDaysBelowMinThreshold() )
                            {
                                current_year.setNumberOfConsecutiveDaysBelowMinThreshold(number_of_consecutive_days_below_min_threshold);
                            }

                            current_year.incrementNumberOfDaysBelowMinThreshold();
                        }

                        if ( low_temperature < country.getRecordMinTemperature() )
                        {
                            country.setRecordMinTemperature( low_temperature );
                            country.setRecordMinYear( current_year_number );
                        }

                        if ( low_temperature < current_state.getRecordMinTemperature() )
                        {
                            current_state.setRecordMinTemperature( low_temperature );
                            current_state.setRecordMinYear( current_year_number );
                        }

                        if ( low_temperature < current_station.getRecordMinTemperature() )
                        {
                            current_station.setRecordMinTemperature( low_temperature );
                            current_station.setRecordMinYear( current_year_number );
                            current_station.setRecordMinDayOfYear( (unsigned int)day_of_year );
                            current_station.setRecordMinMonth( record.getMonth() );
                            current_station.setRecordMinDayOfMonth( (unsigned int)day_number );
                        }

                        if ( low_temperature < current_year.getRecordMinTemperature() )
                        {
                            current_year.setRecordMinTemperature( low_temperature );
                            current_year.setRecordMinMonth( record.getMonth() );
                        }

                        if ( low_temperature < current_month.getRecordMinTemperature() )
                        {
                            current_month.setRecordMinTemperature( low_temperature );
                            current_month.setRecordMinDay( (unsigned int)day_number );
                        }
                    }
                    else if ( record.getRecordTypeString() == "SNOW" && snow != UNKNOWN_TEMPERATURE)
                    {
                        day.setSnowfall(snow);

                        if (snow > 0)
                        {
                            current_year.incrementNumberOfDaysWithSnow();
                        }
                        else
                        {
                            current_year.incrementNumberOfDaysWithoutSnow();
                        }

                        if ( snow > current_station.getRecordSnowfall() )
                        {
                            current_station.setRecordSnowfall(snow);
                            current_station.setRecordSnowfallYear( current_year_number );
                            current_station.setRecordSnowfallDayOfYear( (unsigned int)day_of_year );
                            current_station.setRecordSnowfallMonth( record.getMonth() );
                            current_station.setRecordSnowfallDayOfMonth( (unsigned int)day_number );
                        }
                    }
                    else if ( record.getRecordTypeString() == "PRCP" && precipitation != UNKNOWN_TEMPERATURE)
                    {
                        day.setPrecipitation(precipitation);

                        if ( precipitation > current_station.getRecordPrecipitation() )
                        {
                            current_station.setRecordPrecipitation(precipitation);
                            current_station.setRecordPrecipitationYear( current_year_number );
                            current_station.setRecordPrecipitationDayOfYear( (unsigned int)day_of_year );
                            current_station.setRecordPrecipitationMonth( record.getMonth() );
                            current_station.setRecordPrecipitationDayOfMonth( (unsigned int)day_number );
                        }
                    }
                }
            }
        }

        ghcn_data_file.close();

        std::vector<State>& state_vector = country.getStateVector();
        size_t state_vector_size = state_vector.size();

        // Maps to tracks records and sums

        std::map<unsigned int,double> total_latitude_per_year_map;
        std::map<unsigned int,double> total_longitude_per_year_map;
        std::map<unsigned int,unsigned int> record_max_per_year_map;
        std::map<unsigned int,unsigned int> record_min_per_year_map;
        std::map<unsigned int,unsigned int> record_precipitation_per_year_map;
        std::map<unsigned int,unsigned int> record_snowfall_per_year_map;
        std::map<unsigned int,unsigned int> record_monthly_max_per_year_map;
        std::map<unsigned int,unsigned int> record_monthly_min_per_year_map;
        std::map<unsigned int,unsigned int> record_incremental_max_per_year_map;
        std::map<unsigned int,unsigned int> record_incremental_min_per_year_map;
        std::map<unsigned int,float> total_temperature_per_year_map;
        std::map<unsigned int,float> total_snowfall_per_year_map;
        std::map<unsigned int,unsigned int> number_of_snowfalls_per_year_map;
        std::map<unsigned int,Day> maximum_snowfall_per_year_day_map;
        std::map<unsigned int,Day> maximum_precipitation_per_year_day_map;
        std::map<unsigned int,float> total_precipitation_per_year_map;
        std::map<unsigned int,unsigned int> number_of_precipitation_days_per_year_map;
        std::map<unsigned int,unsigned int> number_of_precipitation_records_per_year_map;
        std::map<unsigned int,unsigned int> number_of_readings_per_year_map;
        std::map<unsigned int,float> total_max_temperature_per_year_map;
        std::map<unsigned int,float> total_max_deviation_per_year_map;
        std::map<unsigned int,unsigned int> number_of_max_readings_per_year_map;
        std::vector< std::map<unsigned int,float> > hottest_temperature_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,float> > coldest_temperature_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,float> > total_max_deviation_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,unsigned int> > number_of_max_readings_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::map<unsigned int,unsigned int> number_of_precip_readings_per_year_map;
        std::map<unsigned int,float> total_min_temperature_per_year_map;
        std::map<unsigned int,float> total_min_deviation_per_year_map;
        std::map<unsigned int,unsigned int> number_of_min_readings_per_year_map;
        std::vector< std::map<unsigned int,float> > total_min_deviation_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,unsigned int> > number_of_min_readings_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::map<unsigned int,float> all_time_record_maximum_per_month_map;
        std::map<unsigned int,float> all_time_record_minimum_per_month_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_maximums_per_month_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_minimums_per_month_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_maximums_per_year_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_minimums_per_year_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_maximums_per_decade_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_minimums_per_decade_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_precipitations_per_year_map;
        std::map<unsigned int,unsigned int> number_of_all_time_record_snowfalls_per_year_map;
        std::map<unsigned int,unsigned int> maximum_number_of_days_without_snow_per_year_map;
        std::map<unsigned int,unsigned int> maximum_number_of_consecutive_days_above_maximum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> maximum_number_of_consecutive_days_below_minimum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> maximum_number_of_consecutive_days_above_minimum_threshold_per_year_map;
        std::map<unsigned int,Station*> maximum_number_of_consecutive_days_above_maximum_threshold_station_per_year_map;
        std::map<unsigned int,Station*> maximum_number_of_consecutive_days_below_maximum_threshold_station_per_year_map;
        std::map<unsigned int,Station*> maximum_number_of_consecutive_days_below_minimum_threshold_station_per_year_map;
        std::map<unsigned int,Station*> maximum_number_of_consecutive_days_above_minimum_threshold_station_per_year_map;
        std::map<unsigned int,unsigned int> maximum_number_of_days_without_rain_per_year_map;
        std::map<unsigned int,unsigned int> number_of_active_stations_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_above_the_maximum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_below_the_maximum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_above_the_minimum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_below_the_minimum_threshold_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_above_the_maximum_threshold_on_the_day_under_test_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_below_the_maximum_threshold_on_the_day_under_test_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_with_snow_per_year_map;
        std::map<unsigned int,unsigned int> number_of_stations_without_snow_per_year_map;
        std::map<unsigned int,unsigned int> last_spring_day_of_snow_per_year_map;
        std::map<unsigned int,unsigned int> first_autumn_day_of_snow_per_year_map;
        std::map<unsigned int,unsigned int> number_of_max_temperatures_below_low_threshold_map;
        std::map<unsigned int,unsigned int> number_of_min_temperatures_above_high_threshold_map;
        std::map<unsigned int,int> first_day_above_high_threshold_per_year_map;
        std::map<unsigned int,int> last_day_above_high_threshold_per_year_map;
        std::map<unsigned int,int> first_day_below_low_threshold_per_year_map;
        std::map<unsigned int,int> last_day_below_low_threshold_per_year_map;
        std::vector< std::map<unsigned int,int> > first_day_above_high_threshold_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,int> > last_day_above_high_threshold_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,int> > first_day_below_low_threshold_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::vector< std::map<unsigned int,int> > last_day_below_low_threshold_per_year_map_per_state_vector(NUMBER_OF_STATES);
        std::map<unsigned int,int> number_of_days_above_precipitation_target_map;
        std::map<unsigned int,int> number_of_days_above_snow_target_map;

        std::map<size_t, float> day_to_dump_max_map;
        std::map<size_t, float> day_to_dump_min_map;
        std::map<size_t, float> day_to_dump_precip_map;
        std::map<size_t, float> day_to_dump_snow_map;

        std::map<unsigned int,int> number_of_hottest_maximum_temperatures_per_year_map;
        std::map<unsigned int,float> total_hottest_maximum_temperature_per_year_map;
        std::map<unsigned int,int> number_of_coldest_minimum_temperatures_per_year_map;
        std::map<unsigned int,float> total_coldest_minimum_temperature_per_year_map;


        size_t number_of_maximum_temperature_readings[NUMBER_OF_YEARS];
        size_t number_of_maximum_temperatures_above_high_threshold[NUMBER_OF_YEARS];
        size_t number_of_maximum_temperatures_below_high_threshold[NUMBER_OF_YEARS];
        size_t number_of_maximum_temperatures_above_high_threshold_and_higher_than_previous_day[NUMBER_OF_YEARS];
        size_t number_of_minimum_temperature_readings[NUMBER_OF_YEARS];
        size_t number_of_minimum_temperatures_below_low_threshold[NUMBER_OF_YEARS];
        size_t number_of_minimum_temperatures_above_low_threshold[NUMBER_OF_YEARS];
        float total_temperature_per_month[NUMBER_OF_YEARS][NUMBER_OF_MONTHS_PER_YEAR];
        unsigned int number_of_readings_per_month[NUMBER_OF_YEARS][NUMBER_OF_MONTHS_PER_YEAR];
        float total_max_temperature_per_month[NUMBER_OF_YEARS][NUMBER_OF_MONTHS_PER_YEAR];
        unsigned int number_of_max_readings_per_month[NUMBER_OF_YEARS][NUMBER_OF_MONTHS_PER_YEAR];
        float total_min_temperature_per_month[NUMBER_OF_YEARS][NUMBER_OF_MONTHS_PER_YEAR];
        unsigned int number_of_min_readings_per_month[NUMBER_OF_YEARS][NUMBER_OF_MONTHS_PER_YEAR];

        Day empty_day;

        for (int i = 0; i < NUMBER_OF_GRAPHING_OPTIONS; i++)
        {
            for (int j = 0; j < (int)MAX_NUMBER_OF_DATA_POINTS; j++)
            {
                g_result_pair[i][j].first = 0;
                g_result_pair[i][j].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair_2[i][j].first = 0;
                g_result_pair_2[i][j].second = UNREASONABLE_LOW_TEMPERATURE;
            }
        }

        // Zero out the maps
        for (unsigned int year = FIRST_YEAR; year <= most_recent_year; year++)
        {
            number_of_hottest_maximum_temperatures_per_year_map[year] = 0;
            total_hottest_maximum_temperature_per_year_map[year] = 0.0f;
            number_of_coldest_minimum_temperatures_per_year_map[year] = 0;
            total_coldest_minimum_temperature_per_year_map[year] = 0.0f;
            total_latitude_per_year_map[year] = 0.0;
            total_longitude_per_year_map[year] = 0.0;
            record_max_per_year_map[year] = 0;
            record_min_per_year_map[year] = 0;
            record_precipitation_per_year_map[year] = 0;
            record_snowfall_per_year_map[year] = 0;
            record_monthly_max_per_year_map[year] = 0;
            record_monthly_min_per_year_map[year] = 0;
            record_incremental_max_per_year_map[year] = 0;
            record_incremental_min_per_year_map[year] = 0;
            total_temperature_per_year_map[year] = 0.0f;
            total_snowfall_per_year_map[year] = 0.0f;
            number_of_snowfalls_per_year_map[year] = 0;
            maximum_snowfall_per_year_day_map[year] = empty_day;
            maximum_precipitation_per_year_day_map[year] = empty_day;
            total_precipitation_per_year_map[year] = 0.0f;
            number_of_precipitation_days_per_year_map[year] = 0;
            number_of_precipitation_records_per_year_map[year] = 0;
            total_max_temperature_per_year_map[year] = 0.0f;
            total_min_temperature_per_year_map[year] = 0.0f;
            total_max_deviation_per_year_map[year] = 0.0f;
            total_min_deviation_per_year_map[year] = 0.0f;
            number_of_readings_per_year_map[year] = 0;
            number_of_max_readings_per_year_map[year] = 0;
            number_of_min_readings_per_year_map[year] = 0;
            number_of_precip_readings_per_year_map[year] = 0;
            first_day_above_high_threshold_per_year_map[year] = NO_VALUE;
            last_day_above_high_threshold_per_year_map[year] = NO_VALUE;
            first_day_below_low_threshold_per_year_map[year] = NO_VALUE;
            last_day_below_low_threshold_per_year_map[year] = NO_VALUE;
            number_of_days_above_precipitation_target_map[year] = 0;
            number_of_days_above_snow_target_map[year] = 0;
            number_of_maximum_temperature_readings[year - FIRST_YEAR] = 0;
            number_of_minimum_temperature_readings[year - FIRST_YEAR] = 0;
            number_of_maximum_temperatures_above_high_threshold[year - FIRST_YEAR] = 0;
            number_of_maximum_temperatures_below_high_threshold[year - FIRST_YEAR] = 0;
            number_of_maximum_temperatures_above_high_threshold_and_higher_than_previous_day[year - FIRST_YEAR] = 0;
            number_of_max_temperatures_below_low_threshold_map[year] = 0;
            number_of_min_temperatures_above_high_threshold_map[year] = 0;
            number_of_minimum_temperatures_below_low_threshold[year - FIRST_YEAR] = 0;
            number_of_minimum_temperatures_above_low_threshold[year - FIRST_YEAR] = 0;
            number_of_all_time_record_maximums_per_year_map[year] = 0;
            number_of_all_time_record_minimums_per_year_map[year] = 0;
            number_of_all_time_record_precipitations_per_year_map[year] = 0;
            number_of_all_time_record_snowfalls_per_year_map[year] = 0;
            maximum_number_of_days_without_snow_per_year_map[year] = 0;
            maximum_number_of_consecutive_days_above_maximum_threshold_per_year_map[year] = 0;
            maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[year] = 0;
            maximum_number_of_consecutive_days_below_minimum_threshold_per_year_map[year] = 0;
            maximum_number_of_consecutive_days_above_minimum_threshold_per_year_map[year] = 0;
            maximum_number_of_consecutive_days_above_maximum_threshold_station_per_year_map[year] = NULL;
            maximum_number_of_consecutive_days_below_maximum_threshold_station_per_year_map[year] = NULL;
            maximum_number_of_consecutive_days_below_minimum_threshold_station_per_year_map[year] = NULL;
            maximum_number_of_days_without_rain_per_year_map[year] = 0;
            last_spring_day_of_snow_per_year_map[year] = 0;
            first_autumn_day_of_snow_per_year_map[year] = 0;
            number_of_active_stations_per_year_map[year] = 0;
            number_of_stations_above_the_maximum_threshold_per_year_map[year] = 0;
            number_of_stations_below_the_maximum_threshold_per_year_map[year] = 0;
            number_of_stations_above_the_minimum_threshold_per_year_map[year] = 0;
            number_of_stations_below_the_minimum_threshold_per_year_map[year] = 0;
            number_of_stations_above_the_maximum_threshold_on_the_day_under_test_per_year_map[year] = 0;
            number_of_stations_below_the_maximum_threshold_on_the_day_under_test_per_year_map[year] = 0;
            number_of_stations_with_snow_per_year_map[year] = 0;
            number_of_stations_without_snow_per_year_map[year] = 0;
            day_to_dump_min_map[year] = (float)INT_MAX;
            day_to_dump_max_map[year] = (float)INT_MIN;
            day_to_dump_precip_map[year] = (float)INT_MIN;
            day_to_dump_snow_map[year] = (float)INT_MIN;

            for (size_t state = 0; state < NUMBER_OF_STATES; state++)
            {
                total_max_deviation_per_year_map_per_state_vector[state][year] = 0.0f;
                number_of_max_readings_per_year_map_per_state_vector[state][year] = 0;
                total_min_deviation_per_year_map_per_state_vector[state][year] = 0.0f;
                number_of_min_readings_per_year_map_per_state_vector[state][year] = 0;
                hottest_temperature_per_year_map_per_state_vector[state][year] = float(UNREASONABLE_LOW_TEMPERATURE);
                coldest_temperature_per_year_map_per_state_vector[state][year] = float(UNREASONABLE_HIGH_TEMPERATURE);
                first_day_above_high_threshold_per_year_map_per_state_vector[state][year] = NO_VALUE;
                last_day_above_high_threshold_per_year_map_per_state_vector[state][year] = NO_VALUE;
                first_day_below_low_threshold_per_year_map_per_state_vector[state][year] = NO_VALUE;
                last_day_below_low_threshold_per_year_map_per_state_vector[state][year] = NO_VALUE;

            }

            for (size_t month = 0; month < NUMBER_OF_MONTHS_PER_YEAR; month++)
            {
                total_temperature_per_month[year - FIRST_YEAR][month] = 0.0f;
                number_of_readings_per_month[year - FIRST_YEAR][month] = 0;
                total_max_temperature_per_month[year - FIRST_YEAR][month] = 0.0f;
                number_of_max_readings_per_month[year - FIRST_YEAR][month] = 0;
                total_min_temperature_per_month[year - FIRST_YEAR][month] = 0.0f;
                number_of_min_readings_per_month[year - FIRST_YEAR][month] = 0;

                size_t absolute_month = (year * NUMBER_OF_MONTHS_PER_YEAR) + month;
                all_time_record_maximum_per_month_map[(unsigned int)absolute_month] = UNREASONABLE_LOW_TEMPERATURE;
                all_time_record_minimum_per_month_map[(unsigned int)absolute_month] = UNREASONABLE_HIGH_TEMPERATURE;
                number_of_all_time_record_maximums_per_month_map[(unsigned int)absolute_month] = 0;
                number_of_all_time_record_minimums_per_month_map[(unsigned int)absolute_month] = 0;
            }
        }

        std::cout << "Stations used in this analysis" << std::endl;
        std::cout << "Station name,State,Station Number,First Year,Last Year";
        std::cout << ",Record Max Year,Record Max Date,Record Max";
        std::cout << ",Record Min Year,Record Min Date,Record Min";
        std::cout << ",Record Precipitation Year,Record Precipitation Date,Record Precipitation";
        std::cout << ",Record Snowfall Year,Record Snowfall Date,Record Snowfall";

        if (g_month_to_dump && year_to_dump)
        {
            std::cout << ",Date,Max C,Max F,Min C,Min F,Precip cm,Snow cm";
        }
        std::cout << std::endl;

        size_t used_station_count = 0;

        // Walk through all temperature records
        for (size_t state_number = 0; state_number < state_vector_size; state_number++)
        {
            std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();
            size_t station_vector_size = station_vector.size();

            for (size_t station_number = 0; station_number < station_vector_size; station_number++)
            {
                Station& station =  *(station_vector[station_number]);
                std::vector<Year>& year_vector = station.getYearVector();

                if ( !stationActiveForTheEntireDateRange(start_date_for_records, end_date_for_records, station) )
                {
                    std::cerr << "Rejecting station " << station.getStationName() << " : " << station.getStationNumber()
                              << " because no records from " << start_date_for_records
                              << " or no records from " << end_date_for_records
                              << std::endl;
                    continue;
                }

                for (size_t i = 0; i < year_vector.size(); i++)
                {
                    int year = year_vector.at(i).getYear();

                    if ( station.getYearContainsTemperatureDataMap()[year] )
                    {
                        total_latitude_per_year_map[year] += fabs(atof( station.getLatitude().c_str() ));
                        total_longitude_per_year_map[year] += fabs(atof( station.getLongitude().c_str() ));
                        number_of_active_stations_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfDaysAboveMaxThreshold() > 0 )
                    {
                        number_of_stations_above_the_maximum_threshold_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfDaysBelowMaxThreshold() > 0 )
                    {
                        number_of_stations_below_the_maximum_threshold_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfDaysAboveMinThreshold() > 0 )
                    {
                        number_of_stations_above_the_minimum_threshold_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfDaysBelowMinThreshold() > 0 )
                    {
                        number_of_stations_below_the_minimum_threshold_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfDaysWithSnow() > 0 )
                    {
                        number_of_stations_with_snow_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfDaysWithoutSnow() > 0 )
                    {
                        number_of_stations_without_snow_per_year_map[year]++;
                    }

                    if ( year_vector.at(i).getNumberOfConsecutiveDaysAboveMaxThreshold() >
                         maximum_number_of_consecutive_days_above_maximum_threshold_per_year_map[year] )
                    {
                        maximum_number_of_consecutive_days_above_maximum_threshold_per_year_map[year] =
                                (unsigned int)year_vector.at(i).getNumberOfConsecutiveDaysAboveMaxThreshold();

                        maximum_number_of_consecutive_days_above_maximum_threshold_station_per_year_map[year] =
                                &station;
                    }

                    if ( year_vector.at(i).getNumberOfConsecutiveDaysBelowMaxThreshold() >
                         maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[year] )
                    {
                        //                		std::cerr << "Setting consecutive max days below to " << year_vector.at(i).getNumberOfConsecutiveDaysBelowMaxThreshold();
                        //                		std::cerr << " for station " << station.getStationName() << " year "<< year << std::endl;
                        maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[year] =
                                (unsigned int)year_vector.at(i).getNumberOfConsecutiveDaysBelowMaxThreshold();

                        maximum_number_of_consecutive_days_below_maximum_threshold_station_per_year_map[year] =
                                &station;
                    }

                    if ( year_vector.at(i).getNumberOfConsecutiveDaysBelowMinThreshold() >
                         maximum_number_of_consecutive_days_below_minimum_threshold_per_year_map[year] )
                    {
                        //                		std::cerr << "Setting consecutive min days below to " << year_vector.at(i).getNumberOfConsecutiveDaysBelowMinThreshold();
                        //                		std::cerr << " for station " << station.getStationName() << " year "<< year << std::endl;
                        maximum_number_of_consecutive_days_below_minimum_threshold_per_year_map[year] =
                                (unsigned int)year_vector.at(i).getNumberOfConsecutiveDaysBelowMinThreshold();

                        maximum_number_of_consecutive_days_below_minimum_threshold_station_per_year_map[year] =
                                &station;
                    }

                    if ( year_vector.at(i).getNumberOfConsecutiveDaysAboveMinThreshold() >
                         maximum_number_of_consecutive_days_above_minimum_threshold_per_year_map[year] )
                    {
                        //                		std::cerr << "Setting consecutive min days below to " << year_vector.at(i).getNumberOfConsecutiveDaysBelowMinThreshold();
                        //                		std::cerr << " for station " << station.getStationName() << " year "<< year << std::endl;
                        maximum_number_of_consecutive_days_above_minimum_threshold_per_year_map[year] =
                                (unsigned int)year_vector.at(i).getNumberOfConsecutiveDaysAboveMinThreshold();

                        maximum_number_of_consecutive_days_above_minimum_threshold_station_per_year_map[year] =
                                &station;
                    }
                }

                number_of_all_time_record_maximums_per_year_map[ station.getRecordMaxYear() ]++;
                number_of_all_time_record_minimums_per_year_map[ station.getRecordMinYear() ]++;
                number_of_all_time_record_precipitations_per_year_map[ station.getRecordPrecipitationYear() ]++;
                number_of_all_time_record_snowfalls_per_year_map[ station.getRecordSnowfallYear() ]++;

                float record_date = (float)station.getRecordMaxYear() + ( ( (float)station.getRecordMaxDayOfYear() - 1 ) / NUMBER_OF_DAYS_PER_YEAR );
                std::cout << station.getStationName().substr(0, 20);
                station.setStateName(state_name_map[station.getStationNumber()]);
                station.setStateNumber(state_number_map[station.getStateName()]);
                std::cout << "," << station.getStateName();
                std::cout << "," << station.getStationNumber();

                if ( station.getYearContainsTemperatureDataMap().size() > 0)
                {
                    std::cout << "," << station.getYearContainsTemperatureDataMap().begin()->first;
                    std::cout << "," << station.getYearContainsTemperatureDataMap().rbegin()->first;
                }
                else
                {
                    std::cout << ",,";
                }

                std::cout << "," << std::setprecision(2) << std::fixed << station.getRecordMaxYear() << "," << station.getRecordMaxMonth() << "/" << station.getRecordMaxDayOfMonth() + 1 << "/" << station.getRecordMaxYear() << "," << cToF( station.getRecordMaxTemperature() );
                std::cerr << station.getStationName().substr(0, 20) << "," << station.getStateName() << "," << station.getStationNumber();
                std::cerr << "," << std::setprecision(2) << std::fixed << station.getRecordMaxYear() << "," << station.getRecordMaxDayOfYear() + 1 << "," << station.getRecordMaxTemperature();
                record_date = (float)station.getRecordMinYear() + ( ( (float)station.getRecordMinDayOfYear() - 1 ) / NUMBER_OF_DAYS_PER_YEAR );
                std::cout << "," << std::setprecision(2) << std::fixed << station.getRecordMinYear() << "," << station.getRecordMinMonth() << "/" << station.getRecordMinDayOfMonth() + 1 << "/" << station.getRecordMinYear() << "," << cToF( station.getRecordMinTemperature() );
                std::cerr << "," << std::setprecision(2) << std::fixed << station.getRecordMinYear() << "," << station.getRecordMinDayOfYear() + 1 << "," << station.getRecordMinTemperature() << std::endl;
                std::cout << "," << std::setprecision(2) << std::fixed << station.getRecordPrecipitationYear() << "," << station.getRecordPrecipitationMonth() << "/" << station.getRecordPrecipitationDayOfMonth() + 1 << "/" << station.getRecordPrecipitationYear() << "," << station.getRecordPrecipitation();
                std::cout << "," << std::setprecision(2) << std::fixed << station.getRecordSnowfallYear() << "," << station.getRecordSnowfallMonth() << "/" << station.getRecordSnowfallDayOfMonth() + 1 << "/" << station.getRecordSnowfallYear() << "," << station.getRecordSnowfall();

                if (g_month_to_dump)
                {
                    std::cout << "," << g_month_to_dump << "/" << g_day_to_dump;

                    if (year_to_dump)
                    {
                        std::cout << "/" << year_to_dump;
                    }
                }
                else
                {
                    std::cout << std::endl;
                }

                used_station_count++;

                size_t year_vector_size = year_vector.size();

                float record_max_temperatures[NUMBER_OF_MONTHS_PER_YEAR][MAX_DAYS_IN_MONTH];
                float record_low_max_temperatures[NUMBER_OF_MONTHS_PER_YEAR][MAX_DAYS_IN_MONTH];
                float record_max_monthly_temperatures[NUMBER_OF_MONTHS_PER_YEAR];
                float record_min_temperatures[NUMBER_OF_MONTHS_PER_YEAR][MAX_DAYS_IN_MONTH];
                float record_min_monthly_temperatures[NUMBER_OF_MONTHS_PER_YEAR];
                std::vector<unsigned int> record_max_temperature_year_vector[NUMBER_OF_MONTHS_PER_YEAR][MAX_DAYS_IN_MONTH];
                std::vector<unsigned int> record_low_max_temperature_year_vector[NUMBER_OF_MONTHS_PER_YEAR][MAX_DAYS_IN_MONTH];
                std::vector<unsigned int> record_max_monthly_temperature_year_vector[NUMBER_OF_MONTHS_PER_YEAR];
                std::vector<unsigned int> record_min_temperature_year_vector[NUMBER_OF_MONTHS_PER_YEAR][MAX_DAYS_IN_MONTH];
                std::vector<unsigned int> record_min_monthly_temperature_year_vector[NUMBER_OF_MONTHS_PER_YEAR];

                //std::cerr << station.getStationName();

                for (size_t i = 0; i < NUMBER_OF_MONTHS_PER_YEAR; i++)
                {
                    record_max_monthly_temperatures[i] = float(INT_MIN);
                    record_min_monthly_temperatures[i] = float(INT_MAX);

                    float average_monthly_min_temperature =
                            station.getMinBaselineTotalPerMonthVector()[i] /
                            (float)station.getMinBaselineCountPerMonthVector()[i];

                    if ( station.getMinBaselineCountPerMonthVector()[i] > 0)
                    {
                        station.getMinBaselineAveragePerMonthVector()[i] = average_monthly_min_temperature / 10.0f;
                    }
                    else
                    {
                        station.getMinBaselineAveragePerMonthVector()[i] = UNKNOWN_TEMPERATURE;
                    }


                    float average_monthly_max_temperature =
                            station.getMaxBaselineTotalPerMonthVector()[i] /
                            (float)station.getMaxBaselineCountPerMonthVector()[i];

                    if ( station.getMaxBaselineCountPerMonthVector()[i] > 0)
                    {
                        station.getMaxBaselineAveragePerMonthVector()[i] = average_monthly_max_temperature / 10.0f;
                    }
                    else
                    {
                        station.getMaxBaselineAveragePerMonthVector()[i] = UNKNOWN_TEMPERATURE;
                    }

                    //std::cerr << " " << average_monthly_max_temperature;

                    for (size_t j = 0; j < MAX_DAYS_IN_MONTH; j++)
                    {
                        record_max_temperatures[i][j] = float(INT_MIN);
                        record_low_max_temperatures[i][j] = float(INT_MAX);
                        record_min_temperatures[i][j] = float(INT_MAX);
                    }
                }

                //std::cerr << std::endl;

                bool found_year_to_dump = false;
                int graphing_index = 0;

                for (size_t year_number = 0; year_number < year_vector_size; year_number++)
                {
                    float hottest_temperature = float(INT_MIN);
                    float coldest_temperature = float(INT_MAX);
                    float hottest_minimum_temperature = float(INT_MIN);
                    float coldest_maximum_temperature = float(INT_MAX);

                    std::vector<Month>& month_vector = year_vector.at(year_number).getMonthVector();
                    size_t month_vector_size = month_vector.size();
                    unsigned int year = year_vector.at(year_number).getYear();
                    unsigned int day_of_year_of_last_snowfall = 0;

                    if ( year_under_test && (year_under_test != year) )
                    {
                        continue;
                    }

                    for (size_t month_number = 0; month_number < month_vector_size; month_number++)
                    {
                        std::vector<Day>& day_vector = month_vector.at(month_number).getDayVector();
                        size_t day_vector_size = day_vector.size();

                        size_t absolute_month_number = (year * NUMBER_OF_MONTHS_PER_YEAR) + month_number;

                        if ( month_under_test && !months_under_test_map[month_number + 1] )
                        {
                            continue;
                        }

                        float previous_max_temperature = (float)INT_MAX;
                        size_t previous_max_day_number = 0;

                        for (size_t day_number = 0; day_number < day_vector_size; day_number++)
                        {
                            Day& day = day_vector.at(day_number);
                            float max_temperature = day.getMaxTemperature();
                            float snow = day.getSnowfall();
                            float precipitation = day.getPrecipitation();
                            float max_deviation;

                            if (station.getMaxBaselineAveragePerMonthVector()[month_number] == UNKNOWN_TEMPERATURE)
                            {
                                max_deviation = 0;
                            }
                            else
                            {
                                max_deviation = max_temperature - station.getMaxBaselineAveragePerMonthVector()[month_number];
                            }

                            //std::cerr << "max deviation = " << max_deviation << std::endl;
                            float min_temperature = day.getMinTemperature();
                            float min_deviation;

                            if (max_temperature > hottest_temperature
                                && max_temperature != UNREASONABLE_LOW_TEMPERATURE
                                && max_temperature != UNKNOWN_TEMPERATURE
                               )
                            {
                                hottest_temperature = max_temperature;
                            }

                            if (min_temperature < coldest_temperature
                                    && min_temperature != UNREASONABLE_LOW_TEMPERATURE
                                    && min_temperature != UNKNOWN_TEMPERATURE
                               )
                            {
                                coldest_temperature = min_temperature;
                            }

                            if (min_temperature > hottest_minimum_temperature
                                && min_temperature != UNREASONABLE_LOW_TEMPERATURE
                                && min_temperature != UNKNOWN_TEMPERATURE
                               )
                            {
                                hottest_minimum_temperature = min_temperature;
                            }

                            if (max_temperature < coldest_maximum_temperature
                                    && max_temperature != UNREASONABLE_LOW_TEMPERATURE
                                    && max_temperature != UNKNOWN_TEMPERATURE
                               )
                            {
                                coldest_maximum_temperature = max_temperature;
                            }

                            if (station.getMinBaselineAveragePerMonthVector()[month_number] == UNKNOWN_TEMPERATURE)
                            {
                                min_deviation = 0;
                            }
                            else
                            {
                                min_deviation = min_temperature - station.getMinBaselineAveragePerMonthVector()[month_number];
                            }

                            if ( g_month_to_dump == (month_number + 1) && g_day_to_dump == (day_number + 1) )
                            {
                                if (max_temperature != UNKNOWN_TEMPERATURE)
                                {
                                    if (max_temperature >= g_temperature_threshold_high)
                                    {
                                        number_of_stations_above_the_maximum_threshold_on_the_day_under_test_per_year_map[year]++;
                                    }
                                    else
                                    {
                                        number_of_stations_below_the_maximum_threshold_on_the_day_under_test_per_year_map[year]++;
                                    }
                                }

                                //if (year_to_dump == 0 )
                                {
                                    g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE_ON_A_DAY_OF_THE_YEAR][graphing_index].first = year;
                                    g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ON_A_DAY_OF_THE_YEAR][graphing_index].first = year;
                                    g_result_pair[GRAPHING_OPTION_PRECIPITATION_ON_A_DAY_OF_THE_YEAR][graphing_index].first = year;
                                    g_result_pair[GRAPHING_OPTION_SNOWFALL_ON_A_DAY_OF_THE_YEAR][graphing_index].first = year;

                                    if (min_temperature != UNKNOWN_TEMPERATURE)
                                    {
                                        g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE_ON_A_DAY_OF_THE_YEAR][graphing_index].second = min_temperature;
                                        day_to_dump_min_map[year] = min_temperature < day_to_dump_min_map[year] ? min_temperature : day_to_dump_min_map[year];
                                    }

                                    if (max_temperature != UNKNOWN_TEMPERATURE)
                                    {
                                        g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ON_A_DAY_OF_THE_YEAR][graphing_index].second = max_temperature;
                                        day_to_dump_max_map[year] = max_temperature > day_to_dump_max_map[year] ? max_temperature : day_to_dump_max_map[year];
                                    }

                                    if (snow != UNKNOWN_TEMPERATURE)
                                    {
                                        g_result_pair[GRAPHING_OPTION_SNOWFALL_ON_A_DAY_OF_THE_YEAR][graphing_index].second = snow;
                                        day_to_dump_snow_map[year] = snow > day_to_dump_snow_map[year] ? snow : day_to_dump_snow_map[year];
                                    }

                                    if (precipitation != UNKNOWN_TEMPERATURE)
                                    {
                                        g_result_pair[GRAPHING_OPTION_PRECIPITATION_ON_A_DAY_OF_THE_YEAR][graphing_index].second = precipitation;
                                        day_to_dump_precip_map[year] = precipitation > day_to_dump_precip_map[year] ? precipitation : day_to_dump_precip_map[year];
                                    }
                                }
                            }

                            size_t day_of_year = getDayOfYear(day_number + 1, month_number + 1, year);

                            if ( ( year_to_dump == year ) &&
                                 ( g_month_to_dump == (month_number + 1) ) &&
                                 ( g_day_to_dump == (day_number + 1) || (g_day_to_dump == 0) )
                                 )
                            {
                                if ( (max_temperature != UNKNOWN_TEMPERATURE) && ( min_temperature != UNKNOWN_TEMPERATURE) )
                                {
                                    std::cout << "," <<  (max_temperature) << "," << cToF(max_temperature) << "," << (min_temperature) << "," << cToF(min_temperature);
                                    std::cout << "," <<  precipitation << "," <<  snow;
                                }

                                found_year_to_dump = true;
                                std::cout << std::endl;
                            }

                            // Don't use broken readings
                            if (max_temperature != UNKNOWN_TEMPERATURE  && max_temperature < UNREASONABLE_HIGH_TEMPERATURE)
                            {
                                total_temperature_per_year_map[year] = total_temperature_per_year_map[year] + max_temperature;
                                number_of_readings_per_year_map[year] = number_of_readings_per_year_map[year] + 1;
                                total_temperature_per_month[year - FIRST_YEAR][month_number] += max_temperature;
                                number_of_readings_per_month[year - FIRST_YEAR][month_number]++;
                                total_max_temperature_per_month[year - FIRST_YEAR][month_number] += max_temperature;
                                number_of_max_readings_per_month[year - FIRST_YEAR][month_number]++;
                                total_max_temperature_per_year_map[year] = total_max_temperature_per_year_map[year] + max_temperature;

                                total_max_deviation_per_year_map[year] = total_max_deviation_per_year_map[year] + max_deviation;

                                if (max_temperature > hottest_temperature_per_year_map_per_state_vector[station.getStateNumber()][year])
                                {
                                    hottest_temperature_per_year_map_per_state_vector[station.getStateNumber()][year] = max_temperature;
                                }

                                if ( !isnan(max_deviation) )
                                {
                                    //std::cerr << "Max temperature = " << max_temperature << std::endl;
                                    //std::cerr << "Max deviation = " << max_deviation << std::endl;
                                    //std::cerr << "Average = " << station.getMaxBaselineAveragePerMonthVector()[month_number] << std::endl;
                                    number_of_max_readings_per_year_map[year] = number_of_max_readings_per_year_map[year] + 1;
                                    total_max_deviation_per_year_map_per_state_vector[station.getStateNumber()][year] =
                                            total_max_deviation_per_year_map_per_state_vector[station.getStateNumber()][year] + max_deviation;
                                    number_of_max_readings_per_year_map_per_state_vector[station.getStateNumber()][year] =
                                            number_of_max_readings_per_year_map_per_state_vector[station.getStateNumber()][year] + 1;
                                }

                                if (round(max_temperature) >= g_temperature_threshold_high)
                                {
                                    number_of_maximum_temperatures_above_high_threshold[year - FIRST_YEAR]++;

                                    if (first_day_above_high_threshold_per_year_map[year] == NO_VALUE)
                                    {
                                        first_day_above_high_threshold_per_year_map[year] = (int)day_of_year;
                                        last_day_above_high_threshold_per_year_map[year] = (int)day_of_year;
                                    }

                                    if ( day_of_year > (size_t)last_day_above_high_threshold_per_year_map[year] )
                                    {
                                        last_day_above_high_threshold_per_year_map[year] = (int)day_of_year;
                                    }

                                    if ( day_of_year < (size_t)first_day_above_high_threshold_per_year_map[year] )
                                    {
                                        first_day_above_high_threshold_per_year_map[year] = (int)day_of_year;
                                    }

                                    // Per state
                                    if (first_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] == NO_VALUE)
                                    {
                                        first_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                        last_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                    }

                                    if ( day_of_year > (size_t)last_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] )
                                    {
                                        last_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                    }

                                    if ( day_of_year < (size_t)first_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] )
                                    {
                                        first_day_above_high_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                    }


                                    if (max_temperature > previous_max_temperature && (day_number - previous_max_day_number) == 1)
                                    {
                                        number_of_maximum_temperatures_above_high_threshold_and_higher_than_previous_day[year - FIRST_YEAR]++;
                                    }
                                }
                                else
                                {
                                    number_of_maximum_temperatures_below_high_threshold[year - FIRST_YEAR]++;
                                }


                                if (max_temperature <= g_temperature_threshold_low)
                                {
                                    number_of_max_temperatures_below_low_threshold_map[year]++;
                                }

                                if ( max_temperature > all_time_record_maximum_per_month_map[(unsigned int)absolute_month_number] )
                                {
                                    all_time_record_maximum_per_month_map[(unsigned int)absolute_month_number] = max_temperature;
                                    number_of_all_time_record_maximums_per_month_map[(unsigned int)absolute_month_number]++;
                                }

                                if ( max_temperature == record_max_monthly_temperatures[month_number] )
                                {
                                    record_max_monthly_temperature_year_vector[month_number].push_back(year);
                                }
                                else if (max_temperature > record_max_monthly_temperatures[month_number] )
                                {
                                    std::vector<unsigned int>& record_max_vector = record_max_monthly_temperature_year_vector[month_number];

                                    record_max_monthly_temperatures[month_number] = max_temperature;
                                    record_max_vector.erase( record_max_vector.begin(),record_max_vector.end() );
                                    record_max_vector.push_back(year);
                                }

                                if ( max_temperature == record_max_temperatures[month_number][day_number] )
                                {
                                    //std::cout << "station_number = " << station_number;
                                    //std::cout << " station name = " << station.getStationName();
                                    //std::cout << " year = " << year;
                                    //std::cout << " month = " << month_number;
                                    //std::cout << " day = " << day_number;
                                    //std::cout << " old record = " << record_max_temperatures[month_number][day_number];
                                    //std::cout << " new record = " << max_temperature;
                                    //std::cout << std::endl;
                                    record_max_temperature_year_vector[month_number][day_number].push_back(year);
                                }

                                if (max_temperature == record_low_max_temperatures[month_number][day_number] )
                                {
                                    record_low_max_temperature_year_vector[month_number][day_number].push_back(year);
                                }

                                if ( max_temperature > record_max_temperatures[month_number][day_number] )
                                {
                                    //std::cout << "station_number = " << station_number;
                                    //std::cout << " station name = " << station.getStationName();
                                    //std::cout << " year = " << year;
                                    //std::cout << " month = " << month_number;
                                    //std::cout << " day = " << day_number;
                                    //std::cout << " old record = " << record_max_temperatures[month_number][day_number];
                                    //std::cout << " new record = " << max_temperature;
                                    //std::cout << std::endl;

                                    record_max_temperatures[month_number][day_number] = max_temperature;
                                    record_incremental_max_per_year_map[year] = record_incremental_max_per_year_map[year] + 1;

                                    std::vector<unsigned int>& record_max_vector = record_max_temperature_year_vector[month_number][day_number];
                                    size_t size = record_max_vector.size();

                                    for (int i = (int)size - 1; i >= 0; i--)
                                    {
                                        record_max_vector.erase( record_max_vector.begin() + i );
                                    }

                                    record_max_vector.push_back(year);
                                }

                                if ( max_temperature < record_low_max_temperatures[month_number][day_number] )
                                {
                                    record_low_max_temperatures[month_number][day_number] = max_temperature;
                                    std::vector<unsigned int>& record_low_max_vector = record_low_max_temperature_year_vector[month_number][day_number];
                                    record_low_max_vector.erase( record_low_max_vector.begin(), record_low_max_vector.end() );
                                    record_low_max_vector.push_back(year);
                                }

                                previous_max_temperature = max_temperature;
                                previous_max_day_number = day_number;
                            }

                            if (min_temperature != UNKNOWN_TEMPERATURE  && min_temperature > UNREASONABLE_LOW_TEMPERATURE)
                            {
                                total_temperature_per_year_map[year] = total_temperature_per_year_map[year] + min_temperature;
                                number_of_readings_per_year_map[year] = number_of_readings_per_year_map[year] + 1;
                                total_temperature_per_month[year - FIRST_YEAR][month_number] += min_temperature;
                                number_of_readings_per_month[year - FIRST_YEAR][month_number]++;
                                total_min_temperature_per_month[year - FIRST_YEAR][month_number] += min_temperature;
                                number_of_min_readings_per_month[year - FIRST_YEAR][month_number]++;
                                total_min_temperature_per_year_map[year] = total_min_temperature_per_year_map[year] + min_temperature;
                                total_min_deviation_per_year_map[year] = total_min_deviation_per_year_map[year] + min_deviation;
                                number_of_min_readings_per_year_map[year] = number_of_min_readings_per_year_map[year] + 1;

                                if (min_temperature < coldest_temperature_per_year_map_per_state_vector[station.getStateNumber()][year])
                                {
                                    coldest_temperature_per_year_map_per_state_vector[station.getStateNumber()][year] = min_temperature;
                                }

                                if ( !isnan(min_deviation) )
                                {
                                    int state_number = station.getStateNumber();
                                    //std::cout << state_number << std::endl;

                                    total_min_deviation_per_year_map_per_state_vector[state_number][year] =
                                            total_min_deviation_per_year_map_per_state_vector[state_number][year] + min_deviation;
                                    number_of_min_readings_per_year_map_per_state_vector[state_number][year] =
                                            number_of_min_readings_per_year_map_per_state_vector[state_number][year] + 1;
                                }

                                if (round(min_temperature) <= g_temperature_threshold_low)
                                {
                                    number_of_minimum_temperatures_below_low_threshold[year - FIRST_YEAR]++;

                                    if (first_day_below_low_threshold_per_year_map[year] == NO_VALUE)
                                    {
                                        first_day_below_low_threshold_per_year_map[year] = (int)day_of_year;
                                        last_day_below_low_threshold_per_year_map[year] = (int)day_of_year;
                                    }

                                    if (day_of_year > (size_t)last_day_below_low_threshold_per_year_map[year])
                                    {
                                        last_day_below_low_threshold_per_year_map[year] = (int)day_of_year;
                                    }

                                    if (day_of_year < (size_t)first_day_below_low_threshold_per_year_map[year])
                                    {
                                        first_day_below_low_threshold_per_year_map[year] = (int)day_of_year;
                                    }

                                    // Per state
                                    if (first_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] == NO_VALUE)
                                    {
                                        first_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                        last_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                    }

                                    if (day_of_year > (size_t)last_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year])
                                    {
                                        last_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                    }

                                    if (day_of_year < (size_t)first_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year])
                                    {
                                        first_day_below_low_threshold_per_year_map_per_state_vector[station.getStateNumber()][year] = (int)day_of_year;
                                    }
                                }
                                else
                                {
                                    number_of_minimum_temperatures_above_low_threshold[year - FIRST_YEAR]++;
                                }

                                if (min_temperature >= g_temperature_threshold_high)
                                {
                                    number_of_min_temperatures_above_high_threshold_map[year]++;
                                }

                                if ( min_temperature == record_min_monthly_temperatures[month_number] )
                                {
                                    record_min_monthly_temperature_year_vector[month_number].push_back(year);
                                }
                                else if (min_temperature < record_min_monthly_temperatures[month_number] )
                                {
                                    std::vector<unsigned int>& record_min_vector = record_min_monthly_temperature_year_vector[month_number];

                                    record_min_monthly_temperatures[month_number] = min_temperature;
                                    record_min_vector.erase( record_min_vector.begin(),record_min_vector.end() );
                                    record_min_vector.push_back(year);
                                }

                                if ( min_temperature == record_min_temperatures[month_number][day_number] )
                                {
                                    record_min_temperature_year_vector[month_number][day_number].push_back(year);
                                }

                                if ( min_temperature < record_min_temperatures[month_number][day_number] )
                                {
                                    record_min_temperatures[month_number][day_number] = min_temperature;
                                    record_incremental_min_per_year_map[year] = record_incremental_min_per_year_map[year] + 1;

                                    std::vector<unsigned int>& record_min_vector = record_min_temperature_year_vector[month_number][day_number];
                                    size_t size = record_min_vector.size();

                                    for (int i = (int)size - 1; i >= 0; i--)
                                    {
                                        record_min_vector.erase( record_min_vector.begin() + i );
                                    }

                                    record_min_vector.push_back(year);
                                }
                            }

                            if (snow != UNKNOWN_TEMPERATURE)
                            {
                                total_snowfall_per_year_map[year] = total_snowfall_per_year_map[year] + snow;
                                
                                if ( snow > 0.0f || lastDayOfYear(day_of_year, year) )
                                {
                                    number_of_snowfalls_per_year_map[year] = number_of_snowfalls_per_year_map[year] + 1;
                                    size_t number_of_days_since_last_snowfall = day_of_year - day_of_year_of_last_snowfall;

                                    if (
                                            number_of_days_since_last_snowfall > maximum_number_of_days_without_snow_per_year_map[year]
                                         || ( day_of_year < (NUMBER_OF_DAYS_PER_YEAR / 2) )
                                       )
                                    {
                                        last_spring_day_of_snow_per_year_map[year] = day_of_year_of_last_snowfall;
                                        //std::cout << "SNOW " <<  year << "," << day_of_year << "," << number_of_days_since_last_snowfall << std::endl;

                                        if ( day_of_year > (NUMBER_OF_DAYS_PER_YEAR / 2) )
                                        {
                                            maximum_number_of_days_without_snow_per_year_map[year] = (unsigned int)number_of_days_since_last_snowfall;
                                            first_autumn_day_of_snow_per_year_map[year] = (unsigned int)day_of_year;
                                        }
                                    }

                                    if ( snow > maximum_snowfall_per_year_day_map[year].getSnowfall() )
                                    {
                                        maximum_snowfall_per_year_day_map[year] = day;
                                    }

                                    day_of_year_of_last_snowfall = (unsigned int)day_of_year;
                                }


                                if (snow >= g_snow_target)
                                {
                                    number_of_days_above_snow_target_map[year] = number_of_days_above_snow_target_map[year] + 1;
                                }
                            }

                            if (precipitation != UNKNOWN_TEMPERATURE)
                            {
                                total_precipitation_per_year_map[year] = total_precipitation_per_year_map[year] + precipitation;
                                number_of_precip_readings_per_year_map[year] = number_of_precip_readings_per_year_map[year] + 1;

                                if (precipitation > 0.0f)
                                {
                                    number_of_precipitation_days_per_year_map[year] = number_of_precipitation_days_per_year_map[year] + 1;
                                }

                                number_of_precipitation_records_per_year_map[year] = number_of_precipitation_records_per_year_map[year] + 1;

                                if ( precipitation > maximum_precipitation_per_year_day_map[year].getPrecipitation() )
                                {
                                    maximum_precipitation_per_year_day_map[year] = day;
                                }

                                if (precipitation >= g_precipitation_target)
                                {
                                    number_of_days_above_precipitation_target_map[year] = number_of_days_above_precipitation_target_map[year] + 1;
                                }
                            }
                        }
                    }

                    g_result_pair[GRAPHING_OPTION_HOTTEST_TEMPERATURE][graphing_index].first = year;
                    g_result_pair[GRAPHING_OPTION_COLDEST_TEMPERATURE][graphing_index].first = year;
                    g_result_pair[GRAPHING_OPTION_HOTTEST_MAXIMUM_MINUS_COLDEST_MINIMUM][graphing_index].first = year;
                    g_result_pair[GRAPHING_OPTION_HOTTEST_MINIMUM_TEMPERATURE][graphing_index].first = year;
                    g_result_pair[GRAPHING_OPTION_COLDEST_MAXIMUM_TEMPERATURE][graphing_index].first = year;

                    if ( hottest_temperature == float(INT_MIN) )
                    {
                        g_result_pair[GRAPHING_OPTION_HOTTEST_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    }
                    else
                    {
                        number_of_hottest_maximum_temperatures_per_year_map[year] =
                                number_of_hottest_maximum_temperatures_per_year_map[year] + 1;
                        total_hottest_maximum_temperature_per_year_map[year] =
                                total_hottest_maximum_temperature_per_year_map[year] + hottest_temperature;
                        g_result_pair[GRAPHING_OPTION_HOTTEST_TEMPERATURE][graphing_index].second = hottest_temperature;
                    }

                    if ( coldest_temperature == float(INT_MAX) )
                    {
                        g_result_pair[GRAPHING_OPTION_COLDEST_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    }
                    else
                    {
                        number_of_coldest_minimum_temperatures_per_year_map[year] =
                                number_of_coldest_minimum_temperatures_per_year_map[year] + 1;
                        total_coldest_minimum_temperature_per_year_map[year] =
                                total_coldest_minimum_temperature_per_year_map[year] + coldest_temperature;
                        g_result_pair[GRAPHING_OPTION_COLDEST_TEMPERATURE][graphing_index].second = coldest_temperature;
                    }

                    if ( hottest_temperature == float(INT_MIN) || coldest_temperature == float(INT_MAX) )
                    {
                        g_result_pair[GRAPHING_OPTION_HOTTEST_MAXIMUM_MINUS_COLDEST_MINIMUM][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    }
                    else
                    {
                        g_result_pair[GRAPHING_OPTION_HOTTEST_MAXIMUM_MINUS_COLDEST_MINIMUM][graphing_index].second =
                                g_result_pair[GRAPHING_OPTION_HOTTEST_TEMPERATURE][graphing_index].second -
                                g_result_pair[GRAPHING_OPTION_COLDEST_TEMPERATURE][graphing_index].second;
                    }

                    if ( hottest_minimum_temperature == float(INT_MIN) )
                    {
                        g_result_pair[GRAPHING_OPTION_HOTTEST_MINIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    }
                    else
                    {
                        g_result_pair[GRAPHING_OPTION_HOTTEST_MINIMUM_TEMPERATURE][graphing_index].second = hottest_minimum_temperature;
                    }

                    if ( coldest_maximum_temperature == float(INT_MAX) )
                    {
                        g_result_pair[GRAPHING_OPTION_COLDEST_MAXIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    }
                    else
                    {
                        g_result_pair[GRAPHING_OPTION_COLDEST_MAXIMUM_TEMPERATURE][graphing_index].second = coldest_maximum_temperature;
                    }

                    graphing_index++;
                }

                if (year_to_dump && !found_year_to_dump)
                {
                    std::cout << std::endl;
                }

                if ( stationActiveForTheEntireDateRange(start_date_for_records, end_date_for_records, station) )
                {
                    for (size_t i = 0; i < NUMBER_OF_MONTHS_PER_YEAR; i++)
                    {
                        size_t size = record_max_monthly_temperature_year_vector[i].size();

                        for (size_t k = 0; k < size; k++)
                        {
                            unsigned int record_max_year = record_max_monthly_temperature_year_vector[i].at(k);
                            record_monthly_max_per_year_map[record_max_year]++;
                        }

                        size = record_min_monthly_temperature_year_vector[i].size();

                        for (size_t k = 0; k < size; k++)
                        {
                            unsigned int record_min_year = record_min_monthly_temperature_year_vector[i].at(k);
                            record_monthly_min_per_year_map[record_min_year]++;
                        }

                        for (size_t j = 0; j < MAX_DAYS_IN_MONTH; j++)
                        {
                            if ( ( g_month_to_dump == (i + 1) ) &&
                                 ( g_day_to_dump == (j + 1) ) &&
                                 ( year_to_dump == 0 )
                                 )
                            {
                                //std::cout << std::setw(15) << station_vector.at(station_number).getStateName() << ", ";
                                //std::cout << station_vector.at(station_number).getStationName() << "," << month_to_dump << "/" << g_day_to_dump;
                                std::cout << "," << cToF(record_max_temperatures[i][j]);

                                size_t size = record_max_temperature_year_vector[i][j].size();
                                size_t k = 0;

                                for ( ; k < size && k < MAX_DATE_DUMP_YEARS; k++)
                                {
                                    std::cout << "," << record_max_temperature_year_vector[i][j].at(k);
                                }

                                for ( ; k < MAX_DATE_DUMP_YEARS; k++)
                                {
                                    std::cout << ",";
                                }

                                std::cout << "," << cToF(record_low_max_temperatures[i][j]);

                                k = 0;
                                size = record_low_max_temperature_year_vector[i][j].size();

                                for ( ; k < size && k < MAX_DATE_DUMP_YEARS; k++)
                                {
                                    std::cout << "," << record_low_max_temperature_year_vector[i][j].at(k);
                                }

                                for ( ; k < MAX_DATE_DUMP_YEARS; k++)
                                {
                                    std::cout << ",";
                                }



                                std::cout << "," << cToF(record_min_temperatures[i][j]);
                                size = record_min_temperature_year_vector[i][j].size();
                                k = 0;

                                for ( ; k < size && k < MAX_DATE_DUMP_YEARS; k++)
                                {
                                    std::cout << "," << record_min_temperature_year_vector[i][j].at(k);
                                }

#if 0
                                std::cerr << "    " << record_min_temperatures[i][j] << " : ";

                                size = record_min_temperature_year_vector[i][j].size();

                                for (size_t k = 0; k < size; k++)
                                {
                                    std::cerr << record_min_temperature_year_vector[i][j].at(k) << ",";
                                }

#endif
                                std::cout << std::endl;
                            }

                            size = record_max_temperature_year_vector[i][j].size();

                            for (size_t k = 0; k < size; k++)
                            {
                                unsigned int record_max_year = record_max_temperature_year_vector[i][j].at(k);
                                record_max_per_year_map[record_max_year] = record_max_per_year_map[record_max_year] + 1;
                            }

                            size = record_min_temperature_year_vector[i][j].size();

                            for (size_t k = 0; k < size; k++)
                            {
                                unsigned int record_min_year = record_min_temperature_year_vector[i][j].at(k);
                                record_min_per_year_map[record_min_year] = record_min_per_year_map[record_min_year] + 1;
                            }
                        }

                        if ( ( g_month_to_dump == (i + 1) ) &&
                             ( g_day_to_dump == 0 ) &&
                             ( year_to_dump == 0 )
                             )
                        {
                            std::cout << std::endl;
                        }
                    }
                }

            }
        }

        std::cout << "Number of stations used in this analysis = " << used_station_count << std::endl;

        if (   g_current_graphing_option == GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE
               || g_current_graphing_option == GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE
               || g_current_graphing_option == GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES
               || g_current_graphing_option == GRAPHING_OPTION_DAILY_PRECIPITATION
               || g_current_graphing_option == GRAPHING_OPTION_DAILY_SNOWFALL
               )
        {
            dump_year = g_first_year;
            dump_year_2 = g_last_year;
        }

        if (dump_year != 0)
        {
            for (size_t state_number = 0; state_number < state_vector_size; state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();
                size_t station_vector_size = station_vector.size();

                for (size_t station_number = 0; station_number < station_vector_size; station_number++)
                {
                    Station& station =  *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();

                    for (size_t i = 0; i < year_vector.size(); i++)
                    {
                        Year& year = year_vector.at(i);
                        int year_number = year.getYear();

                        if (year_number == (int)dump_year || year_number == dump_year_2)
                        {
                            std::cout << "Year,Month,Day,Day of year,Date,Max,Min,Snow,Precip" << std::endl;
                            std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName() << ",";
                            std::cout << station.getStationNumber() << std::endl;
                            int graphing_index = 0;

                            std::vector<Month>& month_vector = year.getMonthVector();

                            for (size_t j = 0; j < month_vector.size(); j++)
                            {
                                size_t month_number = j + 1;
                                Month& month = month_vector.at(j);

                                std::vector<Day>& day_vector = month.getDayVector();

                                for (size_t k = 0; k < day_vector.size(); k++)
                                {
                                    size_t day_number = k + 1;
                                    size_t day_of_year = getDayOfYear(day_number, month_number, year_number);
                                    Day& day = day_vector.at(k);
                                    std::cout << year_number << ",";
                                    std::cout << month_number << ",";
                                    std::cout << day_number << ",";
                                    std::cout << day_of_year << ",";
                                    std::cout << month_number << "/" << day_number << "/" << year_number << ",";

                                    if (year_number == (int)dump_year)
                                    {
                                        g_result_pair[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].first = (float)day_of_year;
                                        g_result_pair[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair[GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE][graphing_index].first = (float)day_of_year;
                                        g_result_pair[GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair[GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES][graphing_index].first = (float)day_of_year;
                                        g_result_pair[GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES][graphing_index].first = (float)day_of_year;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair[GRAPHING_OPTION_DAILY_PRECIPITATION][graphing_index].first = (float)day_of_year;
                                        g_result_pair[GRAPHING_OPTION_DAILY_PRECIPITATION][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair[GRAPHING_OPTION_DAILY_SNOWFALL][graphing_index].first = (float)day_of_year;
                                        g_result_pair[GRAPHING_OPTION_DAILY_SNOWFALL][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;

                                        if ( day.getMaxTemperature() == UNKNOWN_TEMPERATURE )
                                        {
                                            std::cout << ",";
                                        }
                                        else
                                        {
                                            std::cout << cToF( day.getMaxTemperature() ) << ",";
                                            g_result_pair[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].second = day.getMaxTemperature();
                                            g_result_pair[GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES][graphing_index].second = day.getMaxTemperature();
                                            //                                        std::cout << graphing_index << " " << g_result_pair[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].first;
                                            //                                        std::cout << " " << g_result_pair[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].second << std::endl;
                                        }

                                        if ( day.getMinTemperature() == UNKNOWN_TEMPERATURE )
                                        {
                                            std::cout << ",";
                                        }
                                        else
                                        {
                                            std::cout << cToF( day.getMinTemperature() ) << ",";
                                            g_result_pair[GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE][graphing_index].second = day.getMinTemperature();
                                            g_result_pair_2[GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES][graphing_index].second = day.getMinTemperature();
                                        }

                                        if ( day.getSnowfall() == UNKNOWN_TEMPERATURE )
                                        {
                                            std::cout << ",";
                                        }
                                        else
                                        {
                                            g_result_pair[GRAPHING_OPTION_DAILY_SNOWFALL][graphing_index].second = day.getSnowfall();
                                            std::cout << day.getSnowfall() << ",";
                                        }

                                        if ( day.getPrecipitation() == UNKNOWN_TEMPERATURE )
                                        {
                                            std::cout << ",";
                                        }
                                        else
                                        {
                                            g_result_pair[GRAPHING_OPTION_DAILY_PRECIPITATION][graphing_index].second = day.getPrecipitation();
                                            std::cout << day.getPrecipitation() << ",";
                                        }
                                        std::cout << std::endl;

                                    }
                                    else if (year_number == (int)dump_year_2)
                                    {
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].first = (float)day_of_year;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE][graphing_index].first = (float)day_of_year;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_PRECIPITATION][graphing_index].first = (float)day_of_year;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_PRECIPITATION][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_SNOWFALL][graphing_index].first = (float)day_of_year;
                                        g_result_pair_2[GRAPHING_OPTION_DAILY_SNOWFALL][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;

                                        if ( day.getMaxTemperature() == UNKNOWN_TEMPERATURE )
                                        {
                                        }
                                        else
                                        {
                                            g_result_pair_2[GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE][graphing_index].second = day.getMaxTemperature();
                                        }

                                        if ( day.getMinTemperature() == UNKNOWN_TEMPERATURE )
                                        {
                                        }
                                        else
                                        {
                                            g_result_pair_2[GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE][graphing_index].second = day.getMinTemperature();
                                        }

                                        if ( day.getSnowfall() == UNKNOWN_TEMPERATURE )
                                        {
                                        }
                                        else
                                        {
                                            g_result_pair_2[GRAPHING_OPTION_DAILY_SNOWFALL][graphing_index].second = day.getSnowfall();
                                        }

                                        if ( day.getPrecipitation() == UNKNOWN_TEMPERATURE )
                                        {

                                        }
                                        else
                                        {
                                            g_result_pair_2[GRAPHING_OPTION_DAILY_PRECIPITATION][graphing_index].second = day.getPrecipitation();
                                        }
                                    }

                                    graphing_index++;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Dump out the results
        if (g_month_to_dump != 0 && year_to_dump == 0)
        {
            std::cout << "Year,Minimum,Maximum,Precip,Snow" << std::endl;

            for (unsigned int year = FIRST_YEAR; year <= most_recent_year; year++)
            {
                std::cout << year << ",";

                if (day_to_dump_min_map[year] == (float)INT_MAX)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << cToF(day_to_dump_min_map[year]) << ",";
                }

                if (day_to_dump_max_map[year] == (float)INT_MIN)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << cToF(day_to_dump_max_map[year]) << ",";
                }

                if (day_to_dump_precip_map[year] == (float)INT_MIN)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << day_to_dump_precip_map[year] << ",";
                }

                if (day_to_dump_snow_map[year] == (float)INT_MIN)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << day_to_dump_snow_map[year] << ",";
                }

                std::cout << std::endl;
            }
        }


        if (year_for_stats != 0)
        {
            std::cout << "Stations in " << year_for_stats << " which didn't reach the maximum threshold of " << g_temperature_threshold_high << " C" << std::endl;

            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "below_maximum_threshold_" << g_temperature_threshold_high << "_" << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"greenIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();

                    for (size_t i = 0; i < year_vector.size(); i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats)
                        {
                            if ( year_vector.at(i).getRecordMaxTemperature() != float(INT_MIN) &&
                                 year_vector.at(i).getRecordMaxTemperature() < g_temperature_threshold_high )
                            {
                                std::cout << station.getStationName().substr(0, 20)
                                          << "," << station.getStateName()
                                          << "," << station.getStationNumber()
                                          << "," << year_vector.at(i).getRecordMaxTemperature()
                                          << "," << cToF( year_vector.at(i).getRecordMaxTemperature() )
                                          << std::endl;

                                kml_file                                                                                        << std::endl;
                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                kml_file << "        <name>"                                                                    << std::endl;
                                kml_file << "                " << station.getStationName().substr(0, 20)                        << std::endl;
                                kml_file << "        </name>"                                                                   << std::endl;
                                kml_file << "<styleUrl>#greenIcon</styleUrl>"                                                    << std::endl;
                                kml_file << "<Point>"                                                                           << std::endl;
                                kml_file << "        <coordinates>"                                                             << std::endl;
                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                kml_file << "        </coordinates>"                                                            << std::endl;
                                kml_file << "</Point>"                                                                          << std::endl;
                                kml_file << "</Placemark>"                                                                      << std::endl;
                                kml_file                                                                                        << std::endl;

                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (g_month_to_dump != 0 && g_day_to_dump != 0)
        {
            std::cout << "Stations on " << g_month_to_dump << "/" << g_day_to_dump << "/" << year_for_stats;
            std::cout << " which didn't reach the maximum threshold of " << g_temperature_threshold_high << " C" << std::endl;

            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "below_maximum_threshold_" << g_temperature_threshold_high << "_" << g_month_to_dump << g_day_to_dump << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"greenIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ff00</color>" << std::endl;
            kml_file << "   <scale>1.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();
                    bool already_counted = false;

                    for (size_t i = 0; i < year_vector.size() && !already_counted; i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats && !already_counted)
                        {
                            std::vector<Month>& month_vector = year_vector.at(i).getMonthVector();

                            for (size_t j = 0; j < month_vector.size(); j++)
                            {
                                Month& month = month_vector.at(j);
                                size_t month_number = j + 1;

                                if (month_number == g_month_to_dump && !already_counted)
                                {
                                    std::vector<Day>& day_vector = month.getDayVector();

                                    for (size_t k = 0; k < day_vector.size(); k++)
                                    {
                                        Day& day = day_vector.at(k);
                                        size_t day_number = k + 1;

                                        if (day_number == g_day_to_dump && !already_counted)
                                        {
                                            if (day.getMaxTemperature() != UNKNOWN_TEMPERATURE &&
                                                    day.getMaxTemperature() < g_temperature_threshold_high )
                                            {
                                                std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName()
                                                          << "," << station.getStationNumber()
                                                          << "," << day.getMaxTemperature()
                                                          << "," << cToF( day.getMaxTemperature() )
                                                          << std::endl;

                                                kml_file                                                                                        << std::endl;
                                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                                kml_file << "        <name>"                                                                    << std::endl;
                                                kml_file << "                " << station.getStationName().substr(0, 20)                        << std::endl;
                                                kml_file << "        </name>"                                                                   << std::endl;
                                                kml_file << "<styleUrl>#greenIcon</styleUrl>"                                                   << std::endl;
                                                kml_file << "<Point>"                                                                           << std::endl;
                                                kml_file << "        <coordinates>"                                                             << std::endl;
                                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                                kml_file << "        </coordinates>"                                                            << std::endl;
                                                kml_file << "</Point>"                                                                          << std::endl;
                                                kml_file << "</Placemark>"                                                                      << std::endl;
                                                kml_file                                                                                        << std::endl;

                                                already_counted = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (year_for_stats != 0)
        {
            std::cout << "Stations in " << year_for_stats << " which did reach the maximum threshold of " << g_temperature_threshold_high << " C" << std::endl;

            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "above_maximum_threshold_" << g_temperature_threshold_high << "_" << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;


            kml_file << "<Style id=\"blueIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffffaaaa</color>" << std::endl;
            kml_file << "   <scale>1.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"greenIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ff00</color>" << std::endl;
            kml_file << "   <scale>1.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"yellowIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffccffff</color>" << std::endl;
            kml_file << "   <scale>1.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"redIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/red-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffccccff</color>" << std::endl;
            kml_file << "   <scale>1.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();

                    for (size_t i = 0; i < year_vector.size(); i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats)
                        {
                            if ( year_vector.at(i).getRecordMaxTemperature() != float(INT_MIN) &&
                                 year_vector.at(i).getRecordMaxTemperature() >= g_temperature_threshold_high )
                            {
                                std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName() << ","
                                          << station.getStationNumber()
                                          << "," << year_vector.at(i).getRecordMaxTemperature()
                                          << "," << cToF( year_vector.at(i).getRecordMaxTemperature() )
                                          << std::endl;

                                std::stringstream station_name;
                                station_name << station.getStationName().substr(0, 10) << " : "
                                             << round( cToF( year_vector.at(i).getRecordMaxTemperature() ) );

                                kml_file                                                                                        << std::endl;
                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                kml_file << "        <name>"                                                                    << std::endl;
                                kml_file << "                " << station_name.str()                                            << std::endl;
                                kml_file << "        </name>"                                                                   << std::endl;

                                if ( year_vector.at(i).getRecordMaxTemperature() >= round(g_temperature_threshold_high + (10.0f / 1.8f)) )
                                {
                                    kml_file << "<styleUrl>#redIcon</styleUrl>"                                                 << std::endl;
                                }
                                     else
                                {
                                    kml_file << "<styleUrl>#yellowIcon</styleUrl>"                                                 << std::endl;
                                }

                                kml_file << "<Point>"                                                                           << std::endl;
                                kml_file << "        <coordinates>"                                                             << std::endl;
                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                kml_file << "        </coordinates>"                                                            << std::endl;
                                kml_file << "</Point>"                                                                          << std::endl;
                                kml_file << "</Placemark>"                                                                      << std::endl;
                                kml_file                                                                                        << std::endl;

                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (g_month_to_dump != 0 && g_day_to_dump != 0)
        {
             std::cout << "Stations on " << g_month_to_dump << "/" << g_day_to_dump << "/" << year_for_stats;
             std::cout << " which did reach the maximum threshold of " << g_temperature_threshold_high << " C" << std::endl;

             std::stringstream kml_file_name_stringstream;
             kml_file_name_stringstream << "above_maximum_threshold_" << g_temperature_threshold_high << "_" << g_month_to_dump << g_day_to_dump << year_for_stats << ".kml";
             std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

             kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
             kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
             kml_file << std::endl;
             kml_file << "<Document>" << std::endl;

             kml_file << "<Style id=\"blueIcon\">" << std::endl;
             kml_file << " <IconStyle>" << std::endl;
             kml_file << "  <Icon>" << std::endl;
             kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
             kml_file << "  </Icon>" << std::endl;
             kml_file << " </IconStyle>" << std::endl;
             kml_file << " <LabelStyle>" << std::endl;
             kml_file << "   <color>ffffaaaa</color>" << std::endl;
             kml_file << "   <scale>1.0</scale>" << std::endl;
             kml_file << " </LabelStyle>" << std::endl;
             kml_file << "</Style>" << std::endl;

             kml_file << "<Style id=\"greenIcon\">" << std::endl;
             kml_file << " <IconStyle>" << std::endl;
             kml_file << "  <Icon>" << std::endl;
             kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
             kml_file << "  </Icon>" << std::endl;
             kml_file << " </IconStyle>" << std::endl;
             kml_file << " <LabelStyle>" << std::endl;
             kml_file << "   <color>ff00ff00</color>" << std::endl;
             kml_file << "   <scale>1.0</scale>" << std::endl;
             kml_file << " </LabelStyle>" << std::endl;
             kml_file << "</Style>" << std::endl;

             kml_file << "<Style id=\"yellowIcon\">" << std::endl;
             kml_file << " <IconStyle>" << std::endl;
             kml_file << "  <Icon>" << std::endl;
             kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << std::endl;
             kml_file << "  </Icon>" << std::endl;
             kml_file << " </IconStyle>" << std::endl;
             kml_file << " <LabelStyle>" << std::endl;
             kml_file << "   <color>ffccffff</color>" << std::endl;
             kml_file << "   <scale>1.0</scale>" << std::endl;
             kml_file << " </LabelStyle>" << std::endl;
             kml_file << "</Style>" << std::endl;

             kml_file << "<Style id=\"redIcon\">" << std::endl;
             kml_file << " <IconStyle>" << std::endl;
             kml_file << "  <Icon>" << std::endl;
             kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/red-pushpin.png</href>" << std::endl;
             kml_file << "  </Icon>" << std::endl;
             kml_file << " </IconStyle>" << std::endl;
             kml_file << " <LabelStyle>" << std::endl;
             kml_file << "   <color>ffccccff</color>" << std::endl;
             kml_file << "   <scale>1.0</scale>" << std::endl;
             kml_file << " </LabelStyle>" << std::endl;
             kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();
                    bool already_counted = false;

                    for (size_t i = 0; i < year_vector.size() && !already_counted; i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats && !already_counted)
                        {
                            std::vector<Month>& month_vector = year_vector.at(i).getMonthVector();

                            for (size_t j = 0; j < month_vector.size(); j++)
                            {
                                Month& month = month_vector.at(j);
                                size_t month_number = j + 1;

                                if (month_number == g_month_to_dump && !already_counted)
                                {
                                    std::vector<Day>& day_vector = month.getDayVector();

                                    for (size_t k = 0; k < day_vector.size(); k++)
                                    {
                                        Day& day = day_vector.at(k);
                                        size_t day_number = k + 1;

                                        if (day_number == g_day_to_dump && !already_counted)
                                        {
                                            if (day.getMaxTemperature() != UNKNOWN_TEMPERATURE &&
                                                    day.getMaxTemperature() >= g_temperature_threshold_high )
                                            {
                                                std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName() << ","
                                                          << station.getStationNumber()
                                                          << "," << day.getMaxTemperature()
                                                          << "," << cToF( day.getMaxTemperature() )
                                                          << std::endl;

                                                std::stringstream station_name;
                                                station_name << station.getStationName().substr(0, 10) << " : "
                                                             << round( cToF( day.getMaxTemperature() ) );

                                                kml_file                                                                                        << std::endl;
                                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                                kml_file << "        <name>"                                                                    << std::endl;
                                                kml_file << "                " << station_name.str()                                                  << std::endl;
                                                kml_file << "        </name>"                                                                   << std::endl;
                                                kml_file << "<styleUrl>#redIcon</styleUrl>"                                                     << std::endl;

                                                if ( day.getMaxTemperature() >= round(g_temperature_threshold_high + + (10.0f / 1.8f)) )
                                                {
                                                    kml_file << "<styleUrl>#redIcon</styleUrl>"                                                 << std::endl;
                                                }
                                                else
                                                {
                                                    kml_file << "<styleUrl>#yellowIcon</styleUrl>"                                                 << std::endl;
                                                }

                                                kml_file << "<Point>"                                                                           << std::endl;
                                                kml_file << "        <coordinates>"                                                             << std::endl;
                                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                                kml_file << "        </coordinates>"                                                            << std::endl;
                                                kml_file << "</Point>"                                                                          << std::endl;
                                                kml_file << "</Placemark>"                                                                      << std::endl;
                                                kml_file                                                                                        << std::endl;

                                                already_counted = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (year_for_stats != 0)
        {
            std::cout << "Stations in " << year_for_stats << " which did not reach the minimum threshold of " << g_temperature_threshold_low << " C" << std::endl;

            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "below_minimum_threshold_" << g_temperature_threshold_low << "_" << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"blueIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;


            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();

                    for (size_t i = 0; i < year_vector.size(); i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats)
                        {
                            if ( year_vector.at(i).getRecordMinTemperature() != float(INT_MIN) &&
                                 year_vector.at(i).getRecordMinTemperature() <= g_temperature_threshold_low )
                            {
                                std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName() << ","
                                          << station.getStationNumber() << "," << year_vector.at(i).getRecordMinTemperature() << std::endl;

                                kml_file                                                                                        << std::endl;
                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                kml_file << "        <name>"                                                                    << std::endl;
                                kml_file << "                " << station.getStationName().substr(0, 20)                        << std::endl;
                                kml_file << "        </name>"                                                                   << std::endl;
                                kml_file << "<styleUrl>#blueIcon</styleUrl>"                                                    << std::endl;
                                kml_file << "<Point>"                                                                           << std::endl;
                                kml_file << "        <coordinates>"                                                             << std::endl;
                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                kml_file << "        </coordinates>"                                                            << std::endl;
                                kml_file << "</Point>"                                                                          << std::endl;
                                kml_file << "</Placemark>"                                                                      << std::endl;
                                kml_file                                                                                        << std::endl;

                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (g_month_to_dump != 0 && g_day_to_dump != 0)
        {
            std::cout << "Stations on " << g_month_to_dump << "/" << g_day_to_dump << "/" << year_for_stats;
            std::cout << " which did reach the minimum threshold of " << g_temperature_threshold_low << " C" << std::endl;

            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "below_minimum_threshold_" << g_temperature_threshold_low << "_" << g_month_to_dump << g_day_to_dump << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"blueIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();
                    bool already_counted = false;

                    for (size_t i = 0; i < year_vector.size() && !already_counted; i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats && !already_counted)
                        {
                            std::vector<Month>& month_vector = year_vector.at(i).getMonthVector();

                            for (size_t j = 0; j < month_vector.size(); j++)
                            {
                                Month& month = month_vector.at(j);
                                size_t month_number = j + 1;

                                if (month_number == g_month_to_dump && !already_counted)
                                {
                                    std::vector<Day>& day_vector = month.getDayVector();

                                    for (size_t k = 0; k < day_vector.size(); k++)
                                    {
                                        Day& day = day_vector.at(k);
                                        size_t day_number = k + 1;

                                        if (day_number == g_day_to_dump && !already_counted)
                                        {
                                            if (day.getMinTemperature() != UNKNOWN_TEMPERATURE &&
                                                    day.getMinTemperature() <= g_temperature_threshold_low )
                                            {
                                                std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName() << ","
                                                          << station.getStationNumber() << "," << day.getMinTemperature() << std::endl;

                                                std::stringstream station_name;
                                                station_name << station.getStationName().substr(0, 10) << " : "
                                                             << round( cToF( day.getMinTemperature() ) );

                                                kml_file                                                                                        << std::endl;
                                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                                kml_file << "        <name>"                                                                    << std::endl;
                                                kml_file << "                " << station_name.str()                                                  << std::endl;
                                                kml_file << "        </name>"                                                                   << std::endl;
                                                kml_file << "<styleUrl>#blueIcon</styleUrl>"                                                    << std::endl;
                                                kml_file << "<Point>"                                                                           << std::endl;
                                                kml_file << "        <coordinates>"                                                             << std::endl;
                                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                                kml_file << "        </coordinates>"                                                            << std::endl;
                                                kml_file << "</Point>"                                                                          << std::endl;
                                                kml_file << "</Placemark>"                                                                      << std::endl;
                                                kml_file                                                                                        << std::endl;

                                                already_counted = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (year_for_stats != 0)
        {
            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "received_snow_" << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"whiteIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/wht-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();

                    for (size_t i = 0; i < year_vector.size(); i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats)
                        {
                            if ( year_vector.at(i).getNumberOfDaysWithSnow() > 0 )
                            {
                                kml_file                                                                                        << std::endl;
                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                kml_file << "        <name>"                                                                    << std::endl;
                                kml_file << "                " << station.getStationName().substr(0, 20)                        << std::endl;
                                kml_file << "        </name>"                                                                   << std::endl;
                                kml_file << "<styleUrl>#whiteIcon</styleUrl>"                                                   << std::endl;
                                kml_file << "<Point>"                                                                           << std::endl;
                                kml_file << "        <coordinates>"                                                             << std::endl;
                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                kml_file << "        </coordinates>"                                                            << std::endl;
                                kml_file << "</Point>"                                                                          << std::endl;
                                kml_file << "</Placemark>"                                                                      << std::endl;
                                kml_file                                                                                        << std::endl;
                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        if (g_month_to_dump != 0 && g_day_to_dump != 0)
        {
            std::cout << "Stations on " << g_month_to_dump << "/" << g_day_to_dump << "/" << year_for_stats;
            std::cout << " which received snow " << std::endl;

            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "received_snow_" << g_month_to_dump << g_day_to_dump << year_for_stats << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"whiteIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/wht-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            for (size_t state_number = 0; state_number < state_vector.size(); state_number++)
            {
                std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();

                for (size_t station_number = 0; station_number < station_vector.size(); station_number++)
                {
                    Station& station = *(station_vector[station_number]);
                    std::vector<Year>& year_vector = station.getYearVector();
                    bool already_counted = false;

                    for (size_t i = 0; i < year_vector.size() && !already_counted; i++)
                    {
                        int year = year_vector.at(i).getYear();

                        if (year == (int)year_for_stats && !already_counted)
                        {
                            std::vector<Month>& month_vector = year_vector.at(i).getMonthVector();

                            for (size_t j = 0; j < month_vector.size(); j++)
                            {
                                Month& month = month_vector.at(j);
                                size_t month_number = j + 1;

                                if (month_number == g_month_to_dump && !already_counted)
                                {
                                    std::vector<Day>& day_vector = month.getDayVector();

                                    for (size_t k = 0; k < day_vector.size(); k++)
                                    {
                                        Day& day = day_vector.at(k);
                                        size_t day_number = k + 1;

                                        if (day_number == g_day_to_dump && !already_counted)
                                        {
                                            if (day.getSnowfall() != UNKNOWN_TEMPERATURE &&
                                                    day.getSnowfall() > 0 )
                                            {
                                                std::cout << station.getStationName().substr(0, 20) << "," << station.getStateName() << ","
                                                          << station.getStationNumber() << "," << day.getSnowfall() << std::endl;

                                                kml_file                                                                                        << std::endl;
                                                kml_file << "<Placemark id=\"" << station.getStationName().substr(0, 20) << "\">"               << std::endl;
                                                kml_file << "        <name>"                                                                    << std::endl;
                                                kml_file << "                " << station.getStationName().substr(0, 20)                        << std::endl;
                                                kml_file << "        </name>"                                                                   << std::endl;
                                                kml_file << "<styleUrl>#whiteIcon</styleUrl>"                                                   << std::endl;
                                                kml_file << "<Point>"                                                                           << std::endl;
                                                kml_file << "        <coordinates>"                                                             << std::endl;
                                                kml_file << "                " << station.getLongitude() << "," << station.getLatitude()        << std::endl;
                                                kml_file << "        </coordinates>"                                                            << std::endl;
                                                kml_file << "</Point>"                                                                          << std::endl;
                                                kml_file << "</Placemark>"                                                                      << std::endl;
                                                kml_file                                                                                        << std::endl;

                                                already_counted = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();
        }

        std::cout << "Year,Number Of Stations,Average Latitude,Average Longitude,Record Maximums,Record Minimums,All Time Record Maximums,All Time Record Minimums";
        std::cout << ",All Time Record Monthly Maximums,All Time Record Monthly Minimums";
        std::cout << ",Record Precipitations,Record Snowfalls" << std::endl;
        //std::map<unsigned int,unsigned int>::iterator itmax = record_max_per_year_map.begin();

        int graphing_index = 0;

        for (unsigned int year = g_first_year; year <= g_last_year; year++)
        {
            float average_latitude = total_latitude_per_year_map[year] / double(number_of_active_stations_per_year_map[year]);
            float average_longitude = total_longitude_per_year_map[year] / double(number_of_active_stations_per_year_map[year]);
            std::cout << year << "," << number_of_active_stations_per_year_map[year] << "," << average_latitude << "," << average_longitude << "," << record_max_per_year_map[year] << "," << record_min_per_year_map[year];
            std::cout << "," << number_of_all_time_record_maximums_per_year_map[year] << "," << number_of_all_time_record_minimums_per_year_map[year];
            std::cout << "," << record_monthly_max_per_year_map[year] << "," << record_monthly_min_per_year_map[year];
            std::cout << "," << number_of_all_time_record_precipitations_per_year_map[year] << "," << number_of_all_time_record_snowfalls_per_year_map[year];
            std::cout << std::endl;

            g_result_pair[GRAPHING_OPTION_NUMBER_OF_DAILY_MAXIMUM_TEMPERATURE_RECORDS][graphing_index].first = year;
            g_result_pair[GRAPHING_OPTION_NUMBER_OF_DAILY_MAXIMUM_TEMPERATURE_RECORDS][graphing_index].second = record_max_per_year_map[year];
            g_result_pair[GRAPHING_OPTION_NUMBER_OF_DAILY_MINIMUM_TEMPERATURE_RECORDS][graphing_index].first = year;
            g_result_pair[GRAPHING_OPTION_NUMBER_OF_DAILY_MINIMUM_TEMPERATURE_RECORDS][graphing_index].second = record_min_per_year_map[year];
            graphing_index++;
        }

        //while ( itmax != record_max_per_year_map.end() )
        //{
        //    std::cout << itmax->first << "," << itmax->second << "," << std::endl;
        //    ++itmax;
        //}

        //std::cout << "Record Minimums," << std::endl;
        //std::map<unsigned int,unsigned int>::iterator itmin = record_min_per_year_map.begin();

        //while ( itmin != record_min_per_year_map.end() )
        //{
        //    std::cout << itmin->first << "," << itmin->second << "," << std::endl;
        //    ++itmin;
        //}
#ifdef PRINT_INCREMENTALS
        std::cout << "Record Incremental Maximums," << std::endl;
        itmax = record_incremental_max_per_year_map.begin();

        while ( itmax != record_incremental_max_per_year_map.end() )
        {
            std::cout << itmax->first << "," << itmax->second << "," << std::endl;
            ++itmax;
        }

        std::cout << "Record Incremental Minimums," << std::endl;
        itmin = record_incremental_min_per_year_map.begin();

        while ( itmin != record_incremental_min_per_year_map.end() )
        {
            std::cout << itmin->first << "," << itmin->second << "," << std::endl;
            ++itmin;
        }
#endif
#ifdef PRINT_TMAX_TMIN_RATIO
        std::cout << "Ratio Tmax/Tmin," << std::endl;
        itmax = record_max_per_year_map.begin();
        itmin = record_min_per_year_map.begin();

        while ( itmin != record_min_per_year_map.end() && itmax != record_max_per_year_map.end() )
        {
            float ratio = float(itmax->second) / float(itmin->second);
            std::cout << itmin->first << "," << ratio << "," << std::endl;
            ++itmax;
            ++itmin;
        }
#endif
        std::vector<double> year_vector;
        std::vector<double> temperature_vector;
        std::vector<double> state_year_vector[NUMBER_OF_STATES];
        std::vector<double> state_temperature_vector[NUMBER_OF_STATES];
        std::vector<double> state_average_trend_vector(NUMBER_OF_STATES);
        std::vector<double> state_maximum_trend_vector(NUMBER_OF_STATES);
        std::vector<double> state_minimum_trend_vector(NUMBER_OF_STATES);
        std::vector<double> month_year_vector[NUMBER_OF_MONTHS_PER_YEAR];
        std::vector<double> month_temperature_vector[NUMBER_OF_MONTHS_PER_YEAR];
        double slope;
        double x_offset;

        std::cout << "Average anomaly";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
        }

        std::cout << std::endl;


        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (number_of_max_readings_per_year_map_per_state_vector[state_number][year_number] == 0
                        || number_of_min_readings_per_year_map_per_state_vector[state_number][year_number] == 0
                        || year_number < USHCN_FIRST_YEAR)
                {
                     std::cout << ",";
                }
                else
                {
                    double average_max = total_max_deviation_per_year_map_per_state_vector[state_number][year_number] /
                            number_of_max_readings_per_year_map_per_state_vector[state_number][year_number];
                    double average_min = total_min_deviation_per_year_map_per_state_vector[state_number][year_number] /
                            number_of_min_readings_per_year_map_per_state_vector[state_number][year_number];
                    double average = (average_max + average_min) / 2;
                    std::cout << "," << average;

                    state_year_vector[state_number].push_back( double(year_number) );
                    state_temperature_vector[state_number].push_back( double(average) );
                }
            }

            std::cout << std::endl;
        }

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            linearRegression(slope, x_offset, state_year_vector[state_number], state_temperature_vector[state_number]);
            std::cout << "Average anomaly trend," << STATE_NAMES[state_number + 1] << "," << std::setprecision(3) << slope * 100 << std::endl;
            state_average_trend_vector[state_number] = slope;
        }

        {
            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "average_anomaly_trend" << arguments_string << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"blueIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffffaaaa</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"greenIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ff00</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"yellowIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ffff</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"redIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/red-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffaaaaff</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            std::vector<Station*>& station_vector = state_vector.at(0).getStationVector();
            std::string state_name = "";

            for (size_t i = 0; i < station_vector.size(); i++)
            {
                Station* station = station_vector[i];

                //if ( station->getStateName() != state_name )
                {
                    size_t state_number = state_number_map[station->getStationName()];

                    if ( state_number >= state_average_trend_vector.size() )
                    {
                        continue;
                    }

                    double slope = state_average_trend_vector[state_number];

                    kml_file                                                                                        << std::endl;
                    kml_file << "<Placemark id=\"" << station->getStationName().substr(0, 20) << "\">"              << std::endl;

                    if ( station->getStateName() != state_name )
                    {
                        kml_file << "        <name>"                                                                    << std::endl;
                        kml_file << "                " << std::setprecision(3) << slope * 100                           << std::endl;
                        kml_file << "        </name>"                                                                   << std::endl;
                    }

                    if (state_average_trend_vector[state_number] > 0.01)
                    {
                        kml_file << "<styleUrl>#redIcon</styleUrl>"                                                 << std::endl;
                    }
                    else if (state_average_trend_vector[state_number] > 0.0)
                    {
                        kml_file << "<styleUrl>#yellowIcon</styleUrl>"                                              << std::endl;
                    }
                    else if (state_average_trend_vector[state_number] > -0.01)
                    {
                        kml_file << "<styleUrl>#greenIcon</styleUrl>"                                               << std::endl;
                    }
                    else
                    {
                        kml_file << "<styleUrl>#blueIcon</styleUrl>"                                                << std::endl;
                    }

                    kml_file << "<Point>"                                                                           << std::endl;
                    kml_file << "        <coordinates>"                                                             << std::endl;
                    kml_file << "                " << station->getLongitude() << "," << station->getLatitude()      << std::endl;
                    kml_file << "        </coordinates>"                                                            << std::endl;
                    kml_file << "</Point>"                                                                          << std::endl;
                    kml_file << "</Placemark>"                                                                      << std::endl;
                    kml_file                                                                                        << std::endl;

                    state_name = station->getStateName();
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();

        }

        std::cout << "Average temperature,Year,Anomaly C,Anomaly F,,,Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec,Annual C,Annual F,Winter C,Spring C,Summer C,Fall C" << std::endl;
        std::map<unsigned int,float>::iterator temperature_it = total_temperature_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator count_it = number_of_readings_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator max_count_it = number_of_max_readings_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator min_count_it = number_of_min_readings_per_year_map.begin();

        std::map<unsigned int,float>::iterator max_deviation_it = total_max_deviation_per_year_map.begin();
        std::map<unsigned int,float>::iterator min_deviation_it = total_min_deviation_per_year_map.begin();
        std::vector<float> maximum_month_running_total_vector;
        std::vector<float> minimum_month_running_total_vector;
        std::vector<float> average_month_running_total_vector;
        float total_temperature = 0.0f;
        int consecutive_count = 0;
        size_t previous_month_number = 0;

        while ( temperature_it != total_temperature_per_year_map.end() && count_it != number_of_readings_per_year_map.end() )
        {
            float average = float(temperature_it->second) / float(count_it->second);
            average = (
                        ( max_deviation_it->second / float(max_count_it->second) ) +
                        ( min_deviation_it->second / float(min_count_it->second) )
                        ) / 2;

            if ( isnan(average) )
            {
                average = 0;
            }

            std::cout << "Average temperature," << temperature_it->first << "," << average << "," << (average * 1.8) << "," << count_it->second << ",,";


            unsigned int year = temperature_it->first;

            //if (g_current_graphing_option == GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY)
            {
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY][graphing_index].second = average;

                if ( !isnan(average) && year >= g_first_year)
                {
                    year_vector.push_back( double(year) );
                    temperature_vector.push_back( double(average) );
                }
            }

            float annual_monthly_sum = 0.0f;
            size_t number_of_valid_months = 0;
            float average_summer_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_readings_per_month[year - FIRST_YEAR][5] &&
                 number_of_readings_per_month[year - FIRST_YEAR][6] &&
                 number_of_readings_per_month[year - FIRST_YEAR][7] )
            {
                average_summer_temperature =
                        ( total_temperature_per_month[year - FIRST_YEAR][5] +
                        total_temperature_per_month[year - FIRST_YEAR][6] +
                        total_temperature_per_month[year - FIRST_YEAR][7] ) /
                        ( number_of_readings_per_month[year - FIRST_YEAR][5] +
                        number_of_readings_per_month[year - FIRST_YEAR][6] +
                        number_of_readings_per_month[year - FIRST_YEAR][7] );
            }

            float average_winter_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( (year > FIRST_YEAR) &&
                 number_of_readings_per_month[year - FIRST_YEAR - 1][11] &&
                 number_of_readings_per_month[year - FIRST_YEAR][0] &&
                 number_of_readings_per_month[year - FIRST_YEAR][1] )
            {
                average_winter_temperature =
                        ( total_temperature_per_month[year - FIRST_YEAR - 1][11] +
                        total_temperature_per_month[year - FIRST_YEAR][0] +
                        total_temperature_per_month[year - FIRST_YEAR][1] ) /
                        ( number_of_readings_per_month[year - FIRST_YEAR - 1][11] +
                        number_of_readings_per_month[year - FIRST_YEAR][0] +
                        number_of_readings_per_month[year - FIRST_YEAR][1] );
            }

            float average_spring_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_readings_per_month[year - FIRST_YEAR][2] &&
                 number_of_readings_per_month[year - FIRST_YEAR][3] &&
                 number_of_readings_per_month[year - FIRST_YEAR][4] )
            {
                average_spring_temperature =
                        ( total_temperature_per_month[year - FIRST_YEAR][2] +
                        total_temperature_per_month[year - FIRST_YEAR][3] +
                        total_temperature_per_month[year - FIRST_YEAR][4] ) /
                        ( number_of_readings_per_month[year - FIRST_YEAR][2] +
                        number_of_readings_per_month[year - FIRST_YEAR][3] +
                        number_of_readings_per_month[year - FIRST_YEAR][4] );
            }

            float average_fall_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_readings_per_month[year - FIRST_YEAR][8] &&
                 number_of_readings_per_month[year - FIRST_YEAR][9] &&
                 number_of_readings_per_month[year - FIRST_YEAR][10] )
            {
                average_fall_temperature =
                        ( total_temperature_per_month[year - FIRST_YEAR][8] +
                        total_temperature_per_month[year - FIRST_YEAR][9] +
                        total_temperature_per_month[year - FIRST_YEAR][10] ) /
                        ( number_of_readings_per_month[year - FIRST_YEAR][8] +
                        number_of_readings_per_month[year - FIRST_YEAR][9] +
                        number_of_readings_per_month[year - FIRST_YEAR][10] );
            }

            for (size_t month = 0; month < NUMBER_OF_MONTHS_PER_YEAR; month++)
            {
                float monthly_average = UNKNOWN_TEMPERATURE;
                size_t month_number = (year * NUMBER_OF_MONTHS_PER_YEAR) + month;

                if ( number_of_readings_per_month[year - FIRST_YEAR][month] )
                {
                    if ( (month_number - previous_month_number) == 1 )
                    {
                        consecutive_count ++;
                    }
                    else
                    {
                        consecutive_count = 0;
                    }

                    monthly_average = total_temperature_per_month[year - FIRST_YEAR][month] / number_of_readings_per_month[year - FIRST_YEAR][month];
                    total_temperature += monthly_average;
                    average_month_running_total_vector.push_back(total_temperature);
                    annual_monthly_sum += monthly_average;
                    number_of_valid_months++;

                    month_year_vector[month].push_back(year);
                    month_temperature_vector[month].push_back(monthly_average);

                    if (consecutive_count >= number_of_months_under_test)
                    {
                        size_t size = average_month_running_total_vector.size();
                        float total_variable_month_temperature = average_month_running_total_vector.at(size - 1) - average_month_running_total_vector.at(size - 1 - number_of_months_under_test);
                        float average_temperature = total_variable_month_temperature / float(number_of_months_under_test);
                        country.getVariableMonthMeanAverageMap()[average_temperature] = month_number;
                    }

                    previous_month_number = month_number;
                }

                if (monthly_average == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << monthly_average << ",";
                }
            }

            if (number_of_valid_months)
            {
                float annual_monthly_average = annual_monthly_sum / float(number_of_valid_months);
                float annual_monthly_average_f = (annual_monthly_average * 1.8f) + 32.0f;
                std::cout << annual_monthly_average << "," << annual_monthly_average_f;

                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE][graphing_index].second = annual_monthly_average;
                g_result_pair[GRAPHING_OPTION_WINTER_MEAN_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_SPRING_MEAN_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_SUMMER_MEAN_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_AUTUMN_MEAN_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_WINTER_MEAN_TEMPERATURE][graphing_index].second = average_winter_temperature;
                g_result_pair[GRAPHING_OPTION_SPRING_MEAN_TEMPERATURE][graphing_index].second = average_spring_temperature;
                g_result_pair[GRAPHING_OPTION_SUMMER_MEAN_TEMPERATURE][graphing_index].second = average_summer_temperature;
                g_result_pair[GRAPHING_OPTION_AUTUMN_MEAN_TEMPERATURE][graphing_index].second = average_fall_temperature;

                if (average_winter_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_winter_temperature;
                }

                if (average_spring_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_spring_temperature;
                }

                if (average_summer_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_summer_temperature;
                }

                if (average_fall_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_fall_temperature;
                }
            }
            else
            {
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
            }

            std::cout << std::endl;

            temperature_it++;
            count_it++;
            max_count_it++;
            min_count_it++;
            max_deviation_it++;
            min_deviation_it++;
            graphing_index++;
        }

        linearRegression(slope, x_offset, year_vector, temperature_vector);
        std::cout << "Average temperature trend,,,,,," << std::setprecision(3) << slope * 100 << std::endl;
        std::cout << "Monthly temperature trend,,,,,";

        for (size_t month_number = 0; month_number < NUMBER_OF_MONTHS_PER_YEAR; month_number++)
        {
            linearRegression(slope, x_offset, month_year_vector[month_number], month_temperature_vector[month_number]);
            std::cout << "," << std::setprecision(3) << slope * 100;
        }

        std::cout << std::endl;


#if 0
        std::cout << "Hottest Average" << number_of_months_under_test << " month periods " << std::endl;
        std::cout << "Rank," << "Month," << "Year," << "Temperature " << std::endl;
        std::map<float,size_t>::reverse_iterator reverse_variable_month_iterator = country.getVariableMonthMeanAverageMap().rbegin();
        std::map<float,size_t>::iterator forward_variable_month_iterator = country.getVariableMonthMeanAverageMap().begin();
        size_t count = 1;

        for ( ; reverse_variable_month_iterator != country.getVariableMonthMeanAverageMap().rend(), count < 20; reverse_variable_month_iterator++ )
        {
            unsigned int year = reverse_variable_month_iterator->second / NUMBER_OF_MONTHS_PER_YEAR;
            unsigned int month = reverse_variable_month_iterator->second % NUMBER_OF_MONTHS_PER_YEAR;
            float temperature = reverse_variable_month_iterator->first;

            std::cout << count++ << "," << month + 1 << "," << year << "," << temperature << std::endl;
        }

        std::cout << "Coldest Average" << number_of_months_under_test << " month periods " << std::endl;
        std::cout << "Rank," << "Month," << "Year," << "Temperature " << std::endl;
        count = 1;

        for ( ; forward_variable_month_iterator != country.getVariableMonthMeanAverageMap().end(), count < 20; forward_variable_month_iterator++ )
        {
            unsigned int year = forward_variable_month_iterator->second / NUMBER_OF_MONTHS_PER_YEAR;
            unsigned int month = forward_variable_month_iterator->second % NUMBER_OF_MONTHS_PER_YEAR;
            float temperature = forward_variable_month_iterator->first;

            std::cout << count++ << "," << month + 1 << "," << year << "," << temperature << std::endl;
        }

#endif      
        for (size_t month_number = 0; month_number < NUMBER_OF_MONTHS_PER_YEAR; month_number++)
        {
            month_year_vector[month_number].clear();
            month_temperature_vector[month_number].clear();
        }

        std::cout << "Maximum anomaly";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;


        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (number_of_max_readings_per_year_map_per_state_vector[state_number][year_number] == 0 || year_number < USHCN_FIRST_YEAR)
                {
                     std::cout << ",";
                }
                else
                {
                    double average = total_max_deviation_per_year_map_per_state_vector[state_number][year_number] /
                            number_of_max_readings_per_year_map_per_state_vector[state_number][year_number];
                    std::cout << "," << average;

                    state_year_vector[state_number].push_back( double(year_number) );
                    state_temperature_vector[state_number].push_back( double(average) );
                }
            }

            std::cout << std::endl;
        }

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            linearRegression(slope, x_offset, state_year_vector[state_number], state_temperature_vector[state_number]);
            std::cout << "Maximum anomaly trend," << STATE_NAMES[state_number + 1] << "," << std::setprecision(3) << slope * 100 << std::endl;
            state_maximum_trend_vector[state_number] = slope;
        }

        {
            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "maximum_anomaly_trend" << arguments_string << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"blueIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffffaaaa</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"greenIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ff00</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"yellowIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ffff</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"redIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/red-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffaaaaff</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            std::vector<Station*>& station_vector = state_vector.at(0).getStationVector();
            std::string state_name = "";

            for (size_t i = 0; i < station_vector.size(); i++)
            {
                Station* station = station_vector[i];

                //if ( station->getStateName() != state_name )
                {
                    size_t state_number = state_number_map[station->getStationName()];

                    if ( state_number >= state_maximum_trend_vector.size() )
                    {
                        continue;
                    }

                    double slope = state_maximum_trend_vector[state_number];

                    kml_file                                                                                        << std::endl;
                    kml_file << "<Placemark id=\"" << station->getStationName().substr(0, 20) << "\">"              << std::endl;

                    if ( station->getStateName() != state_name )
                    {
                        kml_file << "        <name>"                                                                    << std::endl;
                        kml_file << "                " << std::setprecision(3) << slope * 100                           << std::endl;
                        kml_file << "        </name>"                                                                   << std::endl;
                    }

                    if (state_maximum_trend_vector[state_number] > 0.01)
                    {
                        kml_file << "<styleUrl>#redIcon</styleUrl>"                                                 << std::endl;
                    }
                    else if (state_maximum_trend_vector[state_number] > 0.0)
                    {
                        kml_file << "<styleUrl>#yellowIcon</styleUrl>"                                              << std::endl;
                    }
                    else if (state_maximum_trend_vector[state_number] > -0.01)
                    {
                        kml_file << "<styleUrl>#greenIcon</styleUrl>"                                               << std::endl;
                    }
                    else
                    {
                        kml_file << "<styleUrl>#blueIcon</styleUrl>"                                                << std::endl;
                    }

                    kml_file << "<Point>"                                                                           << std::endl;
                    kml_file << "        <coordinates>"                                                             << std::endl;
                    kml_file << "                " << station->getLongitude() << "," << station->getLatitude()        << std::endl;
                    kml_file << "        </coordinates>"                                                            << std::endl;
                    kml_file << "</Point>"                                                                          << std::endl;
                    kml_file << "</Placemark>"                                                                      << std::endl;
                    kml_file                                                                                        << std::endl;

                    state_name = station->getStateName();
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();

        }


        std::cout << "Hottest Temperature";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (hottest_temperature_per_year_map_per_state_vector[state_number][year_number] == float(UNREASONABLE_LOW_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    float hottest = hottest_temperature_per_year_map_per_state_vector[state_number][year_number];
                    std::cout << "," << hottest;
                    state_year_vector[state_number].push_back( double(year_number) );
                    state_temperature_vector[state_number].push_back( double(hottest) );
                }
            }

            std::cout << std::endl;
        }

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            linearRegression(slope, x_offset, state_year_vector[state_number], state_temperature_vector[state_number]);
            std::cout << "Hottest temperature trend," << STATE_NAMES[state_number + 1] << "," << std::setprecision(3) << slope * 100 << std::endl;
        }

        std::cout << ",Maximum temperature,Anomaly C,Anomaly F,,,Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec,Annual C,Annual F,Winter C,Spring C,Summer C,Fall C" << std::endl;
        temperature_it = total_max_temperature_per_year_map.begin();
        max_deviation_it = total_max_deviation_per_year_map.begin();
        count_it = number_of_max_readings_per_year_map.begin();
        graphing_index = 0;
        year_vector.clear();
        temperature_vector.clear();

        while ( max_deviation_it != total_max_deviation_per_year_map.end() && count_it != number_of_max_readings_per_year_map.end() )
        {
            float average = max_deviation_it->second / (float)count_it->second;

            if ( isnan(average) )
            {
                average = 0;
            }

            //std::cerr << count_it->first << " " << count_it->second << " " << max_deviation_it->second << std::endl;
            std::cout << "Maximum," << max_deviation_it->first << "," << average << "," << (average * 1.8) << "," << count_it->second << ",,";

            unsigned int year = temperature_it->first;

            //if (g_current_graphing_option == GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ANOMALY)
            {
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ANOMALY][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ANOMALY][graphing_index].second = average;

                if ( !isnan(average) && year >= g_first_year)
                {
                    year_vector.push_back( double(year) );
                    temperature_vector.push_back( double(average) );
                }
            }

            float annual_monthly_sum = 0.0f;
            size_t number_of_valid_months = 0;
            float average_summer_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_max_readings_per_month[year - FIRST_YEAR][5] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][6] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][7] )
            {
                average_summer_temperature =
                        ( total_max_temperature_per_month[year - FIRST_YEAR][5] +
                        total_max_temperature_per_month[year - FIRST_YEAR][6] +
                        total_max_temperature_per_month[year - FIRST_YEAR][7] ) /
                        ( number_of_max_readings_per_month[year - FIRST_YEAR][5] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][6] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][7] );
            }

            float average_winter_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( (year > FIRST_YEAR) &&
                 number_of_max_readings_per_month[year - FIRST_YEAR - 1][11] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][0] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][1] )
            {
                average_winter_temperature =
                        ( total_max_temperature_per_month[year - FIRST_YEAR - 1][11] +
                        total_max_temperature_per_month[year - FIRST_YEAR][0] +
                        total_max_temperature_per_month[year - FIRST_YEAR][1] ) /
                        ( number_of_max_readings_per_month[year - FIRST_YEAR - 1][11] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][0] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][1] );
            }

            float average_spring_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_max_readings_per_month[year - FIRST_YEAR][2] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][3] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][4] )
            {
                average_spring_temperature =
                        ( total_max_temperature_per_month[year - FIRST_YEAR][2] +
                        total_max_temperature_per_month[year - FIRST_YEAR][3] +
                        total_max_temperature_per_month[year - FIRST_YEAR][4] ) /
                        ( number_of_max_readings_per_month[year - FIRST_YEAR][2] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][3] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][4] );
            }

            float average_fall_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_max_readings_per_month[year - FIRST_YEAR][8] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][9] &&
                 number_of_max_readings_per_month[year - FIRST_YEAR][10] )
            {
                average_fall_temperature =
                        ( total_max_temperature_per_month[year - FIRST_YEAR][8] +
                        total_max_temperature_per_month[year - FIRST_YEAR][9] +
                        total_max_temperature_per_month[year - FIRST_YEAR][10] ) /
                        ( number_of_max_readings_per_month[year - FIRST_YEAR][8] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][9] +
                        number_of_max_readings_per_month[year - FIRST_YEAR][10] );
            }


            for (size_t month = 0; month < NUMBER_OF_MONTHS_PER_YEAR; month++)
            {
                float monthly_average = UNKNOWN_TEMPERATURE;
                size_t month_number = (year * NUMBER_OF_MONTHS_PER_YEAR) + month;

                if ( number_of_max_readings_per_month[year - FIRST_YEAR][month] )
                {
                    if ( (month_number - previous_month_number) == 1 )
                    {
                        consecutive_count ++;
                    }
                    else
                    {
                        consecutive_count = 0;
                    }

                    monthly_average = total_max_temperature_per_month[year - FIRST_YEAR][month] / number_of_max_readings_per_month[year - FIRST_YEAR][month];
                    total_temperature += monthly_average;
                    maximum_month_running_total_vector.push_back(total_temperature);
                    annual_monthly_sum += monthly_average;
                    number_of_valid_months++;

                    month_year_vector[month].push_back(year);
                    month_temperature_vector[month].push_back(monthly_average);

                    if (consecutive_count >= number_of_months_under_test)
                    {
                        size_t size = maximum_month_running_total_vector.size();
                        float total_variable_month_temperature = maximum_month_running_total_vector.at(size - 1) - maximum_month_running_total_vector.at(size - 1 - number_of_months_under_test);
                        float average_temperature = total_variable_month_temperature / float(number_of_months_under_test);
                        country.getVariableMonthMeanMaximumMap()[average_temperature] = month_number;
                    }

                    previous_month_number = month_number;
                }

                if (monthly_average == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << monthly_average << ",";
                }
            }

            if (number_of_valid_months)
            {
                float annual_monthly_average = annual_monthly_sum / float(number_of_valid_months);
                float annual_monthly_average_f = (annual_monthly_average * 1.8f) + 32.0f;
                std::cout << annual_monthly_average << "," << annual_monthly_average_f;

                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE][graphing_index].second = annual_monthly_average;
                g_result_pair[GRAPHING_OPTION_WINTER_MAXIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_SPRING_MAXIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_SUMMER_MAXIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_AUTUMN_MAXIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_WINTER_MAXIMUM_TEMPERATURE][graphing_index].second = average_winter_temperature;
                g_result_pair[GRAPHING_OPTION_SPRING_MAXIMUM_TEMPERATURE][graphing_index].second = average_spring_temperature;
                g_result_pair[GRAPHING_OPTION_SUMMER_MAXIMUM_TEMPERATURE][graphing_index].second = average_summer_temperature;
                g_result_pair[GRAPHING_OPTION_AUTUMN_MAXIMUM_TEMPERATURE][graphing_index].second = average_fall_temperature;

                if (average_winter_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_winter_temperature;
                }

                if (average_spring_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_spring_temperature;
                }

                if (average_summer_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_summer_temperature;
                }

                if (average_fall_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_fall_temperature;
                }
            }
            else
            {
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ANOMALY][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ANOMALY][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
            }

            std::cout << std::endl;

            max_deviation_it++;
            temperature_it++;
            count_it++;
            graphing_index++;
        }

        linearRegression(slope, x_offset, year_vector, temperature_vector);
        std::cout << "Maximum temperature trend,,,,,," << std::setprecision(3) << slope * 100 << std::endl;
        std::cout << "Monthly maximum emperature trend,,,,,";

        for (size_t month_number = 0; month_number < NUMBER_OF_MONTHS_PER_YEAR; month_number++)
        {
            linearRegression(slope, x_offset, month_year_vector[month_number], month_temperature_vector[month_number]);
            std::cout << "," << std::setprecision(3) << slope * 100;
        }

        std::cout << std::endl;

#if 0
        std::cout << "Hottest Maximum" << number_of_months_under_test << " month periods " << std::endl;
        std::cout << "Rank," << "Month," << "Year," << "Temperature " << std::endl;
        reverse_variable_month_iterator = country.getVariableMonthMeanMaximumMap().rbegin();
        count = 1;

        for ( ; reverse_variable_month_iterator != country.getVariableMonthMeanMaximumMap().rend(), count < 20; reverse_variable_month_iterator++ )
        {
            unsigned int year = reverse_variable_month_iterator->second / NUMBER_OF_MONTHS_PER_YEAR;
            unsigned int month = reverse_variable_month_iterator->second % NUMBER_OF_MONTHS_PER_YEAR;
            float temperature = reverse_variable_month_iterator->first;

            std::cout << count++ << "," << month + 1 << "," << year << "," << temperature << std::endl;
        }

        std::cout << "Coldest Maximum" << number_of_months_under_test << " month periods " << std::endl;
        std::cout << "Rank," << "Month," << "Year," << "Temperature " << std::endl;
        forward_variable_month_iterator = country.getVariableMonthMeanMaximumMap().begin();
        count = 1;

        for ( ; forward_variable_month_iterator != country.getVariableMonthMeanMaximumMap().end(), count < 20; forward_variable_month_iterator++ )
        {
            unsigned int year = forward_variable_month_iterator->second / NUMBER_OF_MONTHS_PER_YEAR;
            unsigned int month = forward_variable_month_iterator->second % NUMBER_OF_MONTHS_PER_YEAR;
            float temperature = forward_variable_month_iterator->first;

            std::cout << count++ << "," << month + 1 << "," << year << "," << temperature << std::endl;
        }
#endif
        for (size_t month_number = 0; month_number < NUMBER_OF_MONTHS_PER_YEAR; month_number++)
        {
            month_year_vector[month_number].clear();
            month_temperature_vector[month_number].clear();
        }

        std::cout << "Minimum anomaly";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (number_of_min_readings_per_year_map_per_state_vector[state_number][year_number] == 0
                        || year_number < USHCN_FIRST_YEAR)
                {
                     std::cout << ",";
                }
                else
                {
                    double average = total_min_deviation_per_year_map_per_state_vector[state_number][year_number] /
                            number_of_min_readings_per_year_map_per_state_vector[state_number][year_number];
                    std::cout << "," << average;

                    state_year_vector[state_number].push_back( double(year_number) );
                    state_temperature_vector[state_number].push_back( double(average) );
                }
            }

            std::cout << std::endl;
        }

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            linearRegression(slope, x_offset, state_year_vector[state_number], state_temperature_vector[state_number]);
            std::cout << "Minimum anomaly trend," << STATE_NAMES[state_number + 1] << "," << std::setprecision(3) << slope * 100 << std::endl;
            state_minimum_trend_vector[state_number] = slope;
        }

        {
            std::stringstream kml_file_name_stringstream;
            kml_file_name_stringstream << "minimum_anomaly_trend" << arguments_string << ".kml";
            std::ofstream kml_file( kml_file_name_stringstream.str().c_str() );

            kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            kml_file << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << std::endl;
            kml_file << std::endl;
            kml_file << "<Document>" << std::endl;

            kml_file << "<Style id=\"blueIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffffaaaa</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"greenIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ff00</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"yellowIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ff00ffff</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            kml_file << "<Style id=\"redIcon\">" << std::endl;
            kml_file << " <IconStyle>" << std::endl;
            kml_file << "  <Icon>" << std::endl;
            kml_file << "   <href>http://maps.google.com/mapfiles/kml/pushpin/red-pushpin.png</href>" << std::endl;
            kml_file << "  </Icon>" << std::endl;
            kml_file << " </IconStyle>" << std::endl;
            kml_file << " <LabelStyle>" << std::endl;
            kml_file << "   <color>ffaaaaff</color>" << std::endl;
            kml_file << "   <scale>2.0</scale>" << std::endl;
            kml_file << " </LabelStyle>" << std::endl;
            kml_file << "</Style>" << std::endl;

            std::vector<Station*>& station_vector = state_vector.at(0).getStationVector();
            std::string state_name = "";

            for (size_t i = 0; i < station_vector.size(); i++)
            {
                Station* station = station_vector[i];

                //if ( station->getStateName() != state_name )
                {
                    size_t state_number = state_number_map[station->getStationName()];

                    if ( state_number >= state_minimum_trend_vector.size() )
                    {
                        continue;
                    }

                    double slope = state_minimum_trend_vector[state_number];

                    kml_file                                                                                        << std::endl;
                    kml_file << "<Placemark id=\"" << station->getStationName().substr(0, 20) << "\">"              << std::endl;

                    if ( station->getStateName() != state_name )
                    {
                        kml_file << "        <name>"                                                                    << std::endl;
                        kml_file << "                " << std::setprecision(3) << slope * 100                           << std::endl;
                        kml_file << "        </name>"                                                                   << std::endl;
                    }

                    if (state_minimum_trend_vector[state_number] > 0.01)
                    {
                        kml_file << "<styleUrl>#redIcon</styleUrl>"                                                 << std::endl;
                    }
                    else if (state_minimum_trend_vector[state_number] > 0.0)
                    {
                        kml_file << "<styleUrl>#yellowIcon</styleUrl>"                                              << std::endl;
                    }
                    else if (state_minimum_trend_vector[state_number] > -0.01)
                    {
                        kml_file << "<styleUrl>#greenIcon</styleUrl>"                                               << std::endl;
                    }
                    else
                    {
                        kml_file << "<styleUrl>#blueIcon</styleUrl>"                                                << std::endl;
                    }

                    kml_file << "<Point>"                                                                           << std::endl;
                    kml_file << "        <coordinates>"                                                             << std::endl;
                    kml_file << "                " << station->getLongitude() << "," << station->getLatitude()        << std::endl;
                    kml_file << "        </coordinates>"                                                            << std::endl;
                    kml_file << "</Point>"                                                                          << std::endl;
                    kml_file << "</Placemark>"                                                                      << std::endl;
                    kml_file                                                                                        << std::endl;

                    state_name = station->getStateName();
                }
            }

            kml_file << "</Document>" << std::endl;
            kml_file << "</kml>" << std::endl;
            kml_file.close();

        }

        std::cout << "Coldest Temperature";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (coldest_temperature_per_year_map_per_state_vector[state_number][year_number] == float(UNREASONABLE_HIGH_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    float coldest = coldest_temperature_per_year_map_per_state_vector[state_number][year_number];
                    std::cout << "," << coldest;
                    state_year_vector[state_number].push_back( double(year_number) );
                    state_temperature_vector[state_number].push_back( double(coldest) );
                }
            }

            std::cout << std::endl;
        }

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            linearRegression(slope, x_offset, state_year_vector[state_number], state_temperature_vector[state_number]);
            std::cout << "Coldest temperature trend," << STATE_NAMES[state_number + 1] << "," << std::setprecision(3) << slope * 100 << std::endl;
        }

        std::cout << ",Minimum,Anomaly C,Anomaly F,,,Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec,Annual C,Annual F,Winter C,Spring C,Summer C,Fall C" << std::endl;
        temperature_it = total_min_temperature_per_year_map.begin();
        min_deviation_it = total_min_deviation_per_year_map.begin();
        count_it = number_of_min_readings_per_year_map.begin();
        graphing_index = 0;
        year_vector.clear();
        temperature_vector.clear();

        while ( min_deviation_it != total_min_deviation_per_year_map.end() && count_it != number_of_min_readings_per_year_map.end() )
        {
            float average = min_deviation_it->second / (float)count_it->second;;

            if ( isnan(average) )
            {
                average = 0;
            }

            std::cout << "Minimum," << min_deviation_it->first << "," << average << "," << (average * 1.8) << "," << count_it->second << ",,";
            //std::cerr << temperature_it->first << "," << average << "," << (average * 1.8) << "," << count_it->second << ",,";

            unsigned int year = temperature_it->first;

            //if (g_current_graphing_option == GRAPHING_OPTION_MINIMUM_TEMPERATURE_ANOMALY)
            {
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE_ANOMALY][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE_ANOMALY][graphing_index].second = average;

                if ( !isnan(average) && year >= g_first_year)
                {
                    year_vector.push_back( double(year) );
                    temperature_vector.push_back( double(average) );
                }
            }

            float annual_monthly_sum = 0.0f;
            size_t number_of_valid_months = 0;
            float average_summer_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_min_readings_per_month[year - FIRST_YEAR][5] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][6] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][7] )
            {
                average_summer_temperature =
                        ( total_min_temperature_per_month[year - FIRST_YEAR][5] +
                        total_min_temperature_per_month[year - FIRST_YEAR][6] +
                        total_min_temperature_per_month[year - FIRST_YEAR][7] ) /
                        ( number_of_min_readings_per_month[year - FIRST_YEAR][5] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][6] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][7] );
            }

            float average_winter_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( (year > FIRST_YEAR) &&
                 number_of_min_readings_per_month[year - FIRST_YEAR - 1][11] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][0] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][1] )
            {
                average_winter_temperature =
                        ( total_min_temperature_per_month[year - FIRST_YEAR - 1][11] +
                        total_min_temperature_per_month[year - FIRST_YEAR][0] +
                        total_min_temperature_per_month[year - FIRST_YEAR][1] ) /
                        ( number_of_min_readings_per_month[year - FIRST_YEAR - 1][11] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][0] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][1] );
            }

            float average_spring_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_min_readings_per_month[year - FIRST_YEAR][2] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][3] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][4] )
            {
                average_spring_temperature =
                        ( total_min_temperature_per_month[year - FIRST_YEAR][2] +
                        total_min_temperature_per_month[year - FIRST_YEAR][3] +
                        total_min_temperature_per_month[year - FIRST_YEAR][4] ) /
                        ( number_of_min_readings_per_month[year - FIRST_YEAR][2] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][3] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][4] );
            }

            float average_fall_temperature = UNREASONABLE_LOW_TEMPERATURE;

            if ( number_of_min_readings_per_month[year - FIRST_YEAR][8] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][9] &&
                 number_of_min_readings_per_month[year - FIRST_YEAR][10] )
            {
                average_fall_temperature =
                        ( total_min_temperature_per_month[year - FIRST_YEAR][8] +
                        total_min_temperature_per_month[year - FIRST_YEAR][9] +
                        total_min_temperature_per_month[year - FIRST_YEAR][10] ) /
                        ( number_of_min_readings_per_month[year - FIRST_YEAR][8] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][9] +
                        number_of_min_readings_per_month[year - FIRST_YEAR][10] );
            }

            for (size_t month = 0; month < NUMBER_OF_MONTHS_PER_YEAR; month++)
            {
                float monthly_average = UNKNOWN_TEMPERATURE;
                size_t month_number = (year * NUMBER_OF_MONTHS_PER_YEAR) + month;

                if ( number_of_min_readings_per_month[year - FIRST_YEAR][month] )
                {
                    if ( (month_number - previous_month_number) == 1 )
                    {
                        consecutive_count ++;
                    }
                    else
                    {
                        consecutive_count = 0;
                    }

                    monthly_average = total_min_temperature_per_month[year - FIRST_YEAR][month] / number_of_min_readings_per_month[year - FIRST_YEAR][month];
                    total_temperature += monthly_average;
                    minimum_month_running_total_vector.push_back(total_temperature);
                    annual_monthly_sum += monthly_average;
                    number_of_valid_months++;

                    month_year_vector[month].push_back(year);
                    month_temperature_vector[month].push_back(monthly_average);

                    if (consecutive_count >= number_of_months_under_test)
                    {
                        size_t size = minimum_month_running_total_vector.size();
                        float total_variable_month_temperature = minimum_month_running_total_vector.at(size - 1) - minimum_month_running_total_vector.at(size - 1 - number_of_months_under_test);
                        float average_temperature = total_variable_month_temperature / float(number_of_months_under_test);
                        country.getVariableMonthMeanMinimumMap()[average_temperature] = month_number;
                    }

                    previous_month_number = month_number;
                }

                if (monthly_average == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << monthly_average << ",";
                }
            }

            if (number_of_valid_months)
            {
                float annual_monthly_average = annual_monthly_sum / float(number_of_valid_months);
                float annual_monthly_average_f = (annual_monthly_average * 1.8f) + 32.0f;
                std::cout << annual_monthly_average << "," << annual_monthly_average_f;

                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE][graphing_index].second = annual_monthly_average;
                g_result_pair[GRAPHING_OPTION_WINTER_MINIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_SPRING_MINIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_SUMMER_MINIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_AUTUMN_MINIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_WINTER_MINIMUM_TEMPERATURE][graphing_index].second = average_winter_temperature;
                g_result_pair[GRAPHING_OPTION_SPRING_MINIMUM_TEMPERATURE][graphing_index].second = average_spring_temperature;
                g_result_pair[GRAPHING_OPTION_SUMMER_MINIMUM_TEMPERATURE][graphing_index].second = average_summer_temperature;
                g_result_pair[GRAPHING_OPTION_AUTUMN_MINIMUM_TEMPERATURE][graphing_index].second = average_fall_temperature;

                if (average_winter_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_winter_temperature;
                }

                if (average_spring_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_spring_temperature;
                }

                if (average_summer_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_summer_temperature;
                }

                if (average_fall_temperature == -99)
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << average_fall_temperature;
                }
            }
            else
            {
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE_ANOMALY][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_MINIMUM_TEMPERATURE_ANOMALY][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
            }

            std::cout << std::endl;

            min_deviation_it++;
            temperature_it++;
            count_it++;
            graphing_index++;
        }

        linearRegression(slope, x_offset, year_vector, temperature_vector);
        std::cout << "Minimum temperature trend," << std::setprecision(3) << slope * 100 << std::endl;
        std::cout << "Monthly minimum temperature trend,,,,,";

        for (size_t month_number = 0; month_number < NUMBER_OF_MONTHS_PER_YEAR; month_number++)
        {
            linearRegression(slope, x_offset, month_year_vector[month_number], month_temperature_vector[month_number]);
            std::cout << "," << std::setprecision(3) << slope * 100;
        }

        std::cout << std::endl;

#if 0
        std::cout << "Hottest Minimum" << number_of_months_under_test << " month periods " << std::endl;
        std::cout << "Rank," << "Month," << "Year," << "Temperature " << std::endl;
        reverse_variable_month_iterator = country.getVariableMonthMeanMinimumMap().rbegin();
        count = 1;

        for ( ; reverse_variable_month_iterator != country.getVariableMonthMeanMinimumMap().rend(), count < 20; reverse_variable_month_iterator++ )
        {
            unsigned int year = reverse_variable_month_iterator->second / NUMBER_OF_MONTHS_PER_YEAR;
            unsigned int month = reverse_variable_month_iterator->second % NUMBER_OF_MONTHS_PER_YEAR;
            float temperature = reverse_variable_month_iterator->first;

            std::cout << count++ << "," << month + 1 << "," << year << "," << temperature << std::endl;
        }

        std::cout << "Coldest Minimum" << number_of_months_under_test << " month periods " << std::endl;
        std::cout << "Rank," << "Month," << "Year," << "Temperature " << std::endl;
        forward_variable_month_iterator = country.getVariableMonthMeanMinimumMap().begin();
        count = 1;

        for ( ; forward_variable_month_iterator != country.getVariableMonthMeanMinimumMap().end(), count < 20; forward_variable_month_iterator++ )
        {
            unsigned int year = forward_variable_month_iterator->second / NUMBER_OF_MONTHS_PER_YEAR;+

            unsigned int month = forward_variable_month_iterator->second % NUMBER_OF_MONTHS_PER_YEAR;
            float temperature = forward_variable_month_iterator->first;

            std::cout << count++ << "," << month + 1 << "," << year << "," << temperature << std::endl;
        }
#endif
        std::cout << "First day above maximum threshold";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (first_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number] == float(UNKNOWN_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << first_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number];
                }
            }

            std::cout << std::endl;
        }

        std::cout << "Last day above maximum threshold";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (last_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number] == float(UNKNOWN_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << last_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number];
                }
            }

            std::cout << std::endl;
        }

        std::cout << "Number of days above maximum threshold";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (last_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number] == float(UNKNOWN_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," <<
                                 last_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number] -
                                 first_day_above_high_threshold_per_year_map_per_state_vector[state_number][year_number];
                }
            }

            std::cout << std::endl;
        }

        std::cout << "First day below minimum threshold";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (first_day_below_low_threshold_per_year_map_per_state_vector[state_number][year_number] == float(UNKNOWN_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << first_day_below_low_threshold_per_year_map_per_state_vector[state_number][year_number];
                }
            }

            std::cout << std::endl;
        }

        std::cout << "Last day below minimum threshold";

        for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
        {
            std::cout << "," << STATE_NAMES[state_number + 1];
            state_year_vector[state_number].clear();
            state_temperature_vector[state_number].clear();
        }

        std::cout << std::endl;

        for (size_t year_number = FIRST_YEAR; year_number <= most_recent_year; year_number++)
        {
            std::cout << year_number;

            for (size_t state_number = 0; state_number < NUMBER_OF_STATES; state_number++)
            {
                if (last_day_below_low_threshold_per_year_map_per_state_vector[state_number][year_number] == float(UNKNOWN_TEMPERATURE))
                {
                    std::cout << ",";
                }
                else
                {
                    std::cout << "," << last_day_below_low_threshold_per_year_map_per_state_vector[state_number][year_number];
                }
            }

            std::cout << std::endl;
        }


        std::cout << "Year";
        std::cout << ",Average hottest maximum,Average coldest minimum";
        std::cout << ",High threshold,Number of days high above,Percent high above";
        std::cout << ",Average number of days high above";
        std::cout << ",Number of days high above and above previous day,Percent high above and above previous day";
        std::cout << ",First day high above,Last day high above,Number of days between first and last high above";
        std::cout << ",Station with maximum consecutive days high above max threshold";
        std::cout << ",State with maximum consecutive days high above max threshold,ID";
        std::cout << ",Maximum consecutive days high above max threshold";
        std::cout << ",Station with maximum consecutive days high below max threshold";
        std::cout << ",State with maximum consecutive days high below max threshold,ID";
        std::cout << ",Maximum consecutive days high below max threshold";
        std::cout << ",Low threshold,Number of low days below,Percent low below";
        std::cout << ",Average number of days low below";
        std::cout << ",Percent of days low above or below";
        std::cout << ",First day low below,Last day low below,Number of days between first and last low below";
        std::cout << ",Number of days maximum below low threshold,Number of days minimum above high threshold";
        std::cout << ",Total maximum above and minimum below";
        std::cout << ",Percent of stations above or below";
        std::cout << ",Percent of stations above max threshold,Percent of stations below max threshold";
        std::cout << ",Percent of stations below min threshold,Percent of stations above min threshold";
        std::cout << ",Percent of stations above max threshold on " << g_month_to_dump << "/" << g_day_to_dump;
        std::cout << std::endl;
        graphing_index = 0;
        
        for (unsigned int i = 0; i <= (most_recent_year - FIRST_YEAR); i++)
        {
            size_t year = FIRST_YEAR + i;
            std::cout << year;
            std::cout << "," << cToF(total_hottest_maximum_temperature_per_year_map[year] / number_of_hottest_maximum_temperatures_per_year_map[year]);
            std::cout << "," << cToF(total_coldest_minimum_temperature_per_year_map[year] / number_of_coldest_minimum_temperatures_per_year_map[year]);
            std::cout << "," << g_temperature_threshold_high;
            std::cout << "," << number_of_maximum_temperatures_above_high_threshold[i];

            double percent_above = (double)number_of_maximum_temperatures_above_high_threshold[i] / (double)number_of_max_readings_per_year_map[(unsigned int)year] * 100.0;

            if ( isnan(percent_above) )
            {
                percent_above = 0;
            }

            double percent_below = (double)number_of_maximum_temperatures_below_high_threshold[i] / (double)number_of_max_readings_per_year_map[(unsigned int)year] * 100.0;

            if ( isnan(percent_below) )
            {
                percent_below = 0;
            }

            std::cout << "," << std::setprecision(5) << percent_above;
            size_t average_above = (size_t)( round( (double)getNumberOfDaysUnderTest((int)year) * percent_above / 100.0 ) );
            std::cout << "," << average_above;

            //if (g_current_graphing_option == GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD)
            {
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_CONSECUTIVE_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_FIRST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_LAST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_LENGTH_OF_MAXIMUM_TEMPERATURE_THRESHOLD_SEASON][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_CONSECUTIVE_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;

                if ( number_of_max_readings_per_year_map[(unsigned int)year] == 0 )
                {
                    g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_FIRST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_LAST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_LENGTH_OF_MAXIMUM_TEMPERATURE_THRESHOLD_SEASON][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                }
                else
                {
                    g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)number_of_maximum_temperatures_above_high_threshold[i];
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)percent_above;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second =
                            maximum_number_of_consecutive_days_above_maximum_threshold_per_year_map[(unsigned int)year];
                    g_result_pair[GRAPHING_OPTION_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)number_of_maximum_temperatures_below_high_threshold[i];
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)percent_below;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second =
                            maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[(unsigned int)year];
                }
            }

            std::cout << "," << number_of_maximum_temperatures_above_high_threshold_and_higher_than_previous_day[i];
            std::cout << "," << std::setprecision(5) << (double)number_of_maximum_temperatures_above_high_threshold_and_higher_than_previous_day[i] / (double)number_of_max_readings_per_year_map[(unsigned int)year] * 100.0;

            if ( number_of_maximum_temperatures_above_high_threshold[i] > 0 &&
                 first_day_above_high_threshold_per_year_map[(unsigned int)year] > NO_VALUE)
            {
                g_result_pair[GRAPHING_OPTION_FIRST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second =
                        first_day_above_high_threshold_per_year_map[(unsigned int)year];
                g_result_pair[GRAPHING_OPTION_LAST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second =
                        last_day_above_high_threshold_per_year_map[(unsigned int)year];
                g_result_pair[GRAPHING_OPTION_LENGTH_OF_MAXIMUM_TEMPERATURE_THRESHOLD_SEASON][graphing_index].second =
                        last_day_above_high_threshold_per_year_map[(unsigned int)year] - first_day_above_high_threshold_per_year_map[(unsigned int)year] + 1;

                std::cout << "," << first_day_above_high_threshold_per_year_map[(unsigned int)year];
                std::cout << "," << last_day_above_high_threshold_per_year_map[(unsigned int)year];
                std::cout << "," << last_day_above_high_threshold_per_year_map[(unsigned int)year] - first_day_above_high_threshold_per_year_map[(unsigned int)year] + 1 << ",";

                Station* station = maximum_number_of_consecutive_days_above_maximum_threshold_station_per_year_map[(unsigned int)year];

                if (station != NULL)
                {
                    std::cout << station->getStationName() << ",";
                    std::cout << station->getStateName() << ",";
                    std::cout << station->getStationNumber() << ",";
                    std::cout << maximum_number_of_consecutive_days_above_maximum_threshold_per_year_map[(unsigned int)year] << ",";
                }
                else
                {                  
                    std::cout << ",,,,";
                }
            }
            else
            {
                if ( number_of_max_readings_per_year_map[(unsigned int)year] > 0 )
                {
                    g_result_pair[GRAPHING_OPTION_FIRST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = NUMBER_OF_DAYS_PER_YEAR + 1;
                    g_result_pair[GRAPHING_OPTION_LAST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = 0;
                    g_result_pair[GRAPHING_OPTION_LENGTH_OF_MAXIMUM_TEMPERATURE_THRESHOLD_SEASON][graphing_index].second = 0;
                }

                std::cout << ",,,,,,,,";
            }

            if ( maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[(unsigned int)year] > 0 )
            {
                Station* station = maximum_number_of_consecutive_days_below_maximum_threshold_station_per_year_map[(unsigned int)year];

                if (station != NULL)
                {
                    std::cout << station->getStationName() << ",";
                    std::cout << station->getStateName() << ",";
                    std::cout << station->getStationNumber() << ",";
                    //					std::cerr << " Printing max days below for year " << year;
                    //					std::cerr << " value " << maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[year] << std::endl;
                    std::cout << maximum_number_of_consecutive_days_below_maximum_threshold_per_year_map[(unsigned int)year] << ",";
                }
                else
                {
                    std::cout << ",,,,";
                }
            }

            std::cout << g_temperature_threshold_low << "," << number_of_minimum_temperatures_below_low_threshold[i];

            percent_below = (double)number_of_minimum_temperatures_below_low_threshold[i] / (double)number_of_min_readings_per_year_map[(unsigned int)year] * 100.0;

            if ( isnan(percent_below) )
            {
                percent_below = 0;
            }

            percent_above = (double)number_of_minimum_temperatures_above_low_threshold[i] / (double)number_of_min_readings_per_year_map[(unsigned int)year] * 100.0;

            if ( isnan(percent_below) )
            {
                percent_below = 0;
            }

            std::cout << "," << std::setprecision(5) << percent_below;
            size_t average_below = (size_t)( round( (double)getNumberOfDaysUnderTest((int)year) * percent_below / 100.0 ) );
            std::cout << "," << average_below;

            double percent_above_or_below = percent_above + percent_below;
            std::cout << "," << percent_above_or_below;

            //if (g_current_graphing_option == GRAPHING_OPTION_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD)
            {
                g_result_pair[GRAPHING_OPTION_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_CONSECUTIVE_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_CONSECUTIVE_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_FIRST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_LAST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].first = year;
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_OR_BELOW_MINIMUM_THRESHOLD][graphing_index].first = year;


                if ( number_of_min_readings_per_year_map[(unsigned int)year] == 0 )
                {
                    g_result_pair[GRAPHING_OPTION_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_FIRST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_LAST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                    g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_OR_BELOW_MINIMUM_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                }
                else
                {
                    g_result_pair[GRAPHING_OPTION_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)number_of_minimum_temperatures_below_low_threshold[i];
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)percent_below;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = maximum_number_of_consecutive_days_below_minimum_threshold_per_year_map[(unsigned int)year];
                    g_result_pair[GRAPHING_OPTION_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)number_of_minimum_temperatures_above_low_threshold[i];
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = (float)percent_above;
                    g_result_pair[GRAPHING_OPTION_CONSECUTIVE_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second
                            = maximum_number_of_consecutive_days_above_minimum_threshold_per_year_map[(unsigned int)year];
                }

                if ( number_of_min_readings_per_year_map[(unsigned int)year] == 0 || number_of_max_readings_per_year_map[(unsigned int)year] == 0 )
                {
                    g_result_pair[GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                }
                else
                {
                    g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_OR_BELOW_MINIMUM_THRESHOLD][graphing_index].second =
                            g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD][graphing_index].second +
                            g_result_pair[GRAPHING_OPTION_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second;
                }
            }

            if ( number_of_minimum_temperatures_below_low_threshold[i] > 0 &&
                 first_day_below_low_threshold_per_year_map[(unsigned int)year] > NO_VALUE)
            {
                g_result_pair[GRAPHING_OPTION_FIRST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second =
                        first_day_below_low_threshold_per_year_map[(unsigned int)year];
                g_result_pair[GRAPHING_OPTION_LAST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second =
                        last_day_below_low_threshold_per_year_map[(unsigned int)year];

                std::cout << "," << first_day_below_low_threshold_per_year_map[(unsigned int)year];
                std::cout << "," << last_day_below_low_threshold_per_year_map[(unsigned int)year];
                std::cout << "," << last_day_below_low_threshold_per_year_map[(unsigned int)year] - first_day_below_low_threshold_per_year_map[(unsigned int)year];
            }
            else
            {
                if ( number_of_min_readings_per_year_map[(unsigned int)year] > 0 )
                {
                    g_result_pair[GRAPHING_OPTION_FIRST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = NUMBER_OF_DAYS_PER_YEAR + 1;
                    g_result_pair[GRAPHING_OPTION_LAST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD][graphing_index].second = 0;
                }

                std::cout << ",,,";
            }

            std::cout << "," << number_of_max_temperatures_below_low_threshold_map[(unsigned int)year];
            std::cout << "," << number_of_min_temperatures_above_high_threshold_map[(unsigned int)year];
            std::cout << "," << number_of_maximum_temperatures_above_high_threshold[i] + number_of_minimum_temperatures_below_low_threshold[i];

            int number_of_stations = number_of_stations_above_the_maximum_threshold_per_year_map[(unsigned int)year] +
                    number_of_stations_below_the_maximum_threshold_per_year_map[(unsigned int)year];
            float percent_above_maximum_threshold =
                    (float)number_of_stations_above_the_maximum_threshold_per_year_map[(unsigned int)year] / float(number_of_stations) * 100.0f;
            float percent_below_maximum_threshold =
                    (float)number_of_stations_below_the_maximum_threshold_per_year_map[(unsigned int)year] / float(number_of_stations) * 100.0f;
            float percent_below_minimum_threshold =
                    (float)number_of_stations_below_the_minimum_threshold_per_year_map[(unsigned int)year] / float(number_of_stations) * 100.0f;
            float percent_above_minimum_threshold =
                    (float)number_of_stations_above_the_minimum_threshold_per_year_map[(unsigned int)year] / float(number_of_stations) * 100.0f;
            std::cout << "," << percent_above_maximum_threshold + percent_below_minimum_threshold;
            std::cout << "," << percent_above_maximum_threshold;
            std::cout << "," << percent_below_maximum_threshold;
            std::cout << "," << percent_below_minimum_threshold;
            std::cout << "," << percent_above_minimum_threshold;

            if (g_month_to_dump != 0 && g_day_to_dump != 0 &&
                    number_of_stations_below_the_maximum_threshold_on_the_day_under_test_per_year_map[year] > 0)
            {
                int number_of_stations =
                        number_of_stations_above_the_maximum_threshold_on_the_day_under_test_per_year_map[year] +
                        number_of_stations_below_the_maximum_threshold_on_the_day_under_test_per_year_map[year];

                float percent_of_stations_above_the_maximum_threshold_on_the_day_under_test =
                        (float)number_of_stations_above_the_maximum_threshold_on_the_day_under_test_per_year_map[year] /
                        (float)number_of_stations * 100.0f;

                std::cout << "," << percent_of_stations_above_the_maximum_threshold_on_the_day_under_test;
            }
            else
            {
                std::cout << "," << "0";
            }

            std::cout << std::endl;

            graphing_index++;
        }

        std::cout << "Year,Average annual snowfall in cm,Maximum snowfall,Maximum snowfall station,Maximum snowfall day,";
        std::cout << "Number of snow days,Maximum number of days without snow,Last spring snow day,First autumn snow day";
        std::cout << ",Percent of stations with snow,Percent of stations without snow";
        std::cout << "Snow target,Number of days reached snow target,Percentage of events reached snow target,";
        std::cout << std::endl;
        std::map<unsigned int,float>::iterator snow_it = total_snowfall_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator snow_count_it = number_of_snowfalls_per_year_map.begin();
        std::map<unsigned int,Day>::iterator snow_maximum_it = maximum_snowfall_per_year_day_map.begin();
        std::map<unsigned int,unsigned int>::iterator snow_free_days_it = maximum_number_of_days_without_snow_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator last_spring_snow_it = last_spring_day_of_snow_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator first_autumn_snow_it = first_autumn_day_of_snow_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator days_with_snow_it = number_of_stations_with_snow_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator days_without_snow_it = number_of_stations_without_snow_per_year_map.begin();
        std::map<unsigned int,int>::iterator snow_days_above_count_it = number_of_days_above_snow_target_map.begin();

        graphing_index = 0;

        while ( snow_it != total_snowfall_per_year_map.end() )
        {
            if (used_station_count == 0)
            {
                break;
            }

            size_t year = snow_it->first;
            std::cout << year;
            std::cout << "," << (snow_it->second / used_station_count);
            std::cout << "," << snow_maximum_it->second.getSnowfall();
            std::cout << "," << snow_maximum_it->second.getStationName();
            std::cout << "," << snow_maximum_it->second.getDayOfYear();
            std::cout << "," << (snow_count_it->second / used_station_count);
            std::cout << "," << snow_free_days_it->second;
            std::cout << "," << last_spring_snow_it->second << "," << first_autumn_snow_it->second;

            int number_of_stations = std::max(number_of_stations_with_snow_per_year_map[(unsigned int)year],
                    number_of_stations_without_snow_per_year_map[(unsigned int)year]);
            std::cout << "," << (float)number_of_stations_with_snow_per_year_map[(unsigned int)year] / float(number_of_stations) * 100.0f;
            std::cout << "," << (float)number_of_stations_without_snow_per_year_map[(unsigned int)year] / float(number_of_stations) * 100.0f;
            std::cout << "," << g_snow_target << "," << snow_days_above_count_it->second;
            std::cout << "," << (float)snow_days_above_count_it->second / (float)snow_count_it->second * 100.0f;
            std::cout << std::endl;

            bool year_valid =    number_of_stations_with_snow_per_year_map[(unsigned int)year] > 0
                              || number_of_stations_without_snow_per_year_map[(unsigned int)year] > 0;


            g_result_pair[GRAPHING_OPTION_SNOWFALL][graphing_index].first = snow_it->first;
            g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_SNOW_THRESHOLD][graphing_index].first = snow_days_above_count_it->first;
            g_result_pair[GRAPHING_OPTION_LAST_SPRING_SNOW][graphing_index].first = last_spring_snow_it->first;
            g_result_pair[GRAPHING_OPTION_FIRST_AUTUMN_SNOW][graphing_index].first = first_autumn_snow_it->first;
            g_result_pair[GRAPHING_OPTION_LENGTH_OF_SNOW_FREE_SEASON][graphing_index].first = snow_free_days_it->first;

            if (!year_valid)
            {
                g_result_pair[GRAPHING_OPTION_SNOWFALL][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_SNOW_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_LAST_SPRING_SNOW][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_FIRST_AUTUMN_SNOW][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_LENGTH_OF_SNOW_FREE_SEASON][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
            }
            else
            {
                g_result_pair[GRAPHING_OPTION_SNOWFALL][graphing_index].second = snow_it->second / used_station_count;
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_SNOW_THRESHOLD][graphing_index].second = (float)snow_days_above_count_it->second;
                g_result_pair[GRAPHING_OPTION_LAST_SPRING_SNOW][graphing_index].second = last_spring_snow_it->second;
                g_result_pair[GRAPHING_OPTION_FIRST_AUTUMN_SNOW][graphing_index].second = first_autumn_snow_it->second;
                g_result_pair[GRAPHING_OPTION_LENGTH_OF_SNOW_FREE_SEASON][graphing_index].second = snow_free_days_it->second;
            }


            snow_it++;
            snow_maximum_it++;
            snow_count_it++;
            snow_free_days_it++;
            last_spring_snow_it++;
            first_autumn_snow_it++;
            days_with_snow_it++;
            days_without_snow_it++;
            snow_days_above_count_it++;
            graphing_index++;
        }

        std::cout << "Year,Average annual precipitation in cm,Number of days with precipitation,";
        std::cout << "Maximum precipitation,Maximum precipitation station,Maximum precipitation day,";
        std::cout << "Precipitation target,Number of days reached precipitation target,Percentage of events reached precipitation target,";
        std::cout << "Percentage of days reached precipitation target ";
        std::cout << g_precipitation_target << " cm " << std::endl;
        std::map<unsigned int,float>::iterator precipitation_it = total_precipitation_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator precipitation_count_it = number_of_precipitation_days_per_year_map.begin();
        std::map<unsigned int,unsigned int>::iterator precipitation_record_count_it = number_of_precipitation_records_per_year_map.begin();
        std::map<unsigned int,Day>::iterator precipitation_maximum_it = maximum_precipitation_per_year_day_map.begin();
        std::map<unsigned int,int>::iterator precipitation_days_above_count_it = number_of_days_above_precipitation_target_map.begin();
        std::map<unsigned int,unsigned int>::iterator precip_readings_per_year_it = number_of_precip_readings_per_year_map.begin();

        graphing_index = 0;

        while ( precipitation_it != total_precipitation_per_year_map.end() )
        {
            if (used_station_count == 0)
            {
                break;
            }

            int year = precipitation_it->first;

            std::cout << precipitation_it->first;
            std::cout << "," << (precipitation_it->second / used_station_count);
            std::cout << "," << (precipitation_count_it->second / used_station_count);
            std::cout << "," << precipitation_maximum_it->second.getPrecipitation();
            std::cout << "," << precipitation_maximum_it->second.getStationName();
            std::cout << "," << precipitation_maximum_it->second.getDayOfYear();
            std::cout << "," << g_precipitation_target << "," << precipitation_days_above_count_it->second;
            std::cout << "," << (float)precipitation_days_above_count_it->second / (float)precipitation_count_it->second * 100.0f;
            std::cout << "," << (float)precipitation_days_above_count_it->second / (float)precip_readings_per_year_it->second * 100.0f;
            std::cout << std::endl;

            bool year_valid = number_of_precipitation_days_per_year_map[year] > 0;

            //if (g_current_graphing_option == GRAPHING_OPTION_DAYS_ABOVE_PRECIPITATION_THRESHOLD)
            {
                g_result_pair[GRAPHING_OPTION_PRECIPITATION][graphing_index].first = precipitation_it->first;
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_PRECIPITATION_THRESHOLD][graphing_index].first = precipitation_days_above_count_it->first;
            }

            if (!year_valid)
            {
                g_result_pair[GRAPHING_OPTION_PRECIPITATION][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_PRECIPITATION_THRESHOLD][graphing_index].second = UNREASONABLE_LOW_TEMPERATURE;
            }
            else
            {
                g_result_pair[GRAPHING_OPTION_PRECIPITATION][graphing_index].second = precipitation_it->second / used_station_count;
                g_result_pair[GRAPHING_OPTION_DAYS_ABOVE_PRECIPITATION_THRESHOLD][graphing_index].second = (float)precipitation_days_above_count_it->second;
            }

            precipitation_it++;
            precipitation_count_it++;
            precipitation_maximum_it++;
            precipitation_record_count_it++;
            precipitation_days_above_count_it++;
            precip_readings_per_year_it++;
            graphing_index++;
        }

#if 0
        // Not necessary. Already being done by someone.
        // Clean up
        for (size_t state_number = 0; state_number < state_vector_size; state_number++)
        {
            std::vector<Station*>& station_vector = state_vector.at(state_number).getStationVector();
            size_t station_vector_size = station_vector.size();

            std::cout << " Deleting " << station_vector_size << " stations state number = " << state_number << std::endl;

            for (size_t station_number = 0; station_number < station_vector_size; station_number++)
            {
                Station* station =  station_vector[station_number];

                if (station != NULL)
                {
                    std::cout << " Deleting " << station->getStationName() << ", " << station->getStateName() << std::endl;
                    delete station;
                }
            }
        }

        std::cout << "Clean up done" << std::endl;
#endif

    }
    else
    {
        std::cout << "Unable to open us.txt" << std::endl;
    }



    return 0;
}



