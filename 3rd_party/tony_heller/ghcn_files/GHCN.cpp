//--------------------------------------------------------------------------------------
// GHCN.cpp
// Code for parsing and processing daily GHCN data from
// http://cdiac.ornl.gov/ftp/ushcn_daily
// Written by Steve Goddard
// If you modify it and mess it up, don't blame it on me

#include "GHCN.h"

extern size_t most_recent_year;
extern size_t baseline_start_year;
extern size_t baseline_end_year;
extern int temperature_threshold_high;
extern int temperature_threshold_low;


void
DataRecord::setHighTemperature(unsigned int day_of_month, float value)
{
    getDailyHighTemperatureVector()[day_of_month] = value;
}

void
DataRecord::setLowTemperature(unsigned int day_of_month, float value)
{
    getDailyLowTemperatureVector()[day_of_month] = value;
}

void
DataRecord::setSnowfall(unsigned int day_of_month, float value)
{
    getDailySnowVector()[day_of_month] = value;
}

void
DataRecord::setPrecipitation(unsigned int day_of_month, float value)
{
    getDailyPrecipitationVector()[day_of_month] = value;
}

void                    
DataRecord::parseTemperatureRecord(Station* station, std::string record_string)
{
    /*
    ------------------------------
    Variable   Columns   Type
    ------------------------------
    ID            1-11   Character
    YEAR         12-15   Integer
    MONTH        16-17   Integer
    ELEMENT      18-21   Character
    VALUE1       22-26   Integer
    MFLAG1       27-27   Character
    QFLAG1       28-28   Character
    SFLAG1       29-29   Character
    VALUE2       30-34   Integer
    MFLAG2       35-35   Character
    QFLAG2       36-36   Character
    SFLAG2       37-37   Character
      .           .          .
      .           .          .
      .           .          .
    VALUE31    262-266   Integer
    MFLAG31    267-267   Character
    QFLAG31    268-268   Character
    SFLAG31    269-269   Character
    ------------------------------
    */

    size_t position = 0;

    if (record_string.length() < 269)
    {
        return;
    }

    setStateNumber(1);

    std::string station_number = record_string.substr(0, 11);
    setStationNumber(station_number);
    std::string country_abbreviation = record_string.substr(0, 2);
    setCountryAbbreviation(country_abbreviation);
    //std::string state_name = getStateName();
    unsigned int year = strtoul( record_string.substr(11, 4).c_str(), NULL, 10 );

    if (year > most_recent_year)
    {
        most_recent_year = year;
    }

    setYear(year);
    unsigned int month = strtoul( record_string.substr(15, 2).c_str(), NULL, 10 );
    setMonth(month);
    std::string record_type = record_string.substr(17, 4);
    //std::cout << record_type << std::endl;
    setRecordTypeString(record_type);

    position = 21;

    for (unsigned int i = 0; i < MAX_DAYS_IN_MONTH; i++)
    {
        std::string value_string =  record_string.substr(position, 5);
        long value = strtol( value_string.c_str(), NULL, 10 );

        if ( getRecordTypeString() == "TMIN" )
        {
            getDailyLowTemperatureVector()[i] = (value == -9999) ? UNKNOWN_TEMPERATURE : float(value) / 10.0f;

            if (getDailyLowTemperatureVector()[i] != UNKNOWN_TEMPERATURE)
            {
                setContainsTemperatureData(true);

                if (year >= baseline_start_year && year <= baseline_end_year)
                {
                    float monthly_total = station->getMinBaselineTotalPerMonthVector()[month - 1] + value;
                    station->getMinBaselineTotalPerMonthVector()[month - 1] = monthly_total;
                    //std::cerr << "Monthly total for month " << month << " = " << station->getMinBaselineTotalPerMonthVector()[month - 1] << std::endl;
                    size_t monthly_count = station->getMinBaselineCountPerMonthVector()[month - 1] + 1;
                    station->getMinBaselineCountPerMonthVector()[month - 1] = monthly_count;
                }
            }
        }
        else if ( getRecordTypeString() == "TMAX" )
        {
            getDailyHighTemperatureVector()[i] = (value == -9999) ? UNKNOWN_TEMPERATURE : float(value) / 10.0f;
            //std::cout << getDailyHighTemperatureVector()[i] << std::endl;

            if (getDailyHighTemperatureVector()[i] != UNKNOWN_TEMPERATURE)
            {
                setContainsTemperatureData(true);

                if (year >= baseline_start_year && year <= baseline_end_year)
                {
                    float monthly_total = station->getMaxBaselineTotalPerMonthVector()[month - 1] + value;
                    station->getMaxBaselineTotalPerMonthVector()[month - 1] = monthly_total;
                    size_t monthly_count = station->getMaxBaselineCountPerMonthVector()[month - 1] + 1;
                    station->getMaxBaselineCountPerMonthVector()[month - 1] = monthly_count;
                }
            }
        }
        else if ( getRecordTypeString() == "SNOW" )
        {
            getDailySnowVector()[i] = (value == -9999) ? UNKNOWN_TEMPERATURE : float(value) / 10.0f;
        }
        else if ( getRecordTypeString() == "PRCP" )
        {
            getDailyPrecipitationVector()[i] = (value == -9999) ? UNKNOWN_TEMPERATURE : float(value) / 100.0f;
        }

        position += 8;
    }
}

