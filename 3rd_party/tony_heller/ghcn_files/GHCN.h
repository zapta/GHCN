//--------------------------------------------------------------------------------------
// GHCN.h
// Code for parsing and processing daily GHCN data from
// http://cdiac.ornl.gov/ftp/ushcn_daily
// Written by Steve Goddard
// If you modify it and mess it up, don't blame it on me

#ifndef GHCN_H_INCLUDED
#define GHCN_H_INCLUDED


#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <cstring>
#include <math.h>

// Comment out the next two lines to compile on MS compilers
#include <stdlib.h>
#include <limits.h>

int ghcnmain(std::vector<std::string> argv);

static const unsigned int   MAX_DAYS_IN_MONTH = 31;
static const unsigned int   NUMBER_OF_MONTHS_PER_YEAR = 12;
static const unsigned int   NUMBER_OF_DAYS_PER_MONTH[NUMBER_OF_MONTHS_PER_YEAR] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const unsigned int   FIRST_YEAR = 1850;
static const unsigned int   USHCN_FIRST_YEAR = 1895;
static const unsigned int   NUMBER_OF_YEARS = 300;
static const unsigned int   MAX_NUMBER_OF_DATA_POINTS = MAX_DAYS_IN_MONTH * NUMBER_OF_MONTHS_PER_YEAR;
static const unsigned int   MAX_YEARS = 3000;
static const unsigned int   MAX_STATE_NAME_LENGTH = 64;
static const unsigned int   NUMBER_OF_STATES = 48;
static const float          UNKNOWN_TEMPERATURE = -99.0f;
static const float          UNREASONABLE_HIGH_TEMPERATURE = 55.0f;
static const float          UNREASONABLE_LOW_TEMPERATURE = -60.0f;
static const unsigned int   NUMBER_OF_DAYS_PER_YEAR = 366;
static const int            NUMBER_OF_MONTHS_UNDER_TEST = 12;
static const int			NO_VALUE = -1;
static const float          NUMBER_OF_F_DEGREES_PER_C_DEGREE = 1.8f;
static const float          NUMBER_OF_CM_PER_INCH = 2.54f;
static const float          NUMBER_OF_DEGREES_F_PER_DEGREE_C = 1.8f;


static const size_t GRAPHING_OPTIONS_STRING_LENGTH = 100;
static const char g_graphing_options_string_array[][GRAPHING_OPTIONS_STRING_LENGTH] =
{
     "Average Mean Temperature"
    ,"Average Maximum Temperature"
    ,"Average Minimum Temperature"
    ,"Average Mean Winter Temperature"
    ,"Average Maximum Winter Temperature"
    ,"Average Minimum Winter Temperature"
    ,"Average Mean Spring Temperature"
    ,"Average Maximum Spring Temperature"
    ,"Average Minimum Spring Temperature"
    ,"Average Mean Summer Temperature"
    ,"Average Maximum Summer Temperature"
    ,"Average Minimum Summer Temperature"
    ,"Average Mean Autumn Temperature"
    ,"Average Maximum Autumn Temperature"
    ,"Average Minimum Autumn Temperature"
    ,"Average Mean Temperature Anomaly"
    ,"Average Maximum Temperature Anomaly"
    ,"Average Minimum Temperature Anomaly"
    ,"Total Precipitation"
    ,"Total Snowfall"
    ,"Date Of Last Spring Snow"
    ,"Date Of First Autumn Snow"
    ,"Length Of Snow Free Season"
    ,"Hottest Daily Maximum Temperature"
    ,"Coldest Daily Minimum Temperature"
    ,"Hottest Max Minus Coldest Min Temperature"
    ,"Hottest Daily Minimum Temperature"
    ,"Coldest Daily Maximum Temperature"
    ,"Maximum Temperature On A Day Of Year"
    ,"Minimum Temperature On A Day Of Year"
    ,"Precipitation On A Day Of Year"
    ,"Snowfall On A Day Of Year"
    ,"# Days Above Maximum Temperature Threshold"
    ,"% Days Above Maximum Temperature Threshold"
    ,"# Consecutive Days Above Max Temp Threshold"
    ,"First Day Above Maximum Temperature Threshold"
    ,"Last Day Above Maximum Temperature Threshold"
    ,"Length Of Maximum Temperature Threshold Season"
    ,"# Days Below Maximum Temperature Threshold"
    ,"% Days Below Maximum Temperature Threshold"
    ,"# Consecutive Days Below Max Temp Threshold"
    ,"# Nights Below Minimum Temperature Threshold"
    ,"% Nights Below Minimum Temperature Threshold"
    ,"# Consecutive Nights Below Min Temp Threshold"
    ,"First Night Below Minimum Temperature Threshold"
    ,"Last Night Below Minimum Temperature Threshold"
    ,"# Nights Above Minimum Temperature Threshold"
    ,"% Nights Above Minimum Temperature Threshold"
    ,"# Consecutive Nights Above Min Temp Threshold"
    ,"# Days Above Maximum Or Below Minimum Threshold"
    ,"# Days Above Precipitation Threshold"
    ,"# Days Above Snow Threshold"
    ,"# Daily Maximum Temperature Records"
    ,"# Daily Minimum Temperature Records"
    ,"Daily Maximum Temperature"
    ,"Daily Minimum Temperature"
    ,"Daily Min/Max Temperatures"
    ,"Daily Precipitation"
    ,"Daily Snowfall"
};


