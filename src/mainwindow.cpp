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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QTextStream>

#include "alarmform.h"
#include "altitudeform.h"
#include "configurationpage.h"
#include "generalform.h"
#include "initializationform.h"
#include "miscellaneousform.h"
#include "rateform.h"
#include "silenceform.h"
#include "speechform.h"
#include "thresholdsform.h"
#include "toneform.h"

#define MAX_ALARMS  10
#define MAX_WINDOWS 2

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    updating(false)
{
    ui->setupUi(this);

    // Create configuration pages
    pages.append(new GeneralForm());
    pages.append(new ToneForm());
    pages.append(new RateForm());
    pages.append(new SpeechForm());
    pages.append(new ThresholdsForm());
    pages.append(new MiscellaneousForm());
    pages.append(new InitializationForm());
    pages.append(new AlarmForm());
    pages.append(new AltitudeForm());
    pages.append(new SilenceForm());

    // Add pages to configuration window
    foreach(ConfigurationPage *page, pages)
    {
        ui->listWidget->addItem(page->title());
        ui->stackedWidget->addWidget(page);

        connect(page, SIGNAL(selectionChanged()),
                this, SLOT(updateConfigurationOptions()));
    }

    ui->listWidget->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);

    // Connect list widget to stacked widget
    connect(ui->listWidget, SIGNAL(currentRowChanged(int)),
            ui->stackedWidget, SLOT(setCurrentIndex(int)));

    // Initialize units list
    ui->unitsComboBox->addItem("Metric");
    ui->unitsComboBox->addItem("Imperial");

    // Watch for unit changes
    connect(ui->unitsComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setUnits(int)));

    // Initial update
    updatePages();

    // Initialize file name
    setCurrentFile(QString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::save()
{
    if (curFile.isEmpty())
    {
        return saveAs();
    }
    else
    {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    // Initialize settings object
    QSettings settings("FlySight", "Configurator");

    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Save As"),
                settings.value("folder").toString(),
                tr("Configuration files (*.txt)"));

    // Return now if user canceled
    if (fileName.isEmpty()) return false;

    // Save the configuration
    return saveFile(fileName);
}

bool MainWindow::loadFile(
        const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    // Initialize settings object
    QSettings settings("FlySight", "Configurator");

    // Remember last file read
    settings.setValue("folder", QFileInfo(fileName).absoluteFilePath());

    // Reset configuration but keep units
    configuration = Configuration(configuration.displayUnits);

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();

        // Remove comments
        line = line.left(line.indexOf(';'));

        // Split into key/value
        QStringList cols = line.split(":");
        if (cols.length() < 2) continue;

        QString name = cols[0].trimmed();
        QString result = cols[1].trimmed();
        int val = result.toInt();

#define HANDLE_VALUE(s,w,t)\
if (!name.compare(s)) { (w) = (t) (val); }

        HANDLE_VALUE("Model", configuration.model, Configuration::Model);
        HANDLE_VALUE("Rate", configuration.rate, int);

        HANDLE_VALUE("Mode", configuration.toneMode, Configuration::Mode);
        HANDLE_VALUE("Min", configuration.minTone, int);
        HANDLE_VALUE("Max", configuration.maxTone, int);
        HANDLE_VALUE("Limits", configuration.limits, Configuration::Limits);
        HANDLE_VALUE("Volume", configuration.toneVolume, int);

        HANDLE_VALUE("Mode_2", configuration.rateMode, Configuration::Mode);
        HANDLE_VALUE("Min_Val_2", configuration.minRateValue, int);
        HANDLE_VALUE("Max_Val_2", configuration.maxRateValue, int);
        HANDLE_VALUE("Min_Rate", configuration.minRate, int);
        HANDLE_VALUE("Max_Rate", configuration.maxRate, int);
        HANDLE_VALUE("Flatline", configuration.flatline, bool);

        HANDLE_VALUE("Sp_Rate", configuration.speechRate, int);
        HANDLE_VALUE("Sp_Volume", configuration.speechVolume, int);

        HANDLE_VALUE("V_Thresh", configuration.vThreshold, int);
        HANDLE_VALUE("H_Thresh", configuration.hThreshold, int);

        HANDLE_VALUE("Use_SAS", configuration.adjustSpeed, bool);
        HANDLE_VALUE("TZ_Offset", configuration.timeZoneOffset, int);

        HANDLE_VALUE("Init_Mode", configuration.initMode, Configuration::InitMode);

        HANDLE_VALUE("Alt_Units", configuration.altitudeUnits, Configuration::AltitudeUnits);
        HANDLE_VALUE("Alt_Step", configuration.altitudeStep, int);

        HANDLE_VALUE("Window", configuration.alarmWindowAbove, int);
        HANDLE_VALUE("Window", configuration.alarmWindowBelow, int);
        HANDLE_VALUE("Win_Above", configuration.alarmWindowAbove, int);
        HANDLE_VALUE("Win_Below", configuration.alarmWindowBelow, int);
        HANDLE_VALUE("DZ_Elev", configuration.groundElevation, int);

#undef HANDLE_VALUE

        if (!name.compare("Config_Name"))
        {
            configuration.configName = result;
        }
        if (!name.compare("Config_Description"))
        {
            configuration.configDescription = result;
        }
        if (!name.compare("Config_Kind"))
        {
            configuration.configKind = result;
        }

        if (!name.compare("Init_File"))
        {
            configuration.initFile = result;
        }

        if (!name.compare("Alarm_Elev") && configuration.alarms.length() < MAX_ALARMS)
        {
            Configuration::Alarm alarm;
            alarm.elevation = val;
            alarm.mode = Configuration::NoAlarm;
            alarm.file = QString();
            configuration.alarms.push_back(alarm);
        }
        if (!name.compare("Alarm_Type"))
        {
            configuration.alarms.back().mode = (Configuration::AlarmMode) val;
        }
        if (!name.compare("Alarm_File"))
        {
            configuration.alarms.back().file = result;
        }

        if (!name.compare("Win_Top") && configuration.windows.length() < MAX_WINDOWS)
        {
            Configuration::Window window;
            window.top = val;
            window.bottom = val;
            configuration.windows.push_back(window);
        }
        if (!name.compare("Win_Bottom"))
        {
            configuration.windows.back().bottom = val;
        }

        if (!name.compare("Sp_Mode") && configuration.alarms.length() < MAX_ALARMS)
        {
            Configuration::Speech speech;
            speech.mode = (Configuration::Mode) val;
            speech.units = Configuration::Miles;
            speech.decimals = 1;
            configuration.speeches.push_back(speech);
        }
        if (!name.compare("Sp_Units"))
        {
            configuration.speeches.back().units = (Configuration::Units) val;
        }
        if (!name.compare("Sp_Dec"))
        {
            configuration.speeches.back().decimals = (int) val;
        }
    }

    // Update configuration
    updatePages();

    // Update file name
    setCurrentFile(fileName);

    return true;
}