size_t
DataRecord::getDayOfYearForStartOfRecord()
{
    // January is 0 December is 11
    size_t month = getMonth();
    size_t year = getYear();
    size_t day = 0;

    for (size_t i = 1; i < month; i++)
    {
        if ( (i - 1) == FEBRUARY )
        {
            bool leap_year = ( (year % 400) == 0 ) ? true : ( (year % 100) == 0 ) ? false : (year % 4) ? false : true;

            if (leap_year)
            {
                day += 29;
            }
            else
            {
                day += 28;
            }
        }
        else
        {
            day += NUMBER_OF_DAYS_PER_MONTH[i - 1];
        }
    }

    return day;
}

bool 
leapYear(size_t year)
{
    bool leap_year = ( (year % 400) == 0 ) ? true : ( (year % 100) == 0 ) ? false : (year % 4) ? false : true;
    return leap_year;
}

bool
lastDayOfYear(size_t day_of_year, size_t year)
{
    if ( leapYear(year) )
    {
        if ( day_of_year == 366 )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if ( day_of_year == 365 )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

size_t
getDayOfYear(size_t day, size_t month, size_t year)
{
    for (size_t i = 1; i < month; i++)
    {
        if ( (i - 1) == FEBRUARY )
        {
            bool leap_year = leapYear(year);

            if (leap_year)
            {
                day += 29;
            }
            else
            {
                day += 28;
            }
        }
        else
        {
            day += NUMBER_OF_DAYS_PER_MONTH[i - 1];
        }
    }

    return day;
}

std::string dateString(int year, int day_of_year)
{
    if (day_of_year == 0 || day_of_year > NUMBER_OF_DAYS_PER_YEAR)
    {
        return std::string("None");
    }

    int days_remaining = day_of_year;
    int month = 0;
    int number_of_days_this_month = NUMBER_OF_DAYS_PER_MONTH[month];

    while (days_remaining > number_of_days_this_month)
    {
        days_remaining -= number_of_days_this_month;
        month++;
        number_of_days_this_month = ( month == FEBRUARY && !leapYear(year) ) ?
                    NUMBER_OF_DAYS_PER_MONTH[month] - 1 : NUMBER_OF_DAYS_PER_MONTH[month];
    }

    char date[100];
    sprintf(date, "%s %d", MONTH_NAMES[month], days_remaining);
    return std::string(date);
}

void linearRegression(double& a, double& b, std::vector<double>& x, std::vector<double>& y)
{
    double xAvg = 0;
    double yAvg = 0;

    for (int i = 0; i < x.size(); i++)
    {
        xAvg += x[i];
        yAvg += y[i];
    }

    xAvg = xAvg / x.size();
    yAvg = yAvg / y.size();

    double v1 = 0;
    double v2 = 0;

    for (int i = 0; i < x.size(); i++)
    {
        v1 += (x[i] - xAvg) * (y[i] - yAvg);
        v2 += pow(x[i] - xAvg, 2);
    }

    a = v1 / v2;
    b = yAvg - (a * xAvg);
}

void replaceString(std::string& original_string, std::string pattern_to_replace, std::string new_pattern)
{
    size_t index = 0;

    while (true)
    {
        /* Locate the substring to replace. */
        index = original_string.find(pattern_to_replace, index);

        if (index == std::string::npos)
        {
            break;
        }

        /* Make the replacement. */
        original_string.replace(index, pattern_to_replace.size(), new_pattern);

        /* Advance index forward so the next iteration doesn't pick it up as well. */
        index += new_pattern.size();
    }
}