// These have to be in exactly the same order as the string array above
enum GRAPHING_OPTIONS
{
     GRAPHING_OPTION_MEAN_TEMPERATURE
    ,GRAPHING_OPTION_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_WINTER_MEAN_TEMPERATURE
    ,GRAPHING_OPTION_WINTER_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_WINTER_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_SPRING_MEAN_TEMPERATURE
    ,GRAPHING_OPTION_SPRING_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_SPRING_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_SUMMER_MEAN_TEMPERATURE
    ,GRAPHING_OPTION_SUMMER_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_SUMMER_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_AUTUMN_MEAN_TEMPERATURE
    ,GRAPHING_OPTION_AUTUMN_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_AUTUMN_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_MEAN_TEMPERATURE_ANOMALY
    ,GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ANOMALY
    ,GRAPHING_OPTION_MINIMUM_TEMPERATURE_ANOMALY
    ,GRAPHING_OPTION_PRECIPITATION
    ,GRAPHING_OPTION_SNOWFALL
    ,GRAPHING_OPTION_LAST_SPRING_SNOW
    ,GRAPHING_OPTION_FIRST_AUTUMN_SNOW
    ,GRAPHING_OPTION_LENGTH_OF_SNOW_FREE_SEASON
    ,GRAPHING_OPTION_HOTTEST_TEMPERATURE
    ,GRAPHING_OPTION_COLDEST_TEMPERATURE
    ,GRAPHING_OPTION_HOTTEST_MAXIMUM_MINUS_COLDEST_MINIMUM
    ,GRAPHING_OPTION_HOTTEST_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_COLDEST_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_MAXIMUM_TEMPERATURE_ON_A_DAY_OF_THE_YEAR
    ,GRAPHING_OPTION_MINIMUM_TEMPERATURE_ON_A_DAY_OF_THE_YEAR
    ,GRAPHING_OPTION_PRECIPITATION_ON_A_DAY_OF_THE_YEAR
    ,GRAPHING_OPTION_SNOWFALL_ON_A_DAY_OF_THE_YEAR
    ,GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_PERCENTAGE_OF_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_CONSECUTIVE_DAYS_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_FIRST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_LAST_DAY_ABOVE_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_LENGTH_OF_MAXIMUM_TEMPERATURE_THRESHOLD_SEASON
    ,GRAPHING_OPTION_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_PERCENTAGE_OF_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_CONSECUTIVE_DAYS_BELOW_MAXIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_CONSECUTIVE_NIGHTS_BELOW_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_FIRST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_LAST_NIGHT_BELOW_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_PERCENTAGE_OF_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_CONSECUTIVE_NIGHTS_ABOVE_MINIMUM_TEMPERATURE_THRESHOLD
    ,GRAPHING_OPTION_DAYS_ABOVE_MAXIMUM_OR_BELOW_MINIMUM_THRESHOLD
    ,GRAPHING_OPTION_DAYS_ABOVE_PRECIPITATION_THRESHOLD
    ,GRAPHING_OPTION_DAYS_ABOVE_SNOW_THRESHOLD
    ,GRAPHING_OPTION_NUMBER_OF_DAILY_MAXIMUM_TEMPERATURE_RECORDS
    ,GRAPHING_OPTION_NUMBER_OF_DAILY_MINIMUM_TEMPERATURE_RECORDS
    ,GRAPHING_OPTION_DAILY_MAXIMUM_TEMPERATURE
    ,GRAPHING_OPTION_DAILY_MINIMUM_TEMPERATURE
    ,GRAPHING_OPTION_DAILY_MINIMUM_MAXIMUM_TEMPERATURES
    ,GRAPHING_OPTION_DAILY_PRECIPITATION
    ,GRAPHING_OPTION_DAILY_SNOWFALL
    ,NUMBER_OF_GRAPHING_OPTIONS
};

extern GRAPHING_OPTIONS g_current_graphing_option;
extern std::pair<size_t,float> g_result_pair[NUMBER_OF_GRAPHING_OPTIONS][MAX_NUMBER_OF_DATA_POINTS];
extern std::pair<size_t,float> g_result_pair_2[NUMBER_OF_GRAPHING_OPTIONS][MAX_NUMBER_OF_DATA_POINTS];
extern float g_temperature_threshold_high;
extern float g_temperature_threshold_low;
extern int g_first_year;
extern int g_last_year;
extern size_t g_month_under_test;
extern size_t g_months_under_test;
extern float g_precipitation_target;
extern float g_snow_target;
extern size_t g_month_to_dump;
extern size_t g_day_to_dump;


size_t getDayOfYear(size_t day, size_t month, size_t year);
std::string dateString(int year, int day_of_year);
bool lastDayOfYear(size_t day_of_year, size_t year);
float cToF(float degrees_c);
float fToC(float degrees_f);
float cmToIn(float cm);
float InToCm(float in);
size_t getNumberOfDaysInAMonth(size_t month_number);
void linearRegression(double& a, double& b, std::vector<double>& x, std::vector<double>& y);
void replaceString(std::string& original_string, std::string pattern_to_replace, std::string new_pattern);

class Station;


enum MONTHS
{
    JANUARY,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER
};

static const char MONTH_NAMES[][4] =
{
     "Jan"
    ,"Feb"
    ,"Mar"
    ,"Apr"
    ,"May"
    ,"Jun"
    ,"Jul"
    ,"Aug"
    ,"Sep"
    ,"Oct"
    ,"Nov"
    ,"Dec"
};

static const char STATE_NAMES[][MAX_STATE_NAME_LENGTH] =
{
    "None",
    "Alaska",
    "Alabama",
    "Arizona",
    "Arkansas",
    "California",
    "Colorado",
    "Connecticut",
    "Delaware",
    "Florida",
    "Georgia",
    "Idaho",
    "Illinois",
    "Indiana",
    "Iowa",
    "Kansas",
    "Kentucky",
    "Louisiana",
    "Maine",
    "Maryland",
    "Massachusetts",
    "Michigan",
    "Minnesota",
    "Mississippi",
    "Missouri",
    "Montana",
    "Nebraska",
    "Nevada",
    "New Hampshire",
    "New Jersey",
    "New Mexico",
    "New York",
    "North Carolina",
    "North Dakota",
    "Ohio",
    "Oklahoma",
    "Oregon",
    "Pennsylvania",
    "Rhode Island",
    "South Carolina",
    "South Dakota",
    "Tennessee",
    "Texas",
    "Utah",
    "Vermont",
    "Virginia",
    "Washington",
    "West Virginia",
    "Wisconsin",
    "Wyoming",
    "Australia"
};

