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

#include "solutiontypes.h"

#include "util/global.h"

#include "scene.h"
#include "field.h"
#include "block.h"
#include "problem.h"
#include "logview.h"

using namespace Hermes::Hermes2D;

FieldSolutionID BlockSolutionID::fieldSolutionID(const FieldInfo *fieldInfo)
{
    bool contains = false;
    foreach(FieldBlock* field, group->fields())
    {
        if(field->fieldInfo() == fieldInfo)
            contains = true;
    }
    assert(contains);

    return FieldSolutionID(fieldInfo, timeStep, adaptivityStep, solutionMode);
}

BlockSolutionID FieldSolutionID::blockSolutionID(const Block *block)
{
    assert(block->contains(this->group));
    return BlockSolutionID(block, timeStep, adaptivityStep, solutionMode);
}

QString FieldSolutionID::toString()
{
    QString str = QString("%1_%2_%3_%4").
            arg(group->fieldId()).
            arg(timeStep).
            arg(adaptivityStep).
            arg(solutionTypeToStringKey(solutionMode));

    return str;
}

// *********************************************************************************************

MultiArray::MultiArray()
{
}

MultiArray::~MultiArray()
{
    clear();
}

void MultiArray::clear()
{
    // m_doFHandler.reset();
    // m_solution.reset();
}

void MultiArray::append(std::shared_ptr<dealii::DoFHandler<2> > doFHandler,
                            std::shared_ptr<dealii::Vector<double> > solution)
{
    m_doFHandler = doFHandler;
    m_solution = solution;
}

MultiArray MultiArray::fieldPart(const Block *block, const FieldInfo *fieldInfo)
{
    assert(block->contains(fieldInfo));
    MultiArray msa;
    int offset = block->offset(block->field(fieldInfo));

    msa.append(m_doFHandler, m_solution);
    return msa;
}

template class SolutionID<FieldInfo>;
