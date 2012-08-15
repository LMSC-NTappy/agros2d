// This file is part of Agros2D.
//
// Agros2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Agros2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Agros2D.  If not, see <http://www.gnu.org/licenses/>.
//
// hp-FEM group (http://hpfem.org/)
// University of Nevada, Reno (UNR) and University of West Bohemia, Pilsen
// Email: agros2d@googlegroups.com, home page: http://hpfem.org/agros2d/

#ifndef FIELD_H
#define FIELD_H

#include "util.h"
#include "sceneedge.h"
#include "scenelabel.h"

namespace Module
{
    struct ModuleAgros;
}

class ProblemConfig;
class CouplingInfo;
class LocalForceValue;

class FieldInfo : public QObject
{
    Q_OBJECT

public:
    FieldInfo(QString fieldId = "", const AnalysisType analysisType = AnalysisType_Undefined);
    ~FieldInfo();

    void clear();

    inline Module::ModuleAgros *module() const { assert(m_module); return m_module; }

    QString fieldId() const { return m_fieldId; }
    AnalysisType analysisType() const { return m_analysisType; }
    void setAnalysisType(const AnalysisType analysisType);

    inline LinearityType linearityType() const {return m_linearityType; }
    void setLinearityType(const LinearityType lt) { m_linearityType = lt; emit changed(); }

    inline double nonlinearTolerance() const { return m_nonlinearTolerance; }
    void setNonlinearTolerance(const double nt) { m_nonlinearTolerance = nt; emit changed(); }

    inline int nonlinearSteps() const { return m_nonlinearSteps; }
    void setNonlinearSteps(const int ns) { m_nonlinearSteps = ns; emit changed(); }

    inline int numberOfRefinements() const { return m_numberOfRefinements; }
    void setNumberOfRefinements(const int nr) {m_numberOfRefinements = nr; emit changed(); }

    int edgeRefinement(SceneEdge *edge);
    void setEdgeRefinement(SceneEdge *edge, int refinement) { edgesRefinement[edge] = refinement; }
    void removeEdgeRefinement(SceneEdge *edge) { edgesRefinement.remove(edge); }

    int labelRefinement(SceneLabel *label);
    void setLabelRefinement(SceneLabel *label, int refinement) { labelsRefinement[label] = refinement; }
    void removeLabelRefinement(SceneLabel *label) { labelsRefinement.remove(label); }

    int labelPolynomialOrder(SceneLabel *label);
    void setLabelPolynomialOrder(SceneLabel *label, int order) { labelsPolynomialOrder[label] = order; }
    void removeLabelPolynomialOrder(SceneLabel *label) { labelsPolynomialOrder.remove(label); }

    inline int polynomialOrder() const { return m_polynomialOrder; }
    void setPolynomialOrder(const int po) { m_polynomialOrder = po; emit changed(); }

    inline AdaptivityType adaptivityType() const { return m_adaptivityType; }
    void setAdaptivityType(const AdaptivityType at) { m_adaptivityType = at; emit changed(); }

    inline int adaptivitySteps() const { return m_adaptivitySteps; }
    void setAdaptivitySteps(const int as) { m_adaptivitySteps = as; emit changed(); }

    inline double adaptivityTolerance() const { return m_adaptivityTolerance; }
    void setAdaptivityTolerance(const double at) { m_adaptivityTolerance = at; emit changed(); }

    inline Value initialCondition() const { return m_initialCondition; }
    void setInitialCondition(const Value& value) { m_initialCondition = value; emit changed(); }

    inline Value timeStepsSkip() const { return m_timeStepsSkip; }
    void setTimeStepsSkip(const Value& value) { m_timeStepsSkip = value; emit changed(); }

    inline WeakFormsType weakFormsType() const { return m_weakFormsType; }
    void setWeakFormsType(const WeakFormsType wft) { m_weakFormsType = wft; emit changed(); }

    inline LocalForceValue *forceValue() { return m_forceValue; }

    // module
    QString name();
    // description
    QString description();

signals:
    void changed();

private:
    /// module
    Module::ModuleAgros *m_module;

    /// pointer to problem info, whose this object is a "subfield"
    ProblemConfig *m_parent;

    /// unique field info
    QString m_fieldId;

    /// field force calculation
    LocalForceValue *m_forceValue;

    // analysis type
    AnalysisType m_analysisType;

    // linearity
    LinearityType m_linearityType;
    double m_nonlinearTolerance; // percent
    int m_nonlinearSteps;

    int m_numberOfRefinements;
    int m_polynomialOrder;

    // TODO: (Franta) gmsh
    QMap<SceneEdge *, int> edgesRefinement;
    QMap<SceneLabel *, int> labelsRefinement;

    QMap<SceneLabel *, int> labelsPolynomialOrder;

    AdaptivityType m_adaptivityType;
    int m_adaptivitySteps;
    double m_adaptivityTolerance; // percent

    // transient
    Value m_initialCondition;
    Value m_timeStepsSkip;

    // weakforms
    WeakFormsType m_weakFormsType;
};

ostream& operator<<(ostream& output, FieldInfo& id);

class Field
{
public:
    Field(FieldInfo* fieldInfo);
    bool solveInitVariables();
    FieldInfo* fieldInfo() { return m_fieldInfo; }

    // mesh
    void setMeshInitial(Hermes::Hermes2D::Mesh *meshInitial);

public:
//private:
    QList<CouplingInfo* > m_couplingSources;
    FieldInfo* m_fieldInfo;

};

#endif // FIELD_H