static const char STATE_ABREVIATIONS[][3] =
{
    "AL",
    "AZ",
    "AR",
    "CA",
    "CO",
    "CT",
    "DE",
    "FL",
    "GA",
    "ID",
    "IL",
    "IN",
    "IA",
    "KS",
    "KY",
    "LA",
    "ME",
    "MD",
    "MA",
    "MI",
    "MN",
    "MS",
    "MO",
    "MT",
    "NE",
    "NV",
    "NH",
    "NJ",
    "NM",
    "NY",
    "NC",
    "ND",
    "OH",
    "OK",
    "OR",
    "PA",
    "RI",
    "SC",
    "SD",
    "TN",
    "TX",
    "UT",
    "VT",
    "VA",
    "WA",
    "WV",
    "WI",
    "WY",
};

// From ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/ghcnd-countries.txt

static const char COUNTRY_NAMES[][2][MAX_STATE_NAME_LENGTH] =
{
    { "AC", "Antigua and Barbuda " },
    { "AE", "United Arab Emirates " },
    { "AF", "Afghanistan" },
    { "AG", "Algeria " },
    { "AJ", "Azerbaijan " },
    { "AL", "Albania" },
    { "AM", "Armenia " },
    { "AO", "Angola " },
    { "AQ", "American Samoa [United States] " },
    { "AR", "Argentina " },
    { "AS", "Australia " },
    { "AU", "Austria " },
    { "AY", "Antarctica " },
    { "BA", "Bahrain " },
    { "BB", "Barbados " },
    { "BC", "Botswana " },
    { "BD", "Bermuda [United Kingdom] " },
    { "BE", "Belgium " },
    { "BF", "Bahamas, The " },
    { "BK", "Bosnia and Herzegovina " },
    { "BL", "Bolivia " },
    { "BN", "Benin " },
    { "BO", "Belarus " },
    { "BP", "Solomon Islands " },
    { "BR", "Brazil " },
    { "BY", "Burundi" },
    { "CA", "Canada " },
    { "CD", "Chad " },
    { "CE", "Sri Lanka " },
    { "CF", "Congo (Brazzaville) " },
    { "CH", "China  " },
    { "CI", "Chile " },
    { "CJ", "Cayman Islands [United Kingdom] " },
    { "CK", "Cocos (Keeling) Islands [Australia] " },
    { "CM", "Cameroon " },
    { "CO", "Colombia " },
    { "CQ", "Northern Mariana Islands [United States] " },
    { "CS", "Costa Rica " },
    { "CT", "Central African Republic " },
    { "CU", "Cuba " },
    { "CY", "Cyprus " },
    { "DA", "Denmark " },
    { "DR", "Dominican Republic " },
    { "EC", "Ecuador " },
    { "EG", "Egypt " },
    { "EI", "Ireland " },
    { "EN", "Estonia " },
    { "ER", "Eritrea " },
    { "ES", "El Salvador " },
    { "ET", "Ethiopia " },
    { "EZ", "Czech Republic " },
    { "FG", "French Guiana [France] " },
    { "FI", "Finland " },
    { "FJ", "Fiji " },
    { "FM", "Federated States of Micronesia " },
    { "FP", "French Polynesia " },
    { "FR", "France " },
    { "FS", "French Southern and Antarctic Lands [France] " },
    { "GB", "Gabon " },
    { "GG", "Georgia " },
    { "GL", "Greenland [Denmark] " },
    { "GM", "Germany " },
    { "GP", "Guadeloupe [France] " },
    { "GQ", "Guam [United States] " },
    { "GR", "Greece " },
    { "GT", "Guatemala " },
    { "GV", "Guinea " },
    { "GY", "Guyana " },
    { "HO", "Honduras " },
    { "HR", "Croatia" },
    { "HU", "Hungary " },
    { "IC", "Iceland " },
    { "ID", "Indonesia " },
    { "IN", "India " },
    { "IO", "British Indian Ocean Territory [United Kingdom] " },
    { "IR", "Iran " },
    { "IS", "Israel " },
    { "IT", "Italy " },
    { "IV", "Cote D'Ivoire " },
    { "IZ", "Iraq " },
    { "JA", "Japan " },
    { "JM", "Jamaica " },
    { "JN", "Jan Mayen [Norway] " },
    { "JQ", "Johnston Atoll [United States] " },
    { "KE", "Kenya " },
    { "KG", "Kyrgyzstan " },
    { "KN", "Korea, North " },
    { "KR", "Kiribati " },
    { "KS", "Korea, South " },
    { "KT", "Christmas Island [Australia] " },
    { "KU", "Kuwait " },
    { "KZ", "Kazakhstan " },
    { "LA", "Laos " },
    { "LG", "Latvia " },
    { "LH", "Lithuania " },
    { "LO", "Slovakia " },
    { "LQ", "Palmyra Atoll [United States] " },
    { "LT", "Lesotho " },
    { "LU", "Luxembourg " },
    { "LY", "Libya " },
    { "MA", "Madagascar " },
    { "MD", "Moldova " },
    { "MG", "Mongolia " },
    { "MI", "Malawi " },
    { "MK", "Macedonia " },
    { "ML", "Mali " },
    { "MO", "Morocco " },
    { "MP", "Mauritius " },
    { "MQ", "Midway Islands [United States} " },
    { "MR", "Mauritania " },
    { "MQ", "Midway Islands [United States} " },
    { "MR", "Mauritania " },
    { "MT", "Malta " },
    { "MU", "Oman " },
    { "MV", "Maldives " },
    { "MX", "Mexico " },
    { "MY", "Malaysia " },
    { "MZ", "Mozambique " },
    { "NC", "New Caledonia [France] " },
    { "NG", "Niger " },
    { "NH", "Vanuatu " },
    { "NL", "Netherlands " },
    { "NO", "Norway " },
    { "NP", "Nepal " },
    { "NU", "Nicaragua " },
    { "NZ", "New Zealand " },
    { "PA", "Paraguay " },
    { "PC", "Pitcairn Islands [United Kingdom] " },
    { "PE", "Peru " },
    { "PK", "Pakistan " },
    { "PL", "Poland " },
    { "PM", "Panama " },
    { "PO", "Portugal " },
    { "PP", "Papua New Guinea " },
    { "PS", "Palau " },
    { "RI", "Serbia" },
    { "RM", "Marshall Islands " },
    { "RO", "Romania " },
    { "RP", "Philippines " },
    { "RQ", "Puerto Rico [United States] " },
    { "RS", "Russia " },
    { "SA", "Saudi Arabia " },
    { "SE", "Seychelles " },
    { "SF", "South Africa " },
    { "SG", "Senegal " },
    { "SH", "Saint Helena [United Kingdom] " },
    { "SI", "Slovenia " },
    { "SL", "Sierra Leone " },
    { "SP", "Spain " },
    { "ST", "Saint Lucia " },
    { "SU", "Sudan " },
    { "SV", "Svalbard [Norway] " },
    { "SW", "Sweden " },
    { "SY", "Syria " },
    { "SZ", "Switzerland " },
    { "TD", "Trinidad and Tobago " },
    { "TH", "Thailand " },
    { "TI", "Tajikistan " },
    { "TL", "Tokelau [New Zealand] " },
    { "TN", "Tonga " },
    { "TO", "Togo " },
    { "TS", "Tunisia " },
    { "TU", "Turkey " },
    { "TV", "Tuvalu " },
    { "TX", "Turkmenistan " },
    { "TZ", "Tanzania " },
    { "UG", "Uganda" },
    { "UK", "United Kingdom " },
    { "UP", "Ukraine " },
    { "US", "United States " },
    { "UV", "Burkina Faso " },
    { "UY", "Uruguay " },
    { "UZ", "Uzbekistan " },
    { "VE", "Venezuela " },
    { "VM", "Vietnam " },
    { "VQ", "Virgin Islands [United States] " },
    { "WA", "Namibia " },
    { "WF", "Wallis and Futuna [France] " },
    { "WQ", "Wake Island [United States]" },
    { "WZ", "Swaziland " },
    { "ZA", "Zambia " },
    { "ZI", "Zimbabwe " },
};