bool MainWindow::saveFile(
        const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    // Initialize settings object
    QSettings settings("FlySight", "Configurator");

    // Remember last file read
    settings.setValue("folder", QFileInfo(fileName).absoluteFilePath());

    // Update configuration
    foreach(ConfigurationPage *page, pages)
    {
        page->updateConfiguration(configuration, ConfigurationPage::Values);
    }

    QTextStream out(&file);

    out << "; For information on configuring FlySight, please go to" << endl;
    out << ";     http://flysight.ca/wiki" << endl << endl;

    out << "; GPS settings" << endl << endl;

    out << "Config_Name:  " << configuration.configName.rightJustified(5) << " ; Configuration name" << endl;
    out << "Config_Description:  " << configuration.configDescription.rightJustified(5) << " ; Configuration Description" << endl;
    out << "Config_Kind:  " << configuration.configKind.rightJustified(5) << " ; Configuration kind. Allows to group configuration files together" << endl << endl;

    out << "Model:      " << QString("%1").arg(configuration.model, 5) << " ; Dynamic model" << endl;
    out << "                  ;   0 = Portable" << endl;
    out << "                  ;   2 = Stationary" << endl;
    out << "                  ;   3 = Pedestrian" << endl;
    out << "                  ;   4 = Automotive" << endl;
    out << "                  ;   5 = Sea" << endl;
    out << "                  ;   6 = Airborne with < 1 G acceleration" << endl;
    out << "                  ;   7 = Airborne with < 2 G acceleration" << endl;
    out << "                  ;   8 = Airborne with < 4 G acceleration" << endl;
    out << "Rate:       " << QString("%1").arg(configuration.rate, 5) << " ; Measurement rate (ms)" << endl << endl;

    out << "; Tone settings" << endl << endl;

    out << "Mode:       " << QString("%1").arg(configuration.toneMode, 5) << " ; Measurement mode" << endl;
    out << "                  ;   0 = Horizontal speed" << endl;
    out << "                  ;   1 = Vertical speed" << endl;
    out << "                  ;   2 = Glide ratio" << endl;
    out << "                  ;   3 = Inverse glide ratio" << endl;
    out << "                  ;   4 = Total speed" << endl;
    out << "                  ;   11 = Dive angle" << endl;
    out << "Min:        " << QString("%1").arg(configuration.minTone, 5) << " ; Lowest pitch value" << endl;
    out << "                  ;   cm/s        in Mode 0, 1, or 4" << endl;
    out << "                  ;   ratio * 100 in Mode 2 or 3" << endl;
    out << "                  ;   degrees     in Mode 11" << endl;
    out << "Max:        " << QString("%1").arg(configuration.maxTone, 5) << " ; Highest pitch value" << endl;
    out << "                  ;   cm/s        in Mode 0, 1, or 4" << endl;
    out << "                  ;   ratio * 100 in Mode 2 or 3" << endl;
    out << "                  ;   degrees     in Mode 11" << endl;
    out << "Limits:     " << QString("%1").arg(configuration.limits, 5) << " ; Behaviour when outside bounds" << endl;
    out << "                  ;   0 = No tone" << endl;
    out << "                  ;   1 = Min/max tone" << endl;
    out << "                  ;   2 = Chirp up/down" << endl;
    out << "                  ;   3 = Chirp down/up" << endl;
    out << "Volume:     " << QString("%1").arg(configuration.toneVolume, 5) << " ; 0 (min) to 8 (max)" << endl << endl;

    out << "; Rate settings" << endl << endl;

    out << "Mode_2:     " << QString("%1").arg(configuration.rateMode, 5) << " ; Determines tone rate" << endl;
    out << "                  ;   0 = Horizontal speed" << endl;
    out << "                  ;   1 = Vertical speed" << endl;
    out << "                  ;   2 = Glide ratio" << endl;
    out << "                  ;   3 = Inverse glide ratio" << endl;
    out << "                  ;   4 = Total speed" << endl;
    out << "                  ;   8 = Magnitude of Value 1" << endl;
    out << "                  ;   9 = Change in Value 1" << endl;
    out << "                  ;   11 = Dive angle" << endl;
    out << "Min_Val_2:  " << QString("%1").arg(configuration.minRateValue, 5) << " ; Lowest rate value" << endl;
    out << "                  ;   cm/s          when Mode 2 = 0, 1, or 4" << endl;
    out << "                  ;   ratio * 100   when Mode 2 = 2 or 3" << endl;
    out << "                  ;   percent * 100 when Mode 2 = 9" << endl;
    out << "                  ;   degrees       when Mode 2 = 11" << endl;
    out << "Max_Val_2:  " << QString("%1").arg(configuration.maxRateValue, 5) << " ; Highest rate value" << endl;
    out << "                  ;   cm/s          when Mode 2 = 0, 1, or 4" << endl;
    out << "                  ;   ratio * 100   when Mode 2 = 2 or 3" << endl;
    out << "                  ;   percent * 100 when Mode 2 = 9" << endl;
    out << "                  ;   degrees       when Mode 2 = 11" << endl;
    out << "Min_Rate:   " << QString("%1").arg(configuration.minRate, 5) << " ; Minimum rate (Hz * 100)" << endl;
    out << "Max_Rate:   " << QString("%1").arg(configuration.maxRate, 5) << " ; Maximum rate (Hz * 100)" << endl;
    out << "Flatline:   " << QString("%1").arg(configuration.flatline, 5) << " ; Flatline at minimum rate" << endl;
    out << "                  ;   0 = No" << endl;
    out << "                  ;   1 = Yes" << endl << endl;

    out << "; Speech settings" << endl << endl;

    out << "Sp_Rate:    " << QString("%1").arg(configuration.speechRate, 5) << " ; Speech rate (s)" << endl;
    out << "                  ;   0 = No speech" << endl;
    out << "Sp_Volume:  " << QString("%1").arg(configuration.speechVolume, 5) << " ; 0 (min) to 8 (max)" << endl << endl;

    if (configuration.speeches.empty())
    {
        Configuration::Speech speech;
        speech.mode = Configuration::GlideRatio;
        speech.units = Configuration::Miles;
        speech.decimals = 1;
        saveSpeech(out, speech, true);
    }
    else
    {
        bool firstSpeech = true;
        foreach (Configuration::Speech speech, configuration.speeches)
        {
            saveSpeech(out, speech, firstSpeech);
            firstSpeech = false;
        }
    }

    out << "; Thresholds" << endl << endl;

    out << "V_Thresh:   " << QString("%1").arg(configuration.vThreshold, 5) << " ; Minimum vertical speed for tone (cm/s)" << endl;
    out << "H_Thresh:   " << QString("%1").arg(configuration.hThreshold, 5) << " ; Minimum horizontal speed for tone (cm/s)" << endl << endl;

    out << "; Miscellaneous" << endl << endl;

    out << "Use_SAS:    " << QString("%1").arg(configuration.adjustSpeed, 5) << " ; Use skydiver's airspeed" << endl;
    out << "                  ;   0 = No" << endl;
    out << "                  ;   1 = Yes" << endl;
    out << "TZ_Offset:  " << QString("%1").arg(configuration.timeZoneOffset, 5) << " ; Timezone offset of output files in seconds" << endl;
    out << "                  ;   -14400 = UTC-4 (EDT)" << endl;
    out << "                  ;   -18000 = UTC-5 (EST, CDT)" << endl;
    out << "                  ;   -21600 = UTC-6 (CST, MDT)" << endl;
    out << "                  ;   -25200 = UTC-7 (MST, PDT)" << endl;
    out << "                  ;   -28800 = UTC-8 (PST)" << endl << endl;

    out << "; Initialization" << endl << endl;

    out << "Init_Mode:  " << QString("%1").arg(configuration.initMode, 5) << " ; When the FlySight is powered on" << endl;
    out << "                  ;   0 = Do nothing" << endl;
    out << "                  ;   1 = Test speech mode" << endl;
    out << "                  ;   2 = Play file" << endl;
    out << "Init_File:  " << configuration.initFile.rightJustified(5) << " ; File to be played" << endl << endl;

    out << "; Alarm settings" << endl << endl;

    out << "; WARNING: GPS measurements depend on very weak signals" << endl;
    out << ";          received from orbiting satellites. As such, they" << endl;
    out << ";          are prone to interference, and should NEVER be" << endl;
    out << ";          relied upon for life saving purposes." << endl << endl;

    out << ";          UNDER NO CIRCUMSTANCES SHOULD THESE ALARMS BE" << endl;
    out << ";          USED TO INDICATE DEPLOYMENT OR BREAKOFF ALTITUDE." << endl << endl;

    out << "; NOTE:    Alarm elevations are given in meters above ground" << endl;
    out << ";          elevation, which is specified in DZ_Elev." << endl << endl;

    out << "Window:     " << QString("%1").arg(configuration.alarmWindowAbove, 5) << " ; Alarm window (m)" << endl;
    out << "Win_Above:  " << QString("%1").arg(configuration.alarmWindowAbove, 5) << " ; Alarm window (m)" << endl;
    out << "Win_Below:  " << QString("%1").arg(configuration.alarmWindowBelow, 5) << " ; Alarm window (m)" << endl;
    out << "DZ_Elev:    " << QString("%1").arg(configuration.groundElevation, 5) << " ; Ground elevation (m above sea level)" << endl << endl;

    if (configuration.alarms.empty())
    {
        Configuration::Alarm alarm;
        alarm.elevation = 0;
        alarm.mode = Configuration::NoAlarm;
        alarm.file = "0";
        saveAlarm(out, alarm, true);
    }
    else
    {
        bool firstAlarm = true;
        foreach (Configuration::Alarm alarm, configuration.alarms)
        {
            saveAlarm(out, alarm, firstAlarm);
            firstAlarm = false;
        }
    }

    out << "; Altitude mode settings" << endl << endl;

    out << "; WARNING: GPS measurements depend on very weak signals" << endl;
    out << ";          received from orbiting satellites. As such, they" << endl;
    out << ";          are prone to interference, and should NEVER be" << endl;
    out << ";          relied upon for life saving purposes." << endl << endl;

    out << ";          UNDER NO CIRCUMSTANCES SHOULD ALTITUDE MODE BE" << endl;
    out << ";          USED TO INDICATE DEPLOYMENT OR BREAKOFF ALTITUDE." << endl << endl;

    out << "; NOTE:    Altitude is given relative to ground elevation," << endl;
    out << ";          which is specified in DZ_Elev. Altitude mode will" << endl;
    out << ";          not function below 1500 m above ground." << endl << endl;;

    out << "Alt_Units:  " << QString("%1").arg(configuration.altitudeUnits, 5) << " ; Altitude units" << endl;
    out << "                  ;   0 = m" << endl;
    out << "                  ;   1 = ft" << endl;
    out << "Alt_Step:   " << QString("%1").arg(configuration.altitudeStep, 5) << " ; Altitude between announcements" << endl;
    out << "                  ;   0 = No altitude" << endl << endl;

    out << "; Silence windows" << endl << endl;

    out << "; NOTE:    Silence windows are given in meters above ground" << endl;
    out << ";          elevation, which is specified in DZ_Elev. Tones" << endl;
    out << ";          will be silenced during these windows and only" << endl;
    out << ";          alarms will be audible." << endl << endl;

    if (configuration.windows.empty())
    {
        Configuration::Window window;
        window.top = 0;
        window.bottom = 0;
        saveWindow(out, window);
    }
    else
    {
        foreach (Configuration::Window window, configuration.windows)
        {
            saveWindow(out, window);
        }
    }

    // Update file name
    setCurrentFile(fileName);

    return true;
}

