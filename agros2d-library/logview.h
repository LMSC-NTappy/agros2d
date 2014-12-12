// This file is part of Agros.
//
// Agros is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Agros is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Agros.  If not, see <http://www.gnu.org/licenses/>.
//
//
// University of West Bohemia, Pilsen, Czech Republic
// Email: info@agros2d.org, home page: http://agros2d.org/

#ifndef TOOLTIPVIEW_H
#define TOOLTIPVIEW_H

#include "util.h"
#include "solver/solver.h"

class QCustomPlot;
class QCPGraph;
class FieldInfo;
class SolverAgros;

class AGROS_LIBRARY_API Log: public QObject
{
    Q_OBJECT
public:
    Log();

    inline void printHeading(const QString &message) { emit headingMsg(message); }
    inline void printMessage(const QString &module, const QString &message) { emit messageMsg(module, message); }
    inline void printError(const QString &module, const QString &message) { emit errorMsg(module, message); }
    inline void printWarning(const QString &module, const QString &message) { emit warningMsg(module, message); }
    inline void printDebug(const QString &module, const QString &message) { emit debugMsg(module, message); }

    inline void updateNonlinearChartInfo(SolverAgros::Phase phase, const QVector<double> steps, const QVector<double> relativeChangeOfSolutions) { emit updateNonlinearChart(phase, steps, relativeChangeOfSolutions); }
    inline void updateAdaptivityChartInfo(const FieldInfo *fieldInfo, int timeStep, int adaptivityStep) { emit updateAdaptivityChart(fieldInfo, timeStep, adaptivityStep); }
    inline void updateTransientChartInfo(double actualTime) { emit updateTransientChart(actualTime); }

    inline void addIcon(const QIcon &icn, const QString &label) { emit addIconImg(icn, label); }

signals:
    void headingMsg(const QString &message);
    void messageMsg(const QString &module, const QString &message);
    void errorMsg(const QString &module, const QString &message);
    void warningMsg(const QString &module, const QString &message);
    void debugMsg(const QString &module, const QString &message);

    void updateNonlinearChart(SolverAgros::Phase phase, const QVector<double> steps, const QVector<double> relativeChangeOfSolutions);
    void updateAdaptivityChart(const FieldInfo *fieldInfo, int timeStep, int adaptivityStep);
    void updateTransientChart(double actualTime);

    void addIconImg(const QIcon &icn, const QString &label);
};

class AGROS_LIBRARY_API LogWidget : public QWidget
{
    Q_OBJECT
public:
    LogWidget(QWidget *parent = 0);
    ~LogWidget();

    void welcomeMessage();

    bool isMemoryLabelVisible() const;
    void setMemoryLabelVisible(bool visible = true);

public slots:
    void clear();

protected:
    void print(const QString &module, const QString &message,
               const QString &color = "");

private:
    QMenu *mnuInfo;

    QPlainTextEdit *plainLog;
    QString m_cascadeStyleSheet;

    QAction *actShowTimestamp;
    QAction *actShowDebug;
    QAction *actClear;

    QLabel *memoryLabel;
    int m_printCounter;

    void createActions();

private slots:
    void contextMenu(const QPoint &pos);

    void printMessage(const QString &module, const QString &message);
    void printError(const QString &module, const QString &message);
    void printWarning(const QString &module, const QString &message);
    void printDebug(const QString &module, const QString &message);
    void printHeading(const QString &message);

    void showTimestamp();
    void showDebug();

    void refreshMemory(int usage);
};

class LogView : public QDockWidget
{
    Q_OBJECT
public:
    LogView(QWidget *parent = 0);

private:
    LogWidget *logWidget;
};

class AGROS_LIBRARY_API LogDialog : public QDialog
{
    Q_OBJECT
public:
    LogDialog(QWidget *parent = 0, const QString &title = tr("Progress..."));
    ~LogDialog();

protected:
    virtual void closeEvent(QCloseEvent *e);
    virtual void reject();

private:
    LogWidget *m_logWidget;

    QPushButton *btnClose;
    QPushButton *btnAbort;
    QPushButton *btnShowHide;

    QCustomPlot *m_nonlinearChart;
    QCPGraph *m_nonlinearErrorGraph;
    QProgressBar *m_nonlinearProgress;

    QCustomPlot *m_adaptivityChart;
    QCPGraph *m_adaptivityErrorGraph;
    QCPGraph *m_adaptivityDOFsGraph;
    QProgressBar *m_adaptivityProgress;

    QCustomPlot *m_timeChart;
    QCPGraph *m_timeTimeStepGraph;
    QCPGraph *m_timeTimeTotalGraph;
    QProgressBar *m_timeProgress;

    QListWidget *m_progress;

    void createControls();

private slots:    
    void printError(const QString &module, const QString &message);

    void updateNonlinearChartInfo(SolverAgros::Phase phase, const QVector<double> steps, const QVector<double> relativeChangeOfSolutions);
    void updateAdaptivityChartInfo(const FieldInfo *fieldInfo, int timeStep, int adaptivityStep);
    void updateTransientChartInfo(double actualTime);

    void addIcon(const QIcon &icn, const QString &label);

    void tryClose();
    void showHide();
};

class AGROS_LIBRARY_API LogStdOut : public QObject
{
    Q_OBJECT
public:
    LogStdOut(QWidget *parent = 0);

private slots:
    void printHeading(const QString &message);
    void printMessage(const QString &module, const QString &message);
    void printError(const QString &module, const QString &message);
    void printWarning(const QString &module, const QString &message);
    void printDebug(const QString &module, const QString &message);
};

#endif // TOOLTIPVIEW_H