class DataRecord 
{
public:
    enum RECORD_TYPE
    {
        RECORD_TYPE_TMAX,
        RECORD_TYPE_TMIN,
        RECORD_TYPE_SNOW,
        RECORD_TYPE_SNWD,
        RECORD_TYPE_PRCP,
        RECORD_TYPE_NONE
    };



                            DataRecord() : 
                                            m_daily_high_temperature_vector(MAX_DAYS_IN_MONTH),
                                            m_daily_low_temperature_vector(MAX_DAYS_IN_MONTH),
                                            m_daily_snow_vector(MAX_DAYS_IN_MONTH),
                                            m_daily_precipitation_vector(MAX_DAYS_IN_MONTH)
                            {
                                for (unsigned int i = 0; i < MAX_DAYS_IN_MONTH; i++)
                                {
                                    setHighTemperature(i, UNKNOWN_TEMPERATURE);
                                    setLowTemperature(i, UNKNOWN_TEMPERATURE);
                                }

                                setContainsTemperatureData(false);
                            }

    std::string&            getCountryAbbreviation() { return m_country_abbreviation; }
    void                    setCountryAbbreviation(std::string value) { m_country_abbreviation = value; }
    std::string&            getStationNumber() { return m_station_number; }
    void                    setStationNumber(std::string value) { m_station_number = value; }
    std::string&            getRecordTypeString() { return m_record_type_string; }
    void                    setRecordTypeString(std::string type) { m_record_type_string = type; }
    unsigned int            getStateNumber() { return m_state_number; }
    void                    setStateNumber(unsigned int value) { m_state_number = value; }
    std::string             getStateName() { return std::string( STATE_NAMES[ getStateNumber() ] ); }
    unsigned int            getYear() { return m_year; }
    void                    setYear(unsigned int value) { m_year = value; }
    unsigned int            getMonth() { return m_month; }
    void                    setMonth(unsigned int value) { m_month = value; }
    std::vector<float>&     getDailyHighTemperatureVector() { return m_daily_high_temperature_vector; }
    float                   getHighTemperature(unsigned int day_of_month) { return getDailyHighTemperatureVector().at(day_of_month); }
    void                    setHighTemperature(unsigned int day_of_month, float value);
    std::vector<float>&     getDailyLowTemperatureVector() { return m_daily_high_temperature_vector; }
    float                   getLowTemperature(unsigned int day_of_month) { return getDailyLowTemperatureVector().at(day_of_month); }
    void                    setLowTemperature(unsigned int day_of_month, float value);
    std::vector<float>&     getDailySnowVector() { return m_daily_snow_vector; }
    float                   getSnowfall(unsigned int day_of_month) { return getDailySnowVector().at(day_of_month); }
    void                    setSnowfall(unsigned int day_of_month, float value);
    std::vector<float>&     getDailyPrecipitationVector() { return m_daily_snow_vector; }
    float                   getPrecipitation(unsigned int day_of_month) { return getDailyPrecipitationVector().at(day_of_month); }
    void                    setPrecipitation(unsigned int day_of_month, float value);
    size_t                  getDayOfYearForStartOfRecord();
    bool					getContainsTemperatureData() { return m_contains_temperature_data; }
    void					setContainsTemperatureData(bool flag) { m_contains_temperature_data = flag; }