void MainWindow::saveSpeech(
        QTextStream &out,
        const Configuration::Speech &speech,
        bool firstSpeech)
{
    out << "Sp_Mode:    " << QString("%1").arg(speech.mode, 5) << " ; Speech mode" << endl;
    if (firstSpeech)
    {
        out << "                  ;   0 = Horizontal speed" << endl;
        out << "                  ;   1 = Vertical speed" << endl;
        out << "                  ;   2 = Glide ratio" << endl;
        out << "                  ;   3 = Inverse glide ratio" << endl;
        out << "                  ;   4 = Total speed" << endl;
        out << "                  ;   5 = Altitude above DZ_Elev" << endl;
        out << "                  ;   11 = Dive angle" << endl;
    }
    out << "Sp_Units:   " << QString("%1").arg(speech.units, 5) << " ; Speech units" << endl;
    if (firstSpeech)
    {
        out << "                  ;   0 = km/h or m" << endl;
        out << "                  ;   1 = mph or feet" << endl;
    }
    out << "Sp_Dec:     " << QString("%1").arg(speech.decimals, 5) << " ; Speech precision" << endl;
    if (firstSpeech)
    {
        out << "                  ;   Altitude step in Mode 5" << endl;
        out << "                  ;   Decimal places in all other Modes" << endl;
    }
    out << endl;
}

