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

#ifndef SOLUTIONSTORE_H
#define SOLUTIONSTORE_H

#include "solutiontypes.h"

class AGROS_LIBRARY_API SolutionStore
{
public:
    ~SolutionStore();

    class SolutionRunTimeDetails
    {
    public:
        class FileName
        {
        public:
            FileName(QString meshFileName = "", QString doFFileName = "", QString solutionFileName = "")
                : m_meshFileName(meshFileName), m_DoFFileName(doFFileName), m_solutionFileName(solutionFileName) {}

            inline QString meshFileName() const { return m_meshFileName; }
            inline void setMeshFileName(const QString &value) { m_meshFileName = value; }
            inline QString doFFileName() const { return m_DoFFileName; }
            inline void setDoFFileName(const QString &value) { m_DoFFileName = value; }
            inline QString solutionFileName() const { return m_solutionFileName; }
            inline void setSolutionFileName(const QString &value) { m_solutionFileName = value; }

        private:
            QString m_meshFileName;
            QString m_DoFFileName;
            QString m_solutionFileName;
        };

        SolutionRunTimeDetails(double time_step_length = 0, double error = 0, int DOFs = 0)
            : m_timeStepLength(time_step_length), m_adaptivityError(error), m_DOFs(DOFs) {}
        ~SolutionRunTimeDetails()
        {
            m_relativeChangeOfSolutions.clear();
            m_newtonResidual.clear();
            m_nonlinearDamping.clear();
        }

        inline double timeStepLength() const { return m_timeStepLength; }
        inline void setTimeStepLength(double value) { m_timeStepLength = value; }
        inline double adaptivityError() const { return m_adaptivityError; }
        inline void setAdaptivityError(double value) { m_adaptivityError = value; }
        inline int DOFs() const { return m_DOFs; }
        inline void setDOFs(int value) { m_DOFs = value; }
        inline int jacobianCalculations() const { return m_jacobianCalculations; }
        inline void setJacobianCalculations(int value) { m_jacobianCalculations = value; }
        inline FileName fileNames() const { return m_fileNames; }
        inline void setFileNames(FileName value) { m_fileNames = value; }
        inline QVector<double> relativeChangeOfSolutions() const { return m_relativeChangeOfSolutions; }
        inline void setRelativeChangeOfSolutions(QVector<double> value) { m_relativeChangeOfSolutions = value; }
        inline QVector<double> newtonResidual() const { return m_newtonResidual; }
        inline void setNewtonResidual(QVector<double> value) { m_newtonResidual = value; }
        inline QVector<double> nonlinearDamping() const { return m_nonlinearDamping; }
        inline void setNonlinearDamping(QVector<double> value) { m_nonlinearDamping = value; }

    private:
        double m_timeStepLength;
        double m_adaptivityError;
        int m_DOFs;
        int m_jacobianCalculations;

        FileName m_fileNames;

        QVector<double> m_relativeChangeOfSolutions;
        QVector<double> m_newtonResidual;
        QVector<double> m_nonlinearDamping;
    };

    bool contains(FieldSolutionID solutionID) const;
    MultiArray multiArray(FieldSolutionID solutionID);

    // returns MultiSolution with components related to last time step, in which was each respective field calculated
    // this time step can be different for respective fields due to time step skipping
    // intented to be used as initial condition for the newton method
    // MultiArray<double> multiSolutionPreviousCalculatedTS(BlockSolutionID solutionID);

    void addSolution(FieldSolutionID solutionID, MultiArray multiArray, SolutionRunTimeDetails runTime);
    void removeSolution(FieldSolutionID solutionID, bool saveRunTime = true);

    // removes all solutions with the given time step
    void removeTimeStep(int timeStep);

    int lastTimeStep(const FieldInfo* fieldInfo, SolutionMode solutionType) const;

    // finds nearest smaller(or equal) time step, where this fieldInfo was calculated
    int nearestTimeStep(const FieldInfo* fieldInfo, int timeStep) const;

    // finds nth calculated time step for the given field
    int nthCalculatedTimeStep(const FieldInfo* fieldInfo, int n) const;

    double lastTime(const FieldInfo* fieldInfo);

    // last adaptive step for given time step. If time step not given, last time step used implicitly
    int lastAdaptiveStep(const FieldInfo* fieldInfo, SolutionMode solutionType, int timeStep = -1) const;

    QList<double> timeLevels(const FieldInfo* fieldInfo) const;

    // number of time steps, where this fieldInfo was calculated up to this time
    int timeLevelIndex(const FieldInfo* fieldInfo, double time);
    double timeLevel(const FieldInfo* fieldInfo, int timeLevelIndex);

    FieldSolutionID lastTimeAndAdaptiveSolution(const FieldInfo* fieldInfo, SolutionMode solutionType);

    void loadRunTimeDetails();

    SolutionRunTimeDetails multiSolutionRunTimeDetail(FieldSolutionID solutionID) const { assert(m_multiSolutionRunTimeDetails.contains(solutionID)); return m_multiSolutionRunTimeDetails[solutionID]; }
    void multiSolutionRunTimeDetailReplace(FieldSolutionID solutionID, SolutionRunTimeDetails runTime);

    inline bool isEmpty() const { return m_multiSolutions.isEmpty(); }
    void clearAll();

    void printDebugCacheStatus();

private:
    QList<FieldSolutionID> m_multiSolutions;
    QMap<FieldSolutionID, SolutionRunTimeDetails> m_multiSolutionRunTimeDetails;
    QMap<FieldSolutionID, MultiArray> m_multiSolutionDealCache;
    QList<FieldSolutionID> m_multiSolutionCacheIDOrder;

    void insertMultiSolutionToCache(FieldSolutionID solutionID, MultiArray multiArray);

    QString baseStoreFileName(FieldSolutionID solutionID) const;

    void saveRunTimeDetails();
};

#endif // SOLUTIONSTORE_H