    void                    parseTemperatureRecord(Station* station, std::string record_string);


protected:
    std::string             m_country_abbreviation;
    std::string             m_station_number;
    std::string             m_record_type_string;
    unsigned int            m_state_number;
    unsigned int            m_year;
    unsigned int            m_month;
    std::vector<float>      m_daily_high_temperature_vector;
    std::vector<float>      m_daily_low_temperature_vector;
    std::vector<float>      m_daily_snow_vector;
    std::vector<float>      m_daily_precipitation_vector;
    bool					m_contains_temperature_data;
};

class Day
{
public:
                            Day()
                            {
                                setMaxTemperature( UNKNOWN_TEMPERATURE );
                                setMinTemperature( UNKNOWN_TEMPERATURE );
                                setSnowfall(UNKNOWN_TEMPERATURE);
                                setPrecipitation(UNKNOWN_TEMPERATURE);
                            }

    float                   getMaxTemperature() { return m_max_temperature; }
    void                    setMaxTemperature(float value) { m_max_temperature = value; }
    float                   getMinTemperature() { return m_min_temperature; }
    void                    setMinTemperature(float value) { m_min_temperature = value; }
    float                   getSnowfall() { return m_snowfall; }
    void                    setSnowfall(float value) { m_snowfall = value; }
    float                   getPrecipitation() { return m_precipitation; }
    void                    setPrecipitation(float value) { m_precipitation = value; }
    std::string&			getStationName() { return m_station_name; }
    void					setStationName(std::string name) { m_station_name = name; }
    size_t					getDayOfYear() { return m_day_of_year; }
    void					setDayOfYear(size_t year) { m_day_of_year = year; }

protected:
    float                   m_max_temperature;
    float                   m_min_temperature;
    float                   m_snowfall;
    float                   m_precipitation;
    std::string				m_station_name;
    size_t					m_day_of_year;
};

class Month
{
public:
                            Month() : m_day_vector(MAX_DAYS_IN_MONTH)
                            {
                                setRecordMaxTemperature( float(INT_MIN) );
                                setRecordMinTemperature( float(INT_MAX) );
                                setRecordMaxDay(0);
                                setRecordMinDay(0);
                                setValid(false);
                            }

    bool                    getValid() { return m_valid; }
    void                    setValid(bool flag) { m_valid = flag; }
    std::vector<Day>&       getDayVector() { return m_day_vector; }
    float                   getRecordMaxTemperature() { return m_record_max_temperature; }
    void                    setRecordMaxTemperature(float value) { m_record_max_temperature = value; }
    float                   getRecordMinTemperature() { return m_record_min_temperature; }
    void                    setRecordMinTemperature(float value) { m_record_min_temperature = value; }
    unsigned int            getRecordMaxDay() { return m_record_max_day; }
    void                    setRecordMaxDay(unsigned int value) { m_record_max_day = value; }
    unsigned int            getRecordMinDay() { return m_record_min_day; }
    void                    setRecordMinDay(unsigned int value) { m_record_min_day = value; }
    float                   getTotalTemperature() { return m_total_temperature; }
    void                    setTotalTemperature(float value) { m_total_temperature = value; }
    void                    addToTotalTemperature(float value) { m_total_temperature += value; }
    unsigned int            getNumberOfTemperatures() { return m_number_of_temperatures; }
    void                    setNumberOfTemperatures(unsigned int value) { m_number_of_temperatures = value; }
    size_t                  getMonthNumber() { return m_month_number; }
    void                    setMonthNumber(unsigned int value) { m_month_number = value; }
    void                    incrementNumberOfTemperatures() { m_number_of_temperatures++; }

protected:
    bool                    m_valid;
    std::vector<Day>        m_day_vector;
    float                   m_record_max_temperature;
    float                   m_record_min_temperature;
    unsigned int            m_record_max_day;
    unsigned int            m_record_min_day;
    float                   m_total_temperature;
    unsigned int            m_number_of_temperatures;
    size_t                  m_month_number;
};

class Year
{
public:
                            Year() : m_month_vector(NUMBER_OF_MONTHS_PER_YEAR)
                            {
                                setRecordMaxTemperature( float(INT_MIN) );
                                setRecordMinTemperature( float(INT_MAX) );
                                setRecordMaxMonth(0);
                                setRecordMinMonth(0);
                                setMaxDaysBetweenSnowfalls(0);
                                setNumberOfDaysAboveMaxThreshold(0);
                                setNumberOfConsecutiveDaysAboveMaxThreshold(0);
                                setNumberOfConsecutiveDaysBelowMaxThreshold(0);
                                setNumberOfConsecutiveDaysBelowMinThreshold(0);
                                setNumberOfConsecutiveDaysAboveMinThreshold(0);
                                setNumberOfDaysBelowMaxThreshold(0);
                                setNumberOfDaysAboveMinThreshold(0);
                                setNumberOfDaysBelowMinThreshold(0);
                                setNumberOfDaysAboveMinThreshold(0);
                                setNumberOfDaysWithSnow(0);
                                setNumberOfDaysWithoutSnow(0);
                            }