void MainWindow::saveAlarm(
        QTextStream &out,
        const Configuration::Alarm &alarm,
        bool firstAlarm)
{
    out << "Alarm_Elev: " << QString("%1").arg(alarm.elevation, 5) << " ; Alarm elevation (m above ground level)" << endl;
    out << "Alarm_Type: " << QString("%1").arg(alarm.mode, 5) << " ; Alarm type" << endl;
    if (firstAlarm)
    {
        out << "                  ;   0 = No alarm" << endl;
        out << "                  ;   1 = Beep" << endl;
        out << "                  ;   2 = Chirp up" << endl;
        out << "                  ;   3 = Chirp down" << endl;
        out << "                  ;   4 = Play file" << endl;
    }
    out << "Alarm_File: " << alarm.file.rightJustified(5) << " ; File to be played" << endl << endl;
}

void MainWindow::saveWindow(
        QTextStream &out,
        const Configuration::Window window)
{
    out << "Win_Top:    " << QString("%1").arg(window.top, 5) << " ; Silence window top (m)" << endl;
    out << "Win_Bottom: " << QString("%1").arg(window.bottom, 5) << " ; Silence window bottom (m)" << endl << endl;
}

void MainWindow::setUnits(
        int units)
{
    if (configuration.displayUnits == (Configuration::DisplayUnits) units)
        return;

    // Update configuration from pages
    foreach(ConfigurationPage *page, pages)
    {
        page->updateConfiguration(configuration, ConfigurationPage::Values);
    }

    // Update display units
    configuration.displayUnits = (Configuration::DisplayUnits) units;
    ui->unitsComboBox->setCurrentIndex(units);

    // Update pages from configuration
    updatePages();
}

