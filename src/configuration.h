/***************************************************************************
**                                                                        **
**  FlySight Configurator                                                 **
**  Copyright 2018 Michael Cooper                                         **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>. **
**                                                                        **
****************************************************************************
**  Contact: Michael Cooper                                               **
**  Website: http://flysight.ca/                                          **
****************************************************************************/

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QString>
#include <QVector>

class Configuration
{
public:
    typedef enum {
        Metric = 0,
        Imperial
    } DisplayUnits;

    typedef enum {
        Portable   = 0,
        Stationary = 2,
        Pedestrian = 3,
        Automotive = 4,
        Sea        = 5,
        Airborne1G = 6,
        Airborne2G = 7,
        Airborne4G = 8
    } Model;

    typedef enum {
        HorizontalSpeed   = 0,
        VerticalSpeed     = 1,
        GlideRatio        = 2,
        InverseGlideRatio = 3,
        TotalSpeed        = 4,
        Altitude          = 5,
        ValueMagnitude    = 8,
        ValueChange       = 9,
        DiveAngle         = 11
    } Mode;

    typedef enum {
        NoTone       = 0,
        Clamp        = 1,
        Chirp        = 2,
        ChirpReverse = 3
    } Limits;

    typedef enum {
        Kilometers = 0,
        Miles      = 1,
        Knots      = 2
    } Units;

    typedef struct {
        Mode mode;
        Units units;
        int decimals;
    } Speech;

    typedef enum {
        NoInit   = 0,
        InitTest = 1,
        InitFile = 2
    } InitMode;

    typedef enum {
        NoAlarm   = 0,
        Beep      = 1,
        ChirpUp   = 2,
        ChirpDown = 3,
        PlayFile  = 4
    } AlarmMode;

    typedef struct {
        int elevation;
        AlarmMode mode;
        QString file;
    } Alarm;

    typedef struct {
        int top;
        int bottom;
    } Window;

    typedef enum {
        Meters = 0,
        Feet   = 1
    } AltitudeUnits;

    typedef QVector< Speech > Speeches;
    typedef QVector< Alarm > Alarms;
    typedef QVector< Window > Windows;

    DisplayUnits displayUnits;

    QString configName;
    QString configDescription;
    QString configKind;

    Model model;
    int   rate;

    Mode toneMode;
    int minTone;
    int maxTone;
    Limits limits;
    int toneVolume;

    Mode rateMode;
    int minRateValue;
    int maxRateValue;
    int minRate;
    int maxRate;
    bool flatline;

    int speechRate;
    int speechVolume;

    Speeches speeches;

    int vThreshold;
    int hThreshold;

    bool adjustSpeed;
    int timeZoneOffset;

    InitMode initMode;
    QString initFile;

    int alarmWindowAbove;
    int alarmWindowBelow;
    int groundElevation;

    Alarms alarms;
    Windows windows;

    AltitudeUnits altitudeUnits;
    int altitudeStep;

    Configuration(DisplayUnits units = Metric);

    QString speedUnits() const;
    QString distanceUnits() const;

    void vThresholdFromUnits(double valueInUnits);
    double vThresholdToUnits() const;

    void hThresholdFromUnits(double valueInUnits);
    double hThresholdToUnits() const;

    void alarmWindowAboveFromUnits(double valueInUnits);
    double alarmWindowAboveToUnits() const;

    void alarmWindowBelowFromUnits(double valueInUnits);
    double alarmWindowBelowToUnits() const;

    void groundElevationFromUnits(double valueInUnits);
    double groundElevationToUnits() const;

    int valueFromSpeedUnits(double valueInUnits) const;
    double valueToSpeedUnits(int value) const;

    int valueFromDistanceUnits(double valueInUnits) const;
    double valueToDistanceUnits(int value) const;

    double minToneToUnits() const;
    void minToneFromUnits(double valueInUnits);

    double maxToneToUnits() const;
    void maxToneFromUnits(double valueInUnits);

    double toneToUnits(int value) const;
    int toneFromUnits(double valueInUnits) const;

    double minRateToUnits() const;
    void minRateFromUnits(double valueInUnits);

    double maxRateToUnits() const;
    void maxRateFromUnits(double valueInUnits);

    double rateToUnits(int value) const;
    int rateFromUnits(double valueInUnits) const;
};

bool operator==(const Configuration &a, const Configuration &b);

#endif // CONFIGURATION_H