    std::vector<Month>&     getMonthVector() { return m_month_vector; }
    unsigned int            getYear() { return m_year; }
    void                    setYear(unsigned int value) { m_year = value; }
    float                   getRecordMaxTemperature() { return m_record_max_temperature; }
    void                    setRecordMaxTemperature(float value) { m_record_max_temperature = value; }
    float                   getRecordMinTemperature() { return m_record_min_temperature; }
    void                    setRecordMinTemperature(float value) { m_record_min_temperature = value; }
    unsigned int            getRecordMaxMonth() { return m_record_max_month; }
    void                    setRecordMaxMonth(unsigned int value) { m_record_max_month = value; }
    unsigned int            getRecordMinMonth() { return m_record_min_month; }
    void                    setRecordMinMonth(unsigned int value) { m_record_min_month = value; }
    float                   getTotalTemperature() { return m_total_temperature; }
    void                    setTotalTemperature(float value) { m_total_temperature = value; }
    void                    addToTotalTemperature(float value) { m_total_temperature += value; }
    unsigned int            getNumberOfTemperatures() { return m_number_of_temperatures; }
    void                    setNumberOfTemperatures(unsigned int value) { m_number_of_temperatures = value; }
    void                    incrementNumberOfTemperatures() { m_number_of_temperatures++; }
    float                   getRecordSnowfall() { return m_record_snowfall; }
    void                    setRecordSnowfall(float value) { m_record_snowfall = value; }
    float                   getRecordPrecipitation() { return m_record_precipitation; }
    void                    setRecordPrecipitation(float value) { m_record_precipitation = value; }
    unsigned int            getMaxDaysBetweenSnowfalls() { return m_max_days_between_snowfalls; }
    void                    setMaxDaysBetweenSnowfalls(unsigned int value) { m_max_days_between_snowfalls = value; }
    unsigned int            getMaxDaysBetweenPrecipitation() { return m_max_days_between_preciptitation; }
    void                    setMaxDaysBetweenPrecipitation(unsigned int value) { m_max_days_between_preciptitation = value; }
    size_t					getNumberOfConsecutiveDaysAboveMaxThreshold() { return m_number_of_consecutive_days_above_max_threshold; }
    size_t					getNumberOfConsecutiveDaysBelowMaxThreshold() { return m_number_of_consecutive_days_below_max_threshold; }
    size_t					getNumberOfConsecutiveDaysBelowMinThreshold() { return m_number_of_consecutive_days_below_min_threshold; }
    size_t					getNumberOfConsecutiveDaysAboveMinThreshold() { return m_number_of_consecutive_days_above_min_threshold; }
    size_t					getNumberOfDaysAboveMaxThreshold() { return m_number_of_days_above_max_threshold; }
    size_t					getNumberOfDaysBelowMaxThreshold() { return m_number_of_days_below_max_threshold; }
    size_t					getNumberOfDaysBelowMinThreshold() { return m_number_of_days_below_min_threshold; }
    size_t					getNumberOfDaysAboveMinThreshold() { return m_number_of_days_above_min_threshold; }
    void					incrementNumberOfConsecutiveDaysAboveMaxThreshold() { m_number_of_consecutive_days_above_max_threshold++; }
    void					incrementNumberOfConsecutiveDaysBelowMaxThreshold() { m_number_of_consecutive_days_below_max_threshold++; }
    void					incrementNumberOfConsecutiveDaysBelowMinThreshold() { m_number_of_consecutive_days_below_min_threshold++; }
    void					incrementNumberOfConsecutiveDaysAboveMinThreshold() { m_number_of_consecutive_days_above_min_threshold++; }
    void					incrementNumberOfDaysAboveMaxThreshold() { m_number_of_days_above_max_threshold++; }
    void					incrementNumberOfDaysBelowMaxThreshold() { m_number_of_days_below_max_threshold++; }
    void					incrementNumberOfDaysBelowMinThreshold() { m_number_of_days_below_min_threshold++; }
    void					incrementNumberOfDaysAboveMinThreshold() { m_number_of_days_above_min_threshold++; }
    void					setNumberOfConsecutiveDaysAboveMaxThreshold(size_t value) { m_number_of_consecutive_days_above_max_threshold = value; }
    void					setNumberOfConsecutiveDaysBelowMaxThreshold(size_t value) { m_number_of_consecutive_days_below_max_threshold = value; }
    void					setNumberOfConsecutiveDaysBelowMinThreshold(size_t value) { m_number_of_consecutive_days_below_min_threshold = value; }
    void					setNumberOfConsecutiveDaysAboveMinThreshold(size_t value) { m_number_of_consecutive_days_above_min_threshold = value; }
    void					setNumberOfDaysAboveMaxThreshold(size_t value) { m_number_of_days_above_max_threshold = value; }
    void					setNumberOfDaysBelowMaxThreshold(size_t value) { m_number_of_days_below_max_threshold = value; }
    void					setNumberOfDaysBelowMinThreshold(size_t value) { m_number_of_days_below_min_threshold = value; }
    void					setNumberOfDaysAboveMinThreshold(size_t value) { m_number_of_days_above_min_threshold = value; }
    size_t					getNumberOfDaysWithSnow() { return m_number_of_days_with_snow; }
    size_t					getNumberOfDaysWithoutSnow() { return m_number_of_days_without_snow; }
    void					incrementNumberOfDaysWithSnow() { m_number_of_days_with_snow++; }
    void					incrementNumberOfDaysWithoutSnow() { m_number_of_days_without_snow++; }
    void					setNumberOfDaysWithSnow(size_t value) { m_number_of_days_with_snow = value; }
    void					setNumberOfDaysWithoutSnow(size_t value) { m_number_of_days_without_snow = value; }

protected:
    std::vector<Month>      m_month_vector;
    unsigned int            m_year;
    float                   m_record_max_temperature;
    float                   m_record_min_temperature;
    float                   m_record_precipitation;
    float                   m_record_snowfall;
    unsigned int            m_record_max_month;
    unsigned int            m_record_min_month;
    float                   m_total_temperature;
    unsigned int            m_number_of_temperatures;
    unsigned int            m_max_days_between_snowfalls;
    unsigned int            m_max_days_between_preciptitation;
    size_t					m_number_of_days_with_snow;
    size_t					m_number_of_days_without_snow;
    size_t					m_number_of_days_above_max_threshold;
    size_t					m_number_of_days_below_max_threshold;
    size_t					m_number_of_days_below_min_threshold;
    size_t					m_number_of_days_above_min_threshold;
    size_t					m_number_of_consecutive_days_above_max_threshold;
    size_t					m_number_of_consecutive_days_below_max_threshold;
    size_t					m_number_of_consecutive_days_below_min_threshold;
    size_t					m_number_of_consecutive_days_above_min_threshold;
};