void MainWindow::updateConfigurationOptions()
{
    if (updating) return;

    // Update configuration from pages
    foreach(ConfigurationPage *page, pages)
    {
        page->updateConfiguration(configuration, ConfigurationPage::Values);
    }
    foreach(ConfigurationPage *page, pages)
    {
        page->updateConfiguration(configuration, ConfigurationPage::Options);
    }

    // Now update the configuration pages
    updatePages();
}

void MainWindow::updatePages()
{
    updating = true;

    // Update pages from configuration
    foreach(ConfigurationPage *page, pages)
    {
        page->setConfiguration(configuration);
    }

    updating = false;
}

void MainWindow::closeEvent(
        QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::on_actionNew_triggered()
{
    if (maybeSave())
    {
        // Reset configuration but keep units
        configuration = Configuration(configuration.displayUnits);

        // Update configuration
        updatePages();

        // Initialize file name
        setCurrentFile(QString());
    }
}

void MainWindow::on_actionOpen_triggered()
{
    if (maybeSave())
    {
        // Initialize settings object
        QSettings settings("FlySight", "Configurator");

        QString fileName = QFileDialog::getOpenFileName(
                    this,
                    tr("Open"),
                    settings.value("folder").toString(),
                    tr("Configuration files (*.txt)"));

        // Return now if user canceled
        if (!fileName.isEmpty())
        {
            // Open the configuration
            loadFile(fileName);
        }
    }
}

void MainWindow::on_actionSave_triggered()
{
    save();
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveAs();
}

void MainWindow::setCurrentFile(
        const QString &fileName)
{
    curFile = fileName;
    savedConfiguration = configuration;

    QString shownName = curFile;
    if (curFile.isEmpty())
    {
        shownName = "config.txt";
    }
    setWindowFilePath(shownName);
}

bool MainWindow::maybeSave()
{
    // Update configuration
    foreach(ConfigurationPage *page, pages)
    {
        page->updateConfiguration(configuration, ConfigurationPage::Values);
    }

    // Check if configuration has changed
    if (configuration == savedConfiguration) return true;

    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("FlySight Configurator"),
                               tr("The configuration has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}
