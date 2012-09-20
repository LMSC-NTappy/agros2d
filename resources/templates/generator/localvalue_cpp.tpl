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

#include "{{ID}}_localvalue.h"

#include "hermes2d/module.h"
#include "hermes2d/module_agros.h"
#include "hermes2d/field.h"
#include "hermes2d/block.h"
#include "hermes2d/problem.h"
#include "hermes2d/solutionstore.h"

#include "util.h"
#include "scene.h"
#include "scenelabel.h"
#include "logview.h"

{{CLASS}}LocalValue::{{CLASS}}LocalValue(FieldInfo *fieldInfo, const Point &point)
    : LocalValue(fieldInfo, point)
{
    calculate();
}

void {{CLASS}}LocalValue::calculate()
{
    m_values.clear();

    // update time functions
    if (m_fieldInfo->analysisType() == AnalysisType_Transient)
    {
        m_fieldInfo->module()->updateTimeFunctions(Util::problem()->timeStepToTime(Util::scene()->activeTimeStep()));
    }

    if (Util::problem()->isSolved())
    {
        int index = findElementInMesh(Util::problem()->meshInitial(m_fieldInfo), m_point);
        if (index != -1)
        {
            // find marker
            Hermes::Hermes2D::Element *e = Util::problem()->meshInitial(m_fieldInfo)->get_element_fast(index);
            SceneLabel *label = Util::scene()->labels->at(atoi(Util::problem()->meshInitial(m_fieldInfo)->get_element_markers_conversion().get_user_marker(e->marker).marker.c_str()));
            SceneMaterial *material = label->marker(m_fieldInfo);

            // set variables
            double x = m_point.x;
            double y = m_point.y;

            double *value = new double[m_fieldInfo->module()->numberOfSolutions()];
            double *dudx = new double[m_fieldInfo->module()->numberOfSolutions()];
            double *dudy = new double[m_fieldInfo->module()->numberOfSolutions()];

            std::vector<Hermes::Hermes2D::Solution<double> *> sln(m_fieldInfo->module()->numberOfSolutions());
            for (int k = 0; k < m_fieldInfo->module()->numberOfSolutions(); k++)
            {
                // todo: do it better! - I could use reference solution. This way I ignore selected active adaptivity step and solution mode
                int adaptivityStep = Util::scene()->activeAdaptivityStep();
                SolutionMode solutionMode = Util::scene()->activeSolutionType();
                int timeStep = Util::solutionStore()->nearestTimeStep(m_fieldInfo, Util::scene()->activeTimeStep());
                if(timeStep != Util::scene()->activeTimeStep())
                {
                    adaptivityStep = Util::solutionStore()->lastAdaptiveStep(m_fieldInfo, SolutionMode_Normal, timeStep);
                    solutionMode = SolutionMode_Normal;
                }
                FieldSolutionID fsid(m_fieldInfo, timeStep, adaptivityStep, solutionMode);
                sln[k] = Util::solutionStore()->multiSolution(fsid).component(k).sln.data();

                double val;
                if ((m_fieldInfo->analysisType() == AnalysisType_Transient) && Util::scene()->activeTimeStep() == 0)
                    // const solution at first time step
                    val = m_fieldInfo->initialCondition().number();
                else
                    val = sln[k]->get_pt_value(m_point.x, m_point.y, Hermes::Hermes2D::H2D_FN_VAL_0);

                Point derivative;
                derivative.x = sln[k]->get_pt_value(m_point.x, m_point.y, Hermes::Hermes2D::H2D_FN_DX_0);
                derivative.y = sln[k]->get_pt_value(m_point.x, m_point.y, Hermes::Hermes2D::H2D_FN_DY_0);

                // set variables
                value[k] = val;
                dudx[k] = derivative.x;
                dudy[k] = derivative.y;
            }

            // expressions
            {{#VARIABLE_SOURCE}}
            if ((m_fieldInfo->module()->analysisType() == {{ANALYSIS_TYPE}})
                    && (m_fieldInfo->module()->coordinateType() == {{COORDINATE_TYPE}}))
                m_values[m_fieldInfo->module()->localVariable("{{VARIABLE}}")] = PointValue({{EXPRESSION_SCALAR}}, Point({{EXPRESSION_VECTORX}}, {{EXPRESSION_VECTORY}}), material);
            {{/VARIABLE_SOURCE}}

            delete [] value;
            delete [] dudx;
            delete [] dudy;
        }
    }
}