class Station
{
public:
                            Station()
                            {
                                setRecordMaxTemperature( float(INT_MIN) );
                                setRecordMinTemperature( float(INT_MAX) );
                                setRecordMaxYear(0);
                                setRecordMinYear(0);
                                setRecordSnowfall(0);
                                setRecordSnowfallYear(0);
                                setRecordSnowfallMonth(0);
                                setRecordSnowfallDayOfMonth(0);
                                setRecordPrecipitation(0);

                                for (unsigned int i = 0; i < NUMBER_OF_MONTHS_PER_YEAR; i++)
                                {
                                	getMinBaselineAveragePerMonthVector().push_back(0);
                                	getMinBaselineTotalPerMonthVector().push_back(0);
                                	getMinBaselineCountPerMonthVector().push_back(0);
                                	getMaxBaselineAveragePerMonthVector().push_back(0);
                                	getMaxBaselineTotalPerMonthVector().push_back(0);
                                	getMaxBaselineCountPerMonthVector().push_back(0);
                                }
                            }

                            std::vector<Year>&      getYearVector() { return m_year_vector; }
    std::string&            getStationNumber() { return m_station_number; }
    void                    setStationNumber(std::string value) { m_station_number = value; }
    std::string&            getStationName() { return m_station_name; }
    void                    setStationName(std::string name) { m_station_name = name; }
    std::string&            getStateName() { return m_state_name; }
    void                    setStateName(std::string name) { m_state_name = name; }
    std::string&			getLatitude() { return m_latitude; }
    void					setLatitude(std::string value) { m_latitude = value; }
    std::string&			getLongitude() { return m_longitude; }
    void					setLongitude(std::string value) { m_longitude = value; }
    int                     getStateNumber() { return m_state_number; }
    void                    setStateNumber(int value) { m_state_number = value; }
    float                   getRecordMaxTemperature() { return m_record_max_temperature; }
    void                    setRecordMaxTemperature(float value) { m_record_max_temperature = value; }
    float                   getRecordMinTemperature() { return m_record_min_temperature; }
    void                    setRecordMinTemperature(float value) { m_record_min_temperature = value; }
    unsigned int            getRecordMaxYear() { return m_record_max_year; }
    void                    setRecordMaxYear(unsigned int value) { m_record_max_year = value; }
    unsigned int            getRecordMinYear() { return m_record_min_year; }
    void                    setRecordMinYear(unsigned int value) { m_record_min_year = value; }
    unsigned int            getRecordMaxMonth() { return m_record_max_month; }
    void                    setRecordMaxMonth(unsigned int value) { m_record_max_month = value; }
    unsigned int            getRecordMinMonth() { return m_record_min_month; }
    void                    setRecordMinMonth(unsigned int value) { m_record_min_month = value; }
    unsigned int            getRecordMaxDayOfYear() { return m_record_max_day_of_year; }
    void                    setRecordMaxDayOfYear(unsigned int value) { m_record_max_day_of_year = value; }
    unsigned int            getRecordMinDayOfYear() { return m_record_min_day_of_year; }
    void                    setRecordMinDayOfYear(unsigned int value) { m_record_min_day_of_year = value; }
    unsigned int            getRecordMaxDayOfMonth() { return m_record_max_day_of_month; }
    void                    setRecordMaxDayOfMonth(unsigned int value) { m_record_max_day_of_month = value; }
    unsigned int            getRecordMinDayOfMonth() { return m_record_min_day_of_month; }
    void                    setRecordMinDayOfMonth(unsigned int value) { m_record_min_day_of_month = value; }
    float                   getRecordSnowfall() { return m_record_snowfall; }
    void                    setRecordSnowfall(float value) { m_record_snowfall = value; }
    float                   getRecordPrecipitation() { return m_record_precipitation; }
    void                    setRecordPrecipitation(float value) { m_record_precipitation = value; }
    unsigned int            getRecordPrecipitationYear() { return m_record_precipitation_year; }
    void                    setRecordPrecipitationYear(unsigned int value) { m_record_precipitation_year = value; }
    unsigned int            getRecordSnowfallYear() { return m_record_snowfall_year; }
    void                    setRecordSnowfallYear(unsigned int value) { m_record_snowfall_year = value; }
    unsigned int            getRecordPrecipitationMonth() { return m_record_precipitation_month; }
    void                    setRecordPrecipitationMonth(unsigned int value) { m_record_precipitation_month = value; }
    unsigned int            getRecordSnowfallMonth() { return m_record_snowfall_month; }
    void                    setRecordSnowfallMonth(unsigned int value) { m_record_snowfall_month = value; }
    unsigned int            getRecordPrecipitationDayOfYear() { return m_record_precipitation_day_of_year; }
    void                    setRecordPrecipitationDayOfYear(unsigned int value) { m_record_precipitation_day_of_year = value; }
    unsigned int            getRecordSnowfallDayOfYear() { return m_record_snowfall_day_of_year; }
    void                    setRecordSnowfallDayOfYear(unsigned int value) { m_record_snowfall_day_of_year = value; }
    unsigned int            getRecordPrecipitationDayOfMonth() { return m_record_precipitation_day_of_month; }
    void                    setRecordPrecipitationDayOfMonth(unsigned int value) { m_record_precipitation_day_of_month = value; }
    unsigned int            getRecordSnowfallDayOfMonth() { return m_record_snowfall_day_of_month; }
    void                    setRecordSnowfallDayOfMonth(unsigned int value) { m_record_snowfall_day_of_month = value; }
    std::map<size_t,bool>&  getYearContainsTemperatureDataMap() { return m_year_contains_temperature_data_map; }
    std::vector<float>&		getMinBaselineAveragePerMonthVector() { return m_min_average_per_month_vector; }
    std::vector<float>&		getMinBaselineTotalPerMonthVector() { return m_min_total_per_month_vector; }
    std::vector<size_t>&	getMinBaselineCountPerMonthVector() { return m_min_count_per_month_vector; }
    std::vector<float>&		getMaxBaselineAveragePerMonthVector() { return m_max_average_per_month_vector; }
    std::vector<float>&		getMaxBaselineTotalPerMonthVector() { return m_max_total_per_month_vector; }
    std::vector<size_t>&	getMaxBaselineCountPerMonthVector() { return m_max_count_per_month_vector; }


protected:
    std::vector<Year>       m_year_vector;
    std::string             m_station_number;
    std::string             m_station_name;
    std::string             m_state_name;
    int                     m_state_number;
    std::string				m_latitude;
    std::string				m_longitude;
    float                   m_record_max_temperature;
    float                   m_record_min_temperature;
    float                   m_record_precipitation;
    float                   m_record_snowfall;
    unsigned int            m_record_max_year;
    unsigned int            m_record_min_year;
    unsigned int            m_record_max_month;
    unsigned int            m_record_min_month;
    unsigned int            m_record_max_day_of_year;
    unsigned int            m_record_min_day_of_year;
    unsigned int            m_record_max_day_of_month;
    unsigned int            m_record_min_day_of_month;
    unsigned int            m_record_precipitation_year;
    unsigned int            m_record_snowfall_year;
    unsigned int            m_record_precipitation_month;
    unsigned int            m_record_snowfall_month;
    unsigned int            m_record_precipitation_day_of_year;
    unsigned int            m_record_snowfall_day_of_year;
    unsigned int            m_record_precipitation_day_of_month;
    unsigned int            m_record_snowfall_day_of_month;
    std::map<size_t,bool>	m_year_contains_temperature_data_map;
    std::vector<float>		m_min_average_per_month_vector;
    std::vector<float>		m_min_total_per_month_vector;
    std::vector<size_t>		m_min_count_per_month_vector;
    std::vector<float>		m_max_average_per_month_vector;
    std::vector<float>		m_max_total_per_month_vector;
    std::vector<size_t>		m_max_count_per_month_vector;
};

class State
{
public:
                            State() 
                            {
                                setRecordMaxTemperature( float(INT_MIN) );
                                setRecordMinTemperature( float(INT_MAX) );
                                setRecordMaxYear(0);
                                setRecordMinYear(0);
                            }

                            ~State()
                            {
                                for (size_t i = 0; i < getStationVector().size(); i++ )
                            	{
                            		delete getStationVector()[i];
                            	}
                            }

    std::vector<Station*>&  getStationVector() { return m_station_vector; }
    unsigned int            getStateNumber() { return m_state_number; }
    void                    setStateNumber(unsigned int value) { m_state_number = value; }
    std::string             getStateName() { return std::string( STATE_NAMES[ getStateNumber() ] ); }
    float                   getRecordMaxTemperature() { return m_record_max_temperature; }
    void                    setRecordMaxTemperature(float value) { m_record_max_temperature = value; }
    float                   getRecordMinTemperature() { return m_record_min_temperature; }
    void                    setRecordMinTemperature(float value) { m_record_min_temperature = value; }
    unsigned int            getRecordMaxYear() { return m_record_max_year; }
    void                    setRecordMaxYear(unsigned int value) { m_record_max_year = value; }
    unsigned int            getRecordMinYear() { return m_record_min_year; }
    void                    setRecordMinYear(unsigned int value) { m_record_min_year = value; }

protected:
    std::vector<Station*>   m_station_vector;
    unsigned int            m_state_number;
    float                   m_record_max_temperature;
    float                   m_record_min_temperature;
    unsigned int            m_record_max_year;
    unsigned int            m_record_min_year;
};

class Country
{
public:
                            Country() : m_state_vector(NUMBER_OF_STATES)
                            {
                                setRecordMaxTemperature( float(INT_MIN) );
                                setRecordMinTemperature( float(INT_MAX) );
                                setRecordMaxYear(0);
                                setRecordMinYear(0);
                            }

    std::map<float,size_t>& getVariableMonthMeanAverageMap() { return m_variable_month_mean_average_map; }
    std::map<float,size_t>& getVariableMonthMeanMaximumMap() { return m_variable_month_mean_maximum_map; }
    std::map<float,size_t>& getVariableMonthMeanMinimumMap() { return m_variable_month_mean_minimum_map; }
    std::vector<State>&     getStateVector() { return m_state_vector; }
    float                   getRecordMaxTemperature() { return m_record_max_temperature; }
    void                    setRecordMaxTemperature(float value) { m_record_max_temperature = value; }
    float                   getRecordMinTemperature() { return m_record_min_temperature; }
    void                    setRecordMinTemperature(float value) { m_record_min_temperature = value; }
    unsigned int            getRecordMaxYear() { return m_record_max_year; }
    void                    setRecordMaxYear(unsigned int value) { m_record_max_year = value; }
    unsigned int            getRecordMinYear() { return m_record_min_year; }
    void                    setRecordMinYear(unsigned int value) { m_record_min_year = value; }

protected:
    std::map<float,size_t>  m_variable_month_mean_average_map;
    std::map<float,size_t>  m_variable_month_mean_maximum_map;
    std::map<float,size_t>  m_variable_month_mean_minimum_map;
    std::vector<State>      m_state_vector;
    float                   m_record_max_temperature;
    float                   m_record_min_temperature;
    unsigned int            m_record_max_year;
    unsigned int            m_record_min_year;
};

#endif // GHCN_H_INCLUDED



