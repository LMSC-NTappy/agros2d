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

#include "hermes_magnetic.h"
#include "scene.h"

struct MagneticEdge
{
    PhysicFieldBC type;
    double value_real;
    double value_imag;
};

struct MagneticLabel
{
    double current_density_real;
    double current_density_imag;
    double permeability;
    double conductivity;
    double remanence;
    double remanence_angle;
    double velocity_x;
    double velocity_y;
    double velocity_angular;
};

MagneticEdge *magneticEdge;
MagneticLabel *magneticLabel;
bool magneticPlanar;
AnalysisType magneticAnalysisType;

double magneticInitialCondition;
double magneticTimeStep;
double magneticTimeTotal;
double magneticFrequency;

int magnetic_bc_types(int marker)
{
    switch (magneticEdge[marker].type)
    {
    case PHYSICFIELDBC_NONE:
        return BC_NONE;
    case PHYSICFIELDBC_MAGNETIC_VECTOR_POTENTIAL:
        return BC_ESSENTIAL;
    case PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT:
        return BC_NATURAL;
    }
}

scalar magnetic_bc_values_real(int marker, double x, double y)
{
    return magneticEdge[marker].value_real;
}

scalar magnetic_bc_values_imag(int marker, double x, double y)
{
    return magneticEdge[marker].value_imag;
}

template<typename Real, typename Scalar>
Scalar magnetic_linear_form_surf_real(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    double K = 0.0;

    if (magneticEdge[e->marker].type == PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT)
        K = magneticEdge[e->marker].value_real;

    if (magneticPlanar)
        return K * int_v<Real, Scalar>(n, wt, v);
    else
        return K * 2 * M_PI * int_x_v<Real, Scalar>(n, wt, v, e);
}

template<typename Real, typename Scalar>
Scalar magnetic_linear_form_surf_imag(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    double K = 0.0;

    if (magneticEdge[e->marker].type == PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT)
        K = magneticEdge[e->marker].value_imag;

    if (magneticPlanar)
        return K * int_v<Real, Scalar>(n, wt, v);
    else
        return K * 2 * M_PI * int_x_v<Real, Scalar>(n, wt, v, e);
}

template<typename Real, typename Scalar>
Scalar magnetic_bilinear_form_real_real(int n, double *wt, Func<Real> *u, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    if (magneticPlanar)
        return 1.0 / (magneticLabel[e->marker].permeability * MU0) * int_grad_u_grad_v<Real, Scalar>(n, wt, u, v) -
               magneticLabel[e->marker].conductivity * int_velocity<Real, Scalar>(n, wt, u, v, e, magneticLabel[e->marker].velocity_x, magneticLabel[e->marker].velocity_y, magneticLabel[e->marker].velocity_angular) +
               ((magneticAnalysisType == ANALYSISTYPE_TRANSIENT) ? magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, u, v) / magneticTimeStep : 0.0);

    else
        return 1.0 / (magneticLabel[e->marker].permeability * MU0) * (int_u_dvdx_over_x<Real, Scalar>(n, wt, u, v, e) + int_grad_u_grad_v<Real, Scalar>(n, wt, u, v)) -
                magneticLabel[e->marker].conductivity * int_velocity<Real, Scalar>(n, wt, u, v, e, magneticLabel[e->marker].velocity_x, magneticLabel[e->marker].velocity_y, magneticLabel[e->marker].velocity_angular) +
                ((magneticAnalysisType == ANALYSISTYPE_TRANSIENT) ? magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, u, v) / magneticTimeStep : 0.0);
}

template<typename Real, typename Scalar>
Scalar magnetic_bilinear_form_real_imag(int n, double *wt, Func<Real> *u, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    if (magneticPlanar)
        return - 2 * M_PI * magneticFrequency * magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, u, v);
    else
        return - 2 * M_PI * magneticFrequency * magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, u, v);
}

template<typename Real, typename Scalar>
Scalar magnetic_bilinear_form_imag_real(int n, double *wt, Func<Real> *u, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    if (magneticPlanar)
        return + 2 * M_PI * magneticFrequency * magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, u, v);
    else
        return + 2 * M_PI * magneticFrequency * magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, u, v);
}

template<typename Real, typename Scalar>
Scalar magnetic_bilinear_form_imag_imag(int n, double *wt, Func<Real> *u, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    if (magneticPlanar)
        return 1.0 / (magneticLabel[e->marker].permeability * MU0) * int_grad_u_grad_v<Real, Scalar>(n, wt, u, v) -
               magneticLabel[e->marker].conductivity * int_velocity<Real, Scalar>(n, wt, u, v, e, magneticLabel[e->marker].velocity_x, magneticLabel[e->marker].velocity_y, magneticLabel[e->marker].velocity_angular);
    else
        return 1.0 / (magneticLabel[e->marker].permeability * MU0) * (int_u_dvdx_over_x<Real, Scalar>(n, wt, u, v, e) + int_grad_u_grad_v<Real, Scalar>(n, wt, u, v)) -
               magneticLabel[e->marker].conductivity * int_velocity<Real, Scalar>(n, wt, u, v, e, magneticLabel[e->marker].velocity_x, magneticLabel[e->marker].velocity_y, magneticLabel[e->marker].velocity_angular);
}

template<typename Real, typename Scalar>
Scalar magnetic_linear_form_real(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    if (magneticPlanar)
        return magneticLabel[e->marker].current_density_real * int_v<Real, Scalar>(n, wt, v) +
               magneticLabel[e->marker].remanence / (magneticLabel[e->marker].permeability * MU0) * int_magnet<Real, Scalar>(n, wt, v, magneticLabel[e->marker].remanence_angle) +
                ((magneticAnalysisType == ANALYSISTYPE_TRANSIENT) ? magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, ext->fn[0], v) / magneticTimeStep : 0.0);
    else
        return (magneticLabel[e->marker].current_density_real * int_v<Real, Scalar>(n, wt, v) -
                magneticLabel[e->marker].remanence / (magneticLabel[e->marker].permeability * MU0) * int_magnet<Real, Scalar>(n, wt, v, magneticLabel[e->marker].remanence_angle) +
                ((magneticAnalysisType == ANALYSISTYPE_TRANSIENT) ? magneticLabel[e->marker].conductivity * int_u_v<Real, Scalar>(n, wt, ext->fn[0], v) / magneticTimeStep : 0.0));
}

template<typename Real, typename Scalar>
Scalar magnetic_linear_form_imag(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
    if (magneticPlanar)
        return magneticLabel[e->marker].current_density_imag * int_v<Real, Scalar>(n, wt, v);
    else
        return magneticLabel[e->marker].current_density_imag * int_v<Real, Scalar>(n, wt, v);
}

QList<SolutionArray *> *magnetic_main(SolverDialog *solverDialog)
{
    int numberOfRefinements = Util::scene()->problemInfo()->numberOfRefinements;
    int polynomialOrder = Util::scene()->problemInfo()->polynomialOrder;
    AdaptivityType adaptivityType = Util::scene()->problemInfo()->adaptivityType;
    int adaptivitySteps = Util::scene()->problemInfo()->adaptivitySteps;
    double adaptivityTolerance = Util::scene()->problemInfo()->adaptivityTolerance;

    magneticPlanar = (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR);

    magneticAnalysisType = Util::scene()->problemInfo()->analysisType;
    magneticTimeStep = Util::scene()->problemInfo()->timeStep.number;
    magneticTimeTotal = Util::scene()->problemInfo()->timeTotal.number;
    magneticInitialCondition = Util::scene()->problemInfo()->initialCondition.number;

    magneticFrequency = (magneticAnalysisType == ANALYSISTYPE_HARMONIC) ? Util::scene()->problemInfo()->frequency : 0.0;

    // save locale
    char *plocale = setlocale (LC_NUMERIC, "");
    setlocale (LC_NUMERIC, "C");

    int ndof;

    // load the mesh file
    Mesh mesh;
    H2DReader meshloader;
    meshloader.load((tempProblemFileName() + ".mesh").toStdString().c_str(), &mesh);
    for (int i = 0; i < numberOfRefinements; i++)
        mesh.refine_all_elements(0);

    // set system locale
    setlocale(LC_NUMERIC, plocale);

    if (magneticAnalysisType == ANALYSISTYPE_HARMONIC)
    {
        // initialize the shapeset and the cache
        H1Shapeset shapeset;
        PrecalcShapeset pssreal(&shapeset);
        PrecalcShapeset pssimag(&shapeset);

        // create the x displacement space
        H1Space spacereal(&mesh, &shapeset);
        spacereal.set_bc_types(magnetic_bc_types);
        spacereal.set_bc_values(magnetic_bc_values_real);
        spacereal.set_uniform_order(polynomialOrder);
        ndof = spacereal.assign_dofs(0);

        // create the y displacement space
        H1Space spaceimag(&mesh, &shapeset);
        spaceimag.set_bc_types(magnetic_bc_types);
        spaceimag.set_bc_values(magnetic_bc_values_imag);
        spaceimag.set_uniform_order(polynomialOrder);

        // initialize the weak formulation
        WeakForm wf(2);
        wf.add_biform(0, 0, callback(magnetic_bilinear_form_real_real));
        wf.add_biform(0, 1, callback(magnetic_bilinear_form_real_imag));
        wf.add_biform(1, 0, callback(magnetic_bilinear_form_imag_real));
        wf.add_biform(1, 1, callback(magnetic_bilinear_form_imag_imag));
        wf.add_liform(0, callback(magnetic_linear_form_real));
        wf.add_liform(1, callback(magnetic_linear_form_imag));
        wf.add_liform_surf(0, callback(magnetic_linear_form_surf_real));
        wf.add_liform_surf(1, callback(magnetic_linear_form_surf_imag));

        // initialize the linear solver
        UmfpackSolver umfpack;
        Solution *slnreal = new Solution();
        Solution *slnimag = new Solution();
        Solution rslnreal, rslnimag;

        // initialize the linear system
        LinSystem sys(&wf, &umfpack);
        sys.set_spaces(2, &spacereal, &spaceimag);
        sys.set_pss(2, &pssreal, &pssimag);

        // output
        SolutionArray *solutionArray;
        QList<SolutionArray *> *solutionArrayList = new QList<SolutionArray *>();

        // assemble the stiffness matrix and solve the system
        double error;
        int i;
        int adaptivitysteps = (adaptivityType == ADAPTIVITYTYPE_NONE) ? 1 : adaptivitySteps;
        for (i = 0; i<(adaptivitysteps); i++)
        {
            int ndof = spacereal.assign_dofs(0);
            spaceimag.assign_dofs(ndof);

            sys.assemble();
            if (sys.get_num_dofs() == 0)
            {
                solverDialog->showMessage(QObject::tr("Solver: DOF is zero."), true);
                return solutionArrayList;
            }
            sys.solve(2, slnreal, slnimag);

            // calculate errors and adapt the solution
            if (adaptivityType != ADAPTIVITYTYPE_NONE)
            {
                RefSystem rs(&sys);
                rs.assemble();
                rs.solve(2, &rslnreal, &rslnimag);

                H1OrthoHP hp(2, &spacereal, &spaceimag);
                error = hp.calc_error_2(slnreal, slnimag, &rslnreal, &rslnimag) * 100;

                // emit signal
                solverDialog->showMessage(QObject::tr("Solver: relative error: %1 %").arg(error, 0, 'f', 5), false);
                if (solverDialog->isCanceled()) return solutionArrayList;

                if (error < adaptivityTolerance || sys.get_num_dofs() >= NDOF_STOP) break;
                if (i != adaptivitysteps-1) hp.adapt(0.3, 0, (int) adaptivityType);
            }
        }

        // real part
        solutionArray = new SolutionArray();
        solutionArray->order = new Orderizer();
        solutionArray->order->process_solution(&spacereal);
        solutionArray->sln = slnreal;
        solutionArray->adaptiveError = error;
        solutionArray->adaptiveSteps = i-1;

        solutionArrayList->append(solutionArray);

        // iamg part
        solutionArray = new SolutionArray();
        solutionArray->order = new Orderizer();
        solutionArray->order->process_solution(&spaceimag);
        solutionArray->sln = slnimag;
        solutionArray->adaptiveError = error;
        solutionArray->adaptiveSteps = i-1;

        solutionArrayList->append(solutionArray);

        return solutionArrayList;
    }

    if (magneticAnalysisType == ANALYSISTYPE_STEADYSTATE ||
        magneticAnalysisType == ANALYSISTYPE_TRANSIENT)
    {
        // initialize the shapeset and the cache
        H1Shapeset shapeset;
        PrecalcShapeset pss(&shapeset);

        // create an H1 space
        H1Space space(&mesh, &shapeset);
        space.set_bc_types(magnetic_bc_types);
        space.set_bc_values(magnetic_bc_values_real);
        space.set_uniform_order(polynomialOrder);
        space.assign_dofs();

        // solution
        QList<SolutionArray *> *solutionArrayList = new QList<SolutionArray *>();

        Solution *sln = new Solution();
        if (magneticAnalysisType == ANALYSISTYPE_TRANSIENT)
        {
            sln->set_const(&mesh, magneticInitialCondition);

            // zero time
            SolutionArray *solutionArray = new SolutionArray();
            solutionArray->order = new Orderizer();
            solutionArray->sln = new Solution();
            solutionArray->sln->copy(sln);
            solutionArray->adaptiveError = 0.0;
            solutionArray->adaptiveSteps = 0.0;
            solutionArray->time = 0.0;

            solutionArrayList->append(solutionArray);
        }
        Solution rsln;

        // initialize the weak formulation
        WeakForm wf(1);
        wf.add_biform(0, 0, callback(magnetic_bilinear_form_real_real));
        if (magneticAnalysisType == ANALYSISTYPE_TRANSIENT)
            wf.add_liform(0, callback(magnetic_linear_form_real), ANY, 1, sln);
        else
            wf.add_liform(0, callback(magnetic_linear_form_real));
        wf.add_liform_surf(0, callback(magnetic_linear_form_surf_real));

        // initialize the linear solver
        UmfpackSolver umfpack;

        // initialize the linear system
        LinSystem sys(&wf, &umfpack);
        sys.set_spaces(1, &space);
        sys.set_pss(1, &pss);

        // assemble the stiffness matrix and solve the system
        int i;
        double error;

        // adaptivity
        int adaptivitysteps = (adaptivityType == ADAPTIVITYTYPE_NONE) ? 1 : adaptivitySteps;
        for (i = 0; i<adaptivitysteps; i++)
        {
            space.assign_dofs();

            sys.assemble();
            sys.solve(1, sln);

            // calculate errors and adapt the solution
            if (adaptivityType != ADAPTIVITYTYPE_NONE)
            {
                RefSystem rs(&sys);
                rs.assemble();
                rs.solve(1, &rsln);

                H1OrthoHP hp(1, &space);
                error = hp.calc_error(sln, &rsln) * 100;

                // emit signal
                solverDialog->showMessage(QObject::tr("Solver: relative error: %1 %").arg(error, 0, 'f', 5), false);
                if (solverDialog->isCanceled())
                {
                    solutionArrayList->clear();
                    return solutionArrayList;
                }

                if (error < adaptivityTolerance || sys.get_num_dofs() >= NDOF_STOP) break;
                if (i != adaptivitysteps-1) hp.adapt(0.3, 0, (int) adaptivityType);
            }
        }

        // timesteps
        int timesteps = (magneticAnalysisType == ANALYSISTYPE_TRANSIENT) ? floor(magneticTimeTotal/magneticTimeStep) : 1;
        for (int n = 0; n<timesteps; n++)
        {
            if (timesteps > 1)
            {
                sys.assemble(true);
                sys.solve(1, sln);
            }
            else if (n > 0)
            {
                space.assign_dofs();
                sys.assemble();
            }

            // output
            SolutionArray *solutionArray = new SolutionArray();
            solutionArray->order = new Orderizer();
            solutionArray->order->process_solution(&space);
            solutionArray->sln = new Solution();
            solutionArray->sln->copy(sln);
            solutionArray->adaptiveError = error;
            solutionArray->adaptiveSteps = i-1;
            if (magneticAnalysisType == ANALYSISTYPE_TRANSIENT) solutionArray->time = (n+1)*magneticTimeStep;

            solutionArrayList->append(solutionArray);

            if (magneticAnalysisType == ANALYSISTYPE_TRANSIENT) solverDialog->showMessage(QObject::tr("Solver: time step: %1/%2").arg(n+1).arg(timesteps), false);
            if (solverDialog->isCanceled())
            {
                solutionArrayList->clear();
                return solutionArrayList;
            }
            solverDialog->showProgress((int) (60.0 + 40.0*(n+1)/timesteps));
        }

        return solutionArrayList;
    }
}

// *******************************************************************************************************

int HermesMagnetic::numberOfSolution()
{
    return (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC) ? 2 : 1;
}

PhysicFieldVariable HermesMagnetic::contourPhysicFieldVariable()
{
    return (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC) ?
            PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL : PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_REAL;
}

PhysicFieldVariable HermesMagnetic::scalarPhysicFieldVariable()
{
    return (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC) ?
            PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY : PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL;
}

PhysicFieldVariableComp HermesMagnetic::scalarPhysicFieldVariableComp()
{
    return (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC) ?
            PHYSICFIELDVARIABLECOMP_MAGNITUDE : PHYSICFIELDVARIABLECOMP_MAGNITUDE;
}

PhysicFieldVariable HermesMagnetic::vectorPhysicFieldVariable()
{
    return (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC) ?
            PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL : PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL;
}


void HermesMagnetic::readEdgeMarkerFromDomElement(QDomElement *element)
{
    PhysicFieldBC type = physicFieldBCFromStringKey(element->attribute("type"));
    switch (type)
    {
    case PHYSICFIELDBC_NONE:
    case PHYSICFIELDBC_MAGNETIC_VECTOR_POTENTIAL:
    case PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT:
        Util::scene()->addEdgeMarker(new SceneEdgeMagneticMarker(element->attribute("name"),
                                                                 type,
                                                                 Value(element->attribute("value_real", "0")),
                                                                 Value(element->attribute("value_imag", "0"))));
        break;
    default:
        std::cerr << tr("Boundary type '%1' doesn't exists.").arg(element->attribute("type")).toStdString() << endl;
        break;
    }
}

void HermesMagnetic::writeEdgeMarkerToDomElement(QDomElement *element, SceneEdgeMarker *marker)
{
    SceneEdgeMagneticMarker *edgeMagneticMarker = dynamic_cast<SceneEdgeMagneticMarker *>(marker);

    element->setAttribute("type", physicFieldBCToStringKey(edgeMagneticMarker->type));
    element->setAttribute("value_real", edgeMagneticMarker->value_real.text);
    element->setAttribute("value_imag", edgeMagneticMarker->value_imag.text);
}

void HermesMagnetic::readLabelMarkerFromDomElement(QDomElement *element)
{
    Util::scene()->addLabelMarker(new SceneLabelMagneticMarker(element->attribute("name"),
                                                               Value(element->attribute("current_density_real", "0")),
                                                               Value(element->attribute("current_density_imag", "0")),
                                                               Value(element->attribute("permeability", "1")),
                                                               Value(element->attribute("conductivity", "0")),
                                                               Value(element->attribute("remanence", "0")),
                                                               Value(element->attribute("remanence_angle", "0")),
                                                               Value(element->attribute("velocity_x", "0")),
                                                               Value(element->attribute("velocity_y", "0")),
                                                               Value(element->attribute("velocity_angular", "0"))));
}

void HermesMagnetic::writeLabelMarkerToDomElement(QDomElement *element, SceneLabelMarker *marker)
{
    SceneLabelMagneticMarker *labelMagneticMarker = dynamic_cast<SceneLabelMagneticMarker *>(marker);

    element->setAttribute("current_density_real", labelMagneticMarker->current_density_real.text);
    element->setAttribute("current_density_imag", labelMagneticMarker->current_density_imag.text);
    element->setAttribute("permeability", labelMagneticMarker->permeability.text);
    element->setAttribute("conductivity", labelMagneticMarker->conductivity.text);
    element->setAttribute("remanence", labelMagneticMarker->remanence.text);
    element->setAttribute("remanence_angle", labelMagneticMarker->remanence_angle.text);
    element->setAttribute("velocity_x", labelMagneticMarker->velocity_x.text);
    element->setAttribute("velocity_y", labelMagneticMarker->velocity_y.text);
    element->setAttribute("velocity_angular", labelMagneticMarker->velocity_angular.text);}

LocalPointValue *HermesMagnetic::localPointValue(Point point)
{
    return new LocalPointValueMagnetic(point);
}

QStringList HermesMagnetic::localPointValueHeader()
{
    QStringList headers;
    headers << "X" << "Y" << "A_real" << "A_imag" << "A"
            << "B" << "Bx_real" << "By_real" << "B_real" << "Bx_imag" << "By_imag" << "B_imag"
            << "H" << "Hx_real" << "Hy_real" << "H_real" << "Hx_imag" << "Hy_imag" << "H_imag"
            << "Je_real" << "Je_imag" << "Je"
            << "Jit_real" << "Jit_imag" << "Jit"
            << "Jiv_real" << "Jiv_imag" << "Jiv"
            << "J_real" << "J_imag" << "J"
            << "pj" << "wm" << "mur" << "gamma" << "Br" << "Brangle" << "vx" << "vy"
            << "Fx_real" << "Fy_real" << "Fx_imag" << "Fy_imag";
    return QStringList(headers);
}

SurfaceIntegralValue *HermesMagnetic::surfaceIntegralValue()
{
    return new SurfaceIntegralValueMagnetic();
}

QStringList HermesMagnetic::surfaceIntegralValueHeader()
{
    QStringList headers;
    headers << "l" << "S" << "Fx" << "Fy";
    return QStringList(headers);
}

VolumeIntegralValue *HermesMagnetic::volumeIntegralValue()
{
    return new VolumeIntegralValueMagnetic();
}

QStringList HermesMagnetic::volumeIntegralValueHeader()
{
    QStringList headers;
    headers << "V" << "S" << "Ie_real" << "Ie_imag" << "Iit_real" << "Iit_imag" << "Iiv_real" << "Iiv_imag" << "I_real" << "I_imag"
            << "Fx" << "Fy" << "T" << "Pj" << "Wm";
    return QStringList(headers);
}

SceneEdgeMarker *HermesMagnetic::newEdgeMarker()
{    
    return new SceneEdgeMagneticMarker("new boundary",
                                       PHYSICFIELDBC_MAGNETIC_VECTOR_POTENTIAL,
                                       Value("0"),
                                       Value("0"));
}

SceneEdgeMarker *HermesMagnetic::newEdgeMarker(PyObject *self, PyObject *args)
{
    double value_real;
    double value_imag = 0.0;
    char *name, *type;
    if (PyArg_ParseTuple(args, "ssd|d", &name, &type, &value_real, &value_imag))
    {
        // check name
        if (Util::scene()->getEdgeMarker(name)) return NULL;

        return new SceneEdgeMagneticMarker(name,
                                           physicFieldBCFromStringKey(type),
                                           Value(QString::number(value_real)),
                                           Value(QString::number(value_imag)));
    }

    return NULL;
}

SceneLabelMarker *HermesMagnetic::newLabelMarker()
{
    return new SceneLabelMagneticMarker("new material",
                                        Value("0"),
                                        Value("0"),
                                        Value("1"),
                                        Value("0"),
                                        Value("0"),
                                        Value("0"),
                                        Value("0"),
                                        Value("0"),
                                        Value("0"));
}

SceneLabelMarker *HermesMagnetic::newLabelMarker(PyObject *self, PyObject *args)
{
    double current_density_real, current_density_imag, permeability, conductivity, remanence, remanence_angle, velocity_x, velocity_y, velocity_angular;
    char *name;
    if (PyArg_ParseTuple(args, "sddddddddd", &name, &current_density_real, &current_density_imag, &permeability, &conductivity, &remanence, &remanence_angle, &velocity_x, &velocity_y, &velocity_angular))
    {
        // check name
        if (Util::scene()->getLabelMarker(name)) return NULL;

        return new SceneLabelMagneticMarker(name,
                                            Value(QString::number(current_density_real)),
                                            Value(QString::number(current_density_imag)),
                                            Value(QString::number(permeability)),
                                            Value(QString::number(conductivity)),
                                            Value(QString::number(remanence)),
                                            Value(QString::number(remanence_angle)),
                                            Value(QString::number(velocity_x)),
                                            Value(QString::number(velocity_y)),
                                            Value(QString::number(velocity_angular)));
    }

    return NULL;
}

void HermesMagnetic::fillComboBoxScalarVariable(QComboBox *cmbFieldVariable)
{
    // harmonic
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL), PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_REAL), PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL), PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD), PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL), PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_REAL), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_REAL), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_REAL), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_REAL), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_IMAG), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_IMAG);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_POWER_LOSSES), PHYSICFIELDVARIABLE_MAGNETIC_POWER_LOSSES);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE), PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_ENERGY_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_ENERGY_DENSITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_PERMEABILITY), PHYSICFIELDVARIABLE_MAGNETIC_PERMEABILITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CONDUCTIVITY), PHYSICFIELDVARIABLE_MAGNETIC_CONDUCTIVITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY), PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE), PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE);
    }

    // steady state and transient
    if (magneticAnalysisType == ANALYSISTYPE_STEADYSTATE ||
        magneticAnalysisType == ANALYSISTYPE_TRANSIENT)
    {
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL), PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD), PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_REAL);
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT) cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL), PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_POWER_LOSSES), PHYSICFIELDVARIABLE_MAGNETIC_POWER_LOSSES);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE), PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_ENERGY_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_ENERGY_DENSITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_PERMEABILITY), PHYSICFIELDVARIABLE_MAGNETIC_PERMEABILITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_CONDUCTIVITY), PHYSICFIELDVARIABLE_MAGNETIC_CONDUCTIVITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY), PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE), PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE);
    }
}

void HermesMagnetic::fillComboBoxVectorVariable(QComboBox *cmbFieldVariable)
{
    // harmonic
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD), PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE), PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE), PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY), PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY);
    }

    // steady state and transient
    if (magneticAnalysisType == ANALYSISTYPE_STEADYSTATE || magneticAnalysisType == ANALYSISTYPE_TRANSIENT)
    {
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY), PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD), PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE), PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE), PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE);
        cmbFieldVariable->addItem(physicFieldVariableString(PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY), PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY);
    }
}


void HermesMagnetic::showLocalValue(QTreeWidget *trvWidget, LocalPointValue *localPointValue)
{
    LocalPointValueMagnetic *localPointValueMagnetic = dynamic_cast<LocalPointValueMagnetic *>(localPointValue);

    // magnetic
    QTreeWidgetItem *magneticNode = new QTreeWidgetItem(trvWidget);
    magneticNode->setText(0, tr("Magnetic field"));
    magneticNode->setExpanded(true);

    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        // Permittivity
        addTreeWidgetItemValue(magneticNode, tr("Permeability:"), QString("%1").arg(localPointValueMagnetic->permeability, 0, 'f', 2), "");

        // Potential
        QTreeWidgetItem *itemPotential = new QTreeWidgetItem(magneticNode);
        itemPotential->setText(0, tr("Vector potential"));
        itemPotential->setExpanded(true);

        addTreeWidgetItemValue(itemPotential, tr("real:"), QString("%1").arg(localPointValueMagnetic->potential_real, 0, 'e', 3), "Wb/m");
        addTreeWidgetItemValue(itemPotential, tr("imag:"), QString("%1").arg(localPointValueMagnetic->potential_imag, 0, 'e', 3), "Wb/m");
        addTreeWidgetItemValue(itemPotential, tr("magnitude:"), QString("%1").arg(sqrt(sqr(localPointValueMagnetic->potential_real) + sqr(localPointValueMagnetic->potential_imag)), 0, 'e', 3), "Wb/m");

        // Flux Density
        addTreeWidgetItemValue(magneticNode, "Flux density:", QString("%1").arg(sqrt(sqr(localPointValueMagnetic->B_real.x) + sqr(localPointValueMagnetic->B_imag.x) + sqr(localPointValueMagnetic->B_real.y) + sqr(localPointValueMagnetic->B_imag.y)), 0, 'e', 3), "T");

        // Flux Density - real
        QTreeWidgetItem *itemFluxDensityReal = new QTreeWidgetItem(magneticNode);
        itemFluxDensityReal->setText(0, tr("Flux density - real"));
        itemFluxDensityReal->setExpanded(true);

        addTreeWidgetItemValue(itemFluxDensityReal, "B" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->B_real.x, 0, 'e', 3), "T");
        addTreeWidgetItemValue(itemFluxDensityReal, "B" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->B_real.y, 0, 'e', 3), "T");
        addTreeWidgetItemValue(itemFluxDensityReal, "B:", QString("%1").arg(localPointValueMagnetic->B_real.magnitude(), 0, 'e', 3), "T");

        // Flux Density - imag
        QTreeWidgetItem *itemFluxDensityImag = new QTreeWidgetItem(magneticNode);
        itemFluxDensityImag->setText(0, tr("Flux density - imag"));
        itemFluxDensityImag->setExpanded(true);

        addTreeWidgetItemValue(itemFluxDensityImag, "B" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->B_imag.x, 0, 'e', 3), "T");
        addTreeWidgetItemValue(itemFluxDensityImag, "B" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->B_imag.y, 0, 'e', 3), "T");
        addTreeWidgetItemValue(itemFluxDensityImag, "B:", QString("%1").arg(localPointValueMagnetic->B_imag.magnitude(), 0, 'e', 3), "T");

        // Magnetic Field
        addTreeWidgetItemValue(magneticNode, "Magnetic field:", QString("%1").arg(sqrt(sqr(localPointValueMagnetic->H_real.x) + sqr(localPointValueMagnetic->H_imag.x) + sqr(localPointValueMagnetic->H_real.y) + sqr(localPointValueMagnetic->H_imag.y)), 0, 'e', 3), "A/m");

        // Magnetic Field - real
        QTreeWidgetItem *itemMagneticFieldReal = new QTreeWidgetItem(magneticNode);
        itemMagneticFieldReal->setText(0, tr("Magnetic field - real"));
        itemMagneticFieldReal->setExpanded(true);

        addTreeWidgetItemValue(itemMagneticFieldReal, "H" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->H_real.x, 0, 'e', 3), "A/m");
        addTreeWidgetItemValue(itemMagneticFieldReal, "H" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->H_real.y, 0, 'e', 3), "A/m");
        addTreeWidgetItemValue(itemMagneticFieldReal, "H", QString("%1").arg(localPointValueMagnetic->H_real.magnitude(), 0, 'e', 3), "A/m");

        // Magnetic Field - imag
        QTreeWidgetItem *itemMagneticFieldImag = new QTreeWidgetItem(magneticNode);
        itemMagneticFieldImag->setText(0, tr("Magnetic field - imag"));
        itemMagneticFieldImag->setExpanded(true);

        addTreeWidgetItemValue(itemMagneticFieldImag, "H" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->H_imag.x, 0, 'e', 3), "A/m");
        addTreeWidgetItemValue(itemMagneticFieldImag, "H" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->H_imag.y, 0, 'e', 3), "A/m");
        addTreeWidgetItemValue(itemMagneticFieldImag, "H", QString("%1").arg(localPointValueMagnetic->H_imag.magnitude(), 0, 'e', 3), "A/m");

        // Current Density
        QTreeWidgetItem *itemCurrentDensity = new QTreeWidgetItem(magneticNode);
        itemCurrentDensity->setText(0, tr("Currrent dens. - external"));
        itemCurrentDensity->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentDensity, tr("real:"), QString("%1").arg(localPointValueMagnetic->current_density_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensity, tr("imag:"), QString("%1").arg(localPointValueMagnetic->current_density_imag, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensity, tr("magnitude:"), QString("%1").arg(sqrt(sqr(localPointValueMagnetic->current_density_real) + sqr(localPointValueMagnetic->current_density_imag)), 0, 'e', 3), "A/m2");

        // Transform induced current density
        QTreeWidgetItem *itemCurrentDensityInducedTransform = new QTreeWidgetItem(magneticNode);
        itemCurrentDensityInducedTransform->setText(0, tr("Current density - avg. transform"));
        itemCurrentDensityInducedTransform->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentDensityInducedTransform, tr("real:"), QString("%1").arg(localPointValueMagnetic->current_density_induced_transform_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensityInducedTransform, tr("imag:"), QString("%1").arg(localPointValueMagnetic->current_density_induced_transform_imag, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensityInducedTransform, tr("magnitude:"), QString("%1").arg(sqrt(sqr(localPointValueMagnetic->current_density_induced_transform_real) + sqr(localPointValueMagnetic->current_density_induced_transform_imag)), 0, 'e', 3), "A/m2");

        // Velocity induced current density
        QTreeWidgetItem *itemCurrentDensityInducedVelocity = new QTreeWidgetItem(magneticNode);
        itemCurrentDensityInducedVelocity->setText(0, tr("Current density - velocity"));
        itemCurrentDensityInducedVelocity->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentDensityInducedVelocity, tr("real:"), QString("%1").arg(localPointValueMagnetic->current_density_induced_velocity_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensityInducedVelocity, tr("imag:"), QString("%1").arg(localPointValueMagnetic->current_density_induced_velocity_imag, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensityInducedVelocity, tr("magnitude:"), QString("%1").arg(sqrt(sqr(localPointValueMagnetic->current_density_induced_velocity_real) + sqr(localPointValueMagnetic->current_density_induced_velocity_imag)), 0, 'e', 3), "A/m2");

        // Total current density
        QTreeWidgetItem *itemCurrentDensityTotal = new QTreeWidgetItem(magneticNode);
        itemCurrentDensityTotal->setText(0, tr("Current density - total"));
        itemCurrentDensityTotal->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentDensityTotal, tr("real:"), QString("%1").arg(localPointValueMagnetic->current_density_total_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensityTotal, tr("imag:"), QString("%1").arg(localPointValueMagnetic->current_density_total_imag, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemCurrentDensityTotal, tr("magnitude:"), QString("%1").arg(sqrt(sqr(localPointValueMagnetic->current_density_total_real) + sqr(localPointValueMagnetic->current_density_total_imag)), 0, 'e', 3), "A/m2");

        // Magnetic Field
        addTreeWidgetItemValue(magneticNode, "Lorentz force:", QString("%1").arg(sqrt(sqr(localPointValueMagnetic->FL_real.x) + sqr(localPointValueMagnetic->FL_imag.x) + sqr(localPointValueMagnetic->FL_real.y) + sqr(localPointValueMagnetic->FL_imag.y)), 0, 'e', 3), "N");

        // Lorentz force - real
        QTreeWidgetItem *itemLorentzForceReal = new QTreeWidgetItem(magneticNode);
        itemLorentzForceReal->setText(0, tr("Lorentz force - real"));
        itemLorentzForceReal->setExpanded(true);

        addTreeWidgetItemValue(itemLorentzForceReal, "FL" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->FL_real.x, 0, 'e', 3), "N/m3");
        addTreeWidgetItemValue(itemLorentzForceReal, "FL" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->FL_real.y, 0, 'e', 3), "N/m3");
        addTreeWidgetItemValue(itemLorentzForceReal, "FL", QString("%1").arg(localPointValueMagnetic->FL_real.magnitude(), 0, 'e', 3), "N/m3");

        // Lorentz force - imag
        QTreeWidgetItem *itemLorentzForceImag = new QTreeWidgetItem(magneticNode);
        itemLorentzForceImag->setText(0, tr("Lorentz force - imag"));
        itemLorentzForceImag->setExpanded(true);

        addTreeWidgetItemValue(itemLorentzForceImag, "FL" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->FL_imag.x, 0, 'e', 3), "N/m3");
        addTreeWidgetItemValue(itemLorentzForceImag, "FL" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->FL_imag.y, 0, 'e', 3), "N/m3");
        addTreeWidgetItemValue(itemLorentzForceImag, "FL", QString("%1").arg(localPointValueMagnetic->FL_imag.magnitude(), 0, 'e', 3), "N/m3");

        // Power losses
        addTreeWidgetItemValue(magneticNode, tr("Losses density:"), QString("%1").arg(localPointValueMagnetic->pj, 0, 'e', 3), "W/m3");

        // Energy density
        addTreeWidgetItemValue(magneticNode, tr("Energy density:"), QString("%1").arg(localPointValueMagnetic->wm, 0, 'e', 3), "J/m3");
    }
    else
    {
        // Permeability
        addTreeWidgetItemValue(magneticNode, tr("Permeability:"), QString("%1").arg(localPointValueMagnetic->permeability, 0, 'f', 2), "");

        // Conductivity
        addTreeWidgetItemValue(magneticNode, tr("Conductivity:"), QString("%1").arg(localPointValueMagnetic->conductivity, 0, 'g', 3), "");

        // Remanence
        addTreeWidgetItemValue(magneticNode, tr("Rem. flux dens.:"), QString("%1").arg(localPointValueMagnetic->remanence, 0, 'e', 3), "T");
        addTreeWidgetItemValue(magneticNode, tr("Direction of rem.:"), QString("%1").arg(localPointValueMagnetic->remanence_angle, 0, 'f', 2), "deg.");

        // Velocity
        QTreeWidgetItem *itemVelocity = new QTreeWidgetItem(magneticNode);
        itemVelocity->setText(0, tr("Velocity"));
        itemVelocity->setExpanded(true);

        addTreeWidgetItemValue(itemVelocity, Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->velocity.x, 0, 'e', 3), "m/s");
        addTreeWidgetItemValue(itemVelocity, Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->velocity.y, 0, 'e', 3), "m/s");

        // Potential
        addTreeWidgetItemValue(magneticNode, tr("Vector potential:"), QString("%1").arg(localPointValueMagnetic->potential_real, 0, 'e', 3), "Wb/m");

        // Flux Density
        QTreeWidgetItem *itemFluxDensity = new QTreeWidgetItem(magneticNode);
        itemFluxDensity->setText(0, tr("Flux density"));
        itemFluxDensity->setExpanded(true);

        addTreeWidgetItemValue(itemFluxDensity, "B" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->B_real.x, 0, 'e', 3), "T");
        addTreeWidgetItemValue(itemFluxDensity, "B" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->B_real.y, 0, 'e', 3), "T");
        addTreeWidgetItemValue(itemFluxDensity, "B:", QString("%1").arg(localPointValueMagnetic->B_real.magnitude(), 0, 'e', 3), "T");

        // Magnetic Field
        QTreeWidgetItem *itemMagneticField = new QTreeWidgetItem(magneticNode);
        itemMagneticField->setText(0, tr("Magnetic field"));
        itemMagneticField->setExpanded(true);

        addTreeWidgetItemValue(itemMagneticField, "H" + Util::scene()->problemInfo()->labelX().toLower() + ":", QString("%1").arg(localPointValueMagnetic->H_real.x, 0, 'e', 3), "A/m");
        addTreeWidgetItemValue(itemMagneticField, "H" + Util::scene()->problemInfo()->labelY().toLower() + ":", QString("%1").arg(localPointValueMagnetic->H_real.y, 0, 'e', 3), "A/m");
        addTreeWidgetItemValue(itemMagneticField, "H", QString("%1").arg(localPointValueMagnetic->H_real.magnitude(), 0, 'e', 3), "A/m");

        // Current density
        QTreeWidgetItem *itemInducedCurrentDensity = new QTreeWidgetItem(magneticNode);
        itemInducedCurrentDensity->setText(0, tr("Current density"));
        itemInducedCurrentDensity->setExpanded(true);

        addTreeWidgetItemValue(itemInducedCurrentDensity, tr("external:"), QString("%1").arg(localPointValueMagnetic->current_density_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemInducedCurrentDensity, tr("transform:"), QString("%1").arg(localPointValueMagnetic->current_density_induced_transform_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemInducedCurrentDensity, tr("velocity:"), QString("%1").arg(localPointValueMagnetic->current_density_induced_velocity_real, 0, 'e', 3), "A/m2");
        addTreeWidgetItemValue(itemInducedCurrentDensity, tr("total:"), QString("%1").arg(localPointValueMagnetic->current_density_total_real, 0, 'e', 3), "A/m2");

        // Power losses
        addTreeWidgetItemValue(magneticNode, tr("Losses density:"), QString("%1").arg(localPointValueMagnetic->pj, 0, 'e', 3), "W/m3");

        // Energy density
        addTreeWidgetItemValue(magneticNode, tr("Energy density:"), QString("%1").arg(localPointValueMagnetic->wm, 0, 'e', 3), "J/m3");
    }
}

void HermesMagnetic::showSurfaceIntegralValue(QTreeWidget *trvWidget, SurfaceIntegralValue *surfaceIntegralValue)
{
    SurfaceIntegralValueMagnetic *surfaceIntegralValueMagnetic = dynamic_cast<SurfaceIntegralValueMagnetic *>(surfaceIntegralValue);

    QTreeWidgetItem *magneticNode = new QTreeWidgetItem(trvWidget);
    magneticNode->setText(0, tr("Magnetic field"));
    magneticNode->setExpanded(true);

    // force
    QTreeWidgetItem *itemForce = new QTreeWidgetItem(magneticNode);
    itemForce->setText(0, tr("Maxwell force"));
    itemForce->setExpanded(true);

    addTreeWidgetItemValue(itemForce, Util::scene()->problemInfo()->labelX(), QString("%1").arg(surfaceIntegralValueMagnetic->forceMaxwellX, 0, 'e', 3), "N");
    addTreeWidgetItemValue(itemForce, Util::scene()->problemInfo()->labelY(), QString("%1").arg(surfaceIntegralValueMagnetic->forceMaxwellY, 0, 'e', 3), "N");
}

void HermesMagnetic::showVolumeIntegralValue(QTreeWidget *trvWidget, VolumeIntegralValue *volumeIntegralValue)
{
    VolumeIntegralValueMagnetic *volumeIntegralValueMagnetic = dynamic_cast<VolumeIntegralValueMagnetic *>(volumeIntegralValue);

    // harmonic
    QTreeWidgetItem *magneticNode = new QTreeWidgetItem(trvWidget);
    magneticNode->setText(0, tr("Magnetic field"));
    magneticNode->setExpanded(true);

    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        // external current
        QTreeWidgetItem *itemCurrentInducedExternal = new QTreeWidgetItem(magneticNode);
        itemCurrentInducedExternal->setText(0, tr("External current"));
        itemCurrentInducedExternal->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentInducedExternal, tr("real:"), QString("%1").arg(volumeIntegralValueMagnetic->currentReal, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentInducedExternal, tr("imag:"), QString("%1").arg(volumeIntegralValueMagnetic->currentImag, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentInducedExternal, tr("magnitude:"), QString("%1").arg(sqrt(sqr(volumeIntegralValueMagnetic->currentReal) + sqr(volumeIntegralValueMagnetic->currentImag)), 0, 'e', 3), "A");

        // transform induced current
        QTreeWidgetItem *itemCurrentInducedTransform = new QTreeWidgetItem(magneticNode);
        itemCurrentInducedTransform->setText(0, tr("Transform induced current"));
        itemCurrentInducedTransform->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentInducedTransform, tr("real:"), QString("%1").arg(volumeIntegralValueMagnetic->currentInducedTransformReal, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentInducedTransform, tr("imag:"), QString("%1").arg(volumeIntegralValueMagnetic->currentInducedTransformImag, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentInducedTransform, tr("magnitude:"), QString("%1").arg(sqrt(sqr(volumeIntegralValueMagnetic->currentInducedTransformReal) + sqr(volumeIntegralValueMagnetic->currentInducedTransformImag)), 0, 'e', 3), "A");

        // velocity induced current
        QTreeWidgetItem *itemCurrentInducedVelocity = new QTreeWidgetItem(magneticNode);
        itemCurrentInducedVelocity->setText(0, tr("Velocity induced current"));
        itemCurrentInducedVelocity->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentInducedVelocity, tr("real:"), QString("%1").arg(volumeIntegralValueMagnetic->currentInducedVelocityReal, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentInducedVelocity, tr("imag:"), QString("%1").arg(volumeIntegralValueMagnetic->currentInducedVelocityImag, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentInducedVelocity, tr("magnitude:"), QString("%1").arg(sqrt(sqr(volumeIntegralValueMagnetic->currentInducedVelocityReal) + sqr(volumeIntegralValueMagnetic->currentInducedVelocityImag)), 0, 'e', 3), "A");

        // total current
        QTreeWidgetItem *itemCurrentTotal = new QTreeWidgetItem(magneticNode);
        itemCurrentTotal->setText(0, tr("Total current"));
        itemCurrentTotal->setExpanded(true);

        addTreeWidgetItemValue(itemCurrentTotal, tr("real:"), QString("%1").arg(volumeIntegralValueMagnetic->currentTotalReal, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentTotal, tr("imag:"), QString("%1").arg(volumeIntegralValueMagnetic->currentTotalImag, 0, 'e', 3), "A");
        addTreeWidgetItemValue(itemCurrentTotal, tr("magnitude:"), QString("%1").arg(sqrt(sqr(volumeIntegralValueMagnetic->currentTotalReal) + sqr(volumeIntegralValueMagnetic->currentTotalImag)), 0, 'e', 3), "A");

        addTreeWidgetItemValue(magneticNode, tr("Power Losses:"), QString("%1").arg(volumeIntegralValueMagnetic->powerLosses, 0, 'e', 3), tr("W"));
        addTreeWidgetItemValue(magneticNode, tr("Energy avg.:"), QString("%1").arg(volumeIntegralValueMagnetic->energy, 0, 'e', 3), tr("J"));

        // force
        QTreeWidgetItem *itemForce = new QTreeWidgetItem(magneticNode);
        itemForce->setText(0, tr("Lorentz force avg."));
        itemForce->setExpanded(true);

        addTreeWidgetItemValue(itemForce, Util::scene()->problemInfo()->labelX(), QString("%1").arg(volumeIntegralValueMagnetic->forceLorentzX, 0, 'e', 3), "N");
        addTreeWidgetItemValue(itemForce, Util::scene()->problemInfo()->labelY(), QString("%1").arg(volumeIntegralValueMagnetic->forceLorentzY, 0, 'e', 3), "N");

        addTreeWidgetItemValue(magneticNode, tr("Torque:"), QString("%1").arg(volumeIntegralValueMagnetic->torque, 0, 'e', 3), tr("Nm"));
    }
    else
    {
        addTreeWidgetItemValue(magneticNode, tr("External current:"), QString("%1").arg(volumeIntegralValueMagnetic->currentReal, 0, 'e', 3), tr("A"));
        addTreeWidgetItemValue(magneticNode, tr("Transform current:"), QString("%1").arg(volumeIntegralValueMagnetic->currentInducedTransformReal, 0, 'e', 3), tr("A"));
        addTreeWidgetItemValue(magneticNode, tr("Velocity current:"), QString("%1").arg(volumeIntegralValueMagnetic->currentInducedVelocityReal, 0, 'e', 3), tr("A"));
        addTreeWidgetItemValue(magneticNode, tr("Total current:"), QString("%1").arg(volumeIntegralValueMagnetic->currentTotalReal, 0, 'e', 3), tr("A"));
        addTreeWidgetItemValue(magneticNode, tr("Power Losses:"), QString("%1").arg(volumeIntegralValueMagnetic->powerLosses, 0, 'e', 3), tr("W"));
        addTreeWidgetItemValue(magneticNode, tr("Energy:"), QString("%1").arg(volumeIntegralValueMagnetic->energy, 0, 'e', 3), tr("J"));

        QTreeWidgetItem *itemForce = new QTreeWidgetItem(magneticNode);
        itemForce->setText(0, tr("Force"));
        itemForce->setExpanded(true);

        addTreeWidgetItemValue(itemForce, Util::scene()->problemInfo()->labelX(), QString("%1").arg(volumeIntegralValueMagnetic->forceLorentzX, 0, 'e', 3), tr("N"));
        addTreeWidgetItemValue(itemForce, Util::scene()->problemInfo()->labelY(), QString("%1").arg(volumeIntegralValueMagnetic->forceLorentzY, 0, 'e', 3), tr("N"));

        addTreeWidgetItemValue(magneticNode, tr("Torque:"), QString("%1").arg(volumeIntegralValueMagnetic->torque, 0, 'e', 3), tr("Nm"));
    }
}

ViewScalarFilter *HermesMagnetic::viewScalarFilter(PhysicFieldVariable physicFieldVariable, PhysicFieldVariableComp physicFieldVariableComp)
{
    Solution *sln1 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution());
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_STEADYSTATE)
    {
        return new ViewScalarFilterMagnetic(sln1,
                                            physicFieldVariable,
                                            physicFieldVariableComp);
    }

    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        Solution *sln2 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution() + 1);
        return new ViewScalarFilterMagnetic(sln1,
                                            sln2,
                                            physicFieldVariable,
                                            physicFieldVariableComp);
    }
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
    {
        Solution *sln2;
        if (Util::scene()->sceneSolution()->timeStep() > 0)
            sln2 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution() - 1);
        else
            sln2 = sln1;

        return new ViewScalarFilterMagnetic(sln1,
                                            sln2,
                                            physicFieldVariable,
                                            physicFieldVariableComp);
    }
}

QList<SolutionArray *> *HermesMagnetic::solve(SolverDialog *solverDialog)
{
    // transient
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
    {
        if (!Util::scene()->problemInfo()->timeStep.evaluate()) return NULL;
        if (!Util::scene()->problemInfo()->timeTotal.evaluate()) return NULL;
        if (!Util::scene()->problemInfo()->initialCondition.evaluate()) return NULL;
    }

    // edge markers
    magneticEdge = new MagneticEdge[Util::scene()->edges.count()+1];
    magneticEdge[0].type = PHYSICFIELDBC_NONE;
    magneticEdge[0].value_real = 0.0;
    magneticEdge[0].value_imag = 0.0;
    for (int i = 0; i<Util::scene()->edges.count(); i++)
    {
        if (Util::scene()->edgeMarkers.indexOf(Util::scene()->edges[i]->marker) == 0)
        {
            magneticEdge[i+1].type = PHYSICFIELDBC_NONE;
            magneticEdge[i+1].value_real = 0.0;
            magneticEdge[i+1].value_imag = 0.0;
        }
        else
        {
            SceneEdgeMagneticMarker *edgeMagneticMarker = dynamic_cast<SceneEdgeMagneticMarker *>(Util::scene()->edges[i]->marker);

            // evaluate script
            if (!edgeMagneticMarker->value_real.evaluate()) return NULL;
            if (!edgeMagneticMarker->value_imag.evaluate()) return NULL;

            magneticEdge[i+1].type = edgeMagneticMarker->type;
            magneticEdge[i+1].value_real = edgeMagneticMarker->value_real.number;
            magneticEdge[i+1].value_imag = edgeMagneticMarker->value_imag.number;
        }
    }

    // label markers
    magneticLabel = new MagneticLabel[Util::scene()->labels.count()];
    for (int i = 0; i<Util::scene()->labels.count(); i++)
    {
        if (Util::scene()->labelMarkers.indexOf(Util::scene()->labels[i]->marker) == 0)
        {
        }
        else
        {
            SceneLabelMagneticMarker *labelMagneticMarker = dynamic_cast<SceneLabelMagneticMarker *>(Util::scene()->labels[i]->marker);

            // evaluate script
            if (!labelMagneticMarker->current_density_real.evaluate()) return NULL;
            if (!labelMagneticMarker->current_density_imag.evaluate()) return NULL;
            if (!labelMagneticMarker->permeability.evaluate()) return NULL;
            if (!labelMagneticMarker->conductivity.evaluate()) return NULL;
            if (!labelMagneticMarker->remanence.evaluate()) return NULL;
            if (!labelMagneticMarker->remanence_angle.evaluate()) return NULL;
            if (!labelMagneticMarker->velocity_x.evaluate()) return NULL;
            if (!labelMagneticMarker->velocity_y.evaluate()) return NULL;
            if (!labelMagneticMarker->velocity_angular.evaluate()) return NULL;

            magneticLabel[i].current_density_real = labelMagneticMarker->current_density_real.number;
            magneticLabel[i].current_density_imag = labelMagneticMarker->current_density_imag.number;
            magneticLabel[i].permeability = labelMagneticMarker->permeability.number;
            magneticLabel[i].conductivity = labelMagneticMarker->conductivity.number;
            magneticLabel[i].remanence = labelMagneticMarker->remanence.number;
            magneticLabel[i].remanence_angle = labelMagneticMarker->remanence_angle.number;
            magneticLabel[i].velocity_x = labelMagneticMarker->velocity_x.number;
            magneticLabel[i].velocity_y = labelMagneticMarker->velocity_y.number;
            magneticLabel[i].velocity_angular = labelMagneticMarker->velocity_angular.number;        }
    }

    QList<SolutionArray *> *solutionArrayList = magnetic_main(solverDialog);

    delete [] magneticEdge;
    delete [] magneticLabel;

    return solutionArrayList;
}

// ****************************************************************************************************************

LocalPointValueMagnetic::LocalPointValueMagnetic(Point &point) : LocalPointValue(point)
{
    permeability = 0;
    conductivity = 0;
    remanence = 0;
    remanence_angle = 0;

    potential_real = 0;
    potential_imag = 0;

    current_density_real = 0;
    current_density_imag = 0;
    current_density_induced_transform_real = 0;
    current_density_induced_transform_imag = 0;
    current_density_induced_velocity_real = 0;
    current_density_induced_velocity_imag = 0;
    current_density_total_real = 0;
    current_density_total_imag = 0;

    H_real = Point();
    H_imag = Point();
    B_real = Point();
    B_imag = Point();
    FL_real = Point();
    FL_imag = Point();

    pj = 0;
    wm = 0;

    if (Util::scene()->sceneSolution()->isSolved())
    {
        // value real
        PointValue valueReal = PointValue(value, derivative, labelMarker);

        SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(valueReal.marker);
        // solution
        if (marker != NULL)
        {
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_STEADYSTATE ||
                Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
            {
                Point derReal;
                derReal = valueReal.derivative;

                if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
                {
                    // potential
                    potential_real = valueReal.value;

                    // flux density
                    B_real.x =   derReal.y;
                    B_real.y = - derReal.x;
                }
                else
                {
                    // potential
                    potential_real = valueReal.value;

                    // flux density
                    B_real.x = -  derReal.y;
                    B_real.y =   (derReal.x + ((point.x > 0.0) ? valueReal.value/point.x : 0.0));
                }

                permeability = marker->permeability.number;
                conductivity = marker->conductivity.number;
                remanence = marker->remanence.number;
                remanence_angle = marker->remanence_angle.number;
                velocity = Point(marker->velocity_x.number - marker->velocity_angular.number * point.y,
                                 marker->velocity_y.number + marker->velocity_angular.number * point.x);

                // current density
                current_density_real = marker->current_density_real.number;

                // induced transform current density
                if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
                {
                    Solution *sln2;
                    if (Util::scene()->sceneSolution()->timeStep() > 0)
                        sln2 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution() - 1);
                    else
                        sln2 = Util::scene()->sceneSolution()->sln();

                    PointValue valuePrevious = pointValue(sln2, point);
                    current_density_induced_transform_real = - marker->conductivity.number * (valueReal.value - valuePrevious.value) / Util::scene()->problemInfo()->timeStep.number;
                }

                // induced current density velocity
                current_density_induced_velocity_real = - conductivity * (velocity.x * derReal.x + velocity.y * derReal.y);

                // total current density
                current_density_total_real = current_density_real + current_density_induced_transform_real + current_density_induced_velocity_real;

                // electric displacement
                H_real = B_real / (marker->permeability.number * MU0);

                // Ltorentz force
                FL_real.x = - current_density_total_real*B_real.y;
                FL_real.y =   current_density_total_real*B_real.x;

                // power losses
                pj = (marker->conductivity.number > 0.0) ?
                        1.0 / marker->conductivity.number * (sqr(current_density_total_real) + sqr(current_density_total_imag))
                        :
                        0.0;

                // energy density
                wm = 0.5 * (sqr(B_real.x) + sqr(B_real.y) + sqr(B_imag.x) + sqr(B_imag.y)) / (marker->permeability.number * MU0);
            }

            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
            {
                Solution *sln2 = Util::scene()->sceneSolution()->sln(1);

                // value imag
                PointValue valueImag = pointValue(sln2, point);
                double frequency = Util::scene()->problemInfo()->frequency;

                Point derReal = valueReal.derivative;
                Point derImag = valueImag.derivative;

                if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
                {
                    // potential
                    potential_real = valueReal.value;
                    potential_imag = valueImag.value;

                    // flux density
                    B_real.x =  derReal.y;
                    B_real.y = -derReal.x;

                    B_imag.x =  derImag.y;
                    B_imag.y = -derImag.x;
                }
                else
                {
                    // potential
                    potential_real = valueReal.value;
                    potential_imag = valueImag.value;

                    // flux density
                    B_real.x = -  derReal.y;
                    B_real.y =   (derReal.x + ((point.x > 0.0) ? valueReal.value/point.x : 0.0));

                    B_imag.x = -  derImag.y;
                    B_imag.y =   (derImag.x + ((point.x > 0.0) ? valueImag.value/point.x : 0.0));
                }

                permeability = marker->permeability.number;
                conductivity = marker->conductivity.number;
                remanence = marker->remanence.number;
                remanence_angle = marker->remanence_angle.number;
                velocity = Point(marker->velocity_x.number - marker->velocity_angular.number * point.y,
                                 marker->velocity_y.number + marker->velocity_angular.number * point.x);

                // current density
                current_density_real = marker->current_density_real.number;
                current_density_imag = marker->current_density_imag.number;

                // induced current density transform (harmonic)
                current_density_induced_transform_real =   2 * M_PI * frequency * marker->conductivity.number * valueImag.value;
                current_density_induced_transform_imag = - 2 * M_PI * frequency * marker->conductivity.number * valueReal.value;

                // induced current density velocity
                current_density_induced_velocity_real = - conductivity * (velocity.x * derReal.x + velocity.y * derReal.y);
                current_density_induced_velocity_imag = - conductivity * (velocity.x * derImag.x + velocity.y * derImag.y);

                // total current density
                current_density_total_real = current_density_real + current_density_induced_transform_real + current_density_induced_velocity_real;
                current_density_total_imag = current_density_imag + current_density_induced_transform_imag + current_density_induced_velocity_imag;

                // electric displacement
                H_real = B_real / (marker->permeability.number * MU0);
                H_imag = B_imag / (marker->permeability.number * MU0);

                // Lorentz force
                FL_real.x = - (current_density_total_real*B_real.y - current_density_total_imag*B_imag.y);
                FL_real.y =   (current_density_total_real*B_real.x - current_density_total_imag*B_imag.x);
                FL_imag.x = - (current_density_total_imag*B_real.y + current_density_total_real*B_imag.y);
                FL_imag.y =   (current_density_total_imag*B_real.x + current_density_total_real*B_imag.x);

                // power losses
                pj = (marker->conductivity.number > 0.0) ?
                        0.5 / marker->conductivity.number * (sqr(current_density_total_real) + sqr(current_density_total_imag))
                        :
                        0.0;

                // energy density
                wm = 0.25 * (sqr(B_real.x) + sqr(B_real.y) + sqr(B_imag.x) + sqr(B_imag.y)) / (marker->permeability.number * MU0);
            }
        }
    }
}

double LocalPointValueMagnetic::variableValue(PhysicFieldVariable physicFieldVariable, PhysicFieldVariableComp physicFieldVariableComp)
{
    switch (physicFieldVariable)
    {
    case PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL:
        {
            return sqrt(sqr(potential_real) + sqr(potential_imag));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_REAL:
        {
            return potential_real;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_IMAG:
        {
            return potential_imag;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY:
        {
            return sqrt(sqr(B_real.x) + sqr(B_imag.x) + sqr(B_real.y) + sqr(B_imag.y));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL:
        {
            switch (physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                return B_real.x;
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                return B_real.y;
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                return B_real.magnitude();
                break;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_IMAG:
        {
            switch (physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                return B_imag.x;
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                return B_imag.y;
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                return B_imag.magnitude();
                break;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD:
        {
            return sqrt(sqr(H_real.x) + sqr(H_imag.x) + sqr(H_real.y) + sqr(H_imag.y));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL:
        {
            switch (physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                return H_real.x;
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                return H_real.y;
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                return H_real.magnitude();
                break;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_IMAG:
        {
            switch (physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                return H_imag.x;
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                return H_imag.y;
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                return H_imag.magnitude();
                break;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_REAL:
        {
            return current_density_real;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_IMAG:
        {
            return current_density_imag;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY:
        {
            return sqrt(sqr(current_density_real) + sqr(current_density_imag));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_REAL:
        {
            return current_density_induced_transform_real;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_IMAG:
        {
            return current_density_induced_transform_imag;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM:
        {
            return sqrt(sqr(current_density_induced_transform_real) + sqr(current_density_induced_transform_imag));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_REAL:
        {
            return current_density_induced_velocity_real;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_IMAG:
        {
            return current_density_induced_velocity_imag;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY:
        {
            return sqrt(sqr(current_density_induced_velocity_real) + sqr(current_density_induced_velocity_imag));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_REAL:
        {
            return current_density_total_real;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_IMAG:
        {
            return current_density_total_imag;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL:
        {
            return sqrt(sqr(current_density_total_real) + sqr(current_density_total_imag));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_POWER_LOSSES:
        {
            return pj;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_ENERGY_DENSITY:
        {
            return wm;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_PERMEABILITY:
        {
            return permeability;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CONDUCTIVITY:
        {
            return conductivity;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY:
        {
            switch (physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                return velocity.x;
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                return velocity.y;
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                return sqrt(sqr(velocity.x) + sqr(velocity.y));
                break;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE:
        {
            switch (physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                return remanence * cos(remanence_angle / 180.0 * M_PI);
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                return remanence * sin(remanence_angle / 180.0 * M_PI);
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                return remanence;
                break;
            }
        }
        break;
    default:
        cerr << "Physical field variable '" + physicFieldVariableString(physicFieldVariable).toStdString() + "' is not implemented. LocalPointValueMagnetic::variableValue(PhysicFieldVariable physicFieldVariable, PhysicFieldVariableComp physicFieldVariableComp)" << endl;
        throw;
        break;
    }
}

QStringList LocalPointValueMagnetic::variables()
{
    QStringList row;
    row <<  QString("%1").arg(point.x, 0, 'e', 5) <<
            QString("%1").arg(point.y, 0, 'e', 5) <<
            QString("%1").arg(potential_real, 0, 'e', 5) <<
            QString("%1").arg(potential_imag, 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(potential_real) + sqr(potential_imag)), 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(B_real.x) + sqr(B_imag.x) + sqr(B_real.y) + sqr(B_imag.y)), 0, 'e', 5) <<
            QString("%1").arg(B_real.x, 0, 'e', 5) <<
            QString("%1").arg(B_real.y, 0, 'e', 5) <<
            QString("%1").arg(B_real.magnitude(), 0, 'e', 5) <<
            QString("%1").arg(B_imag.x, 0, 'e', 5) <<
            QString("%1").arg(B_imag.y, 0, 'e', 5) <<
            QString("%1").arg(B_imag.magnitude(), 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(H_real.x) + sqr(H_imag.x) + sqr(H_real.y) + sqr(H_imag.y)), 0, 'e', 5) <<
            QString("%1").arg(H_real.x, 0, 'e', 5) <<
            QString("%1").arg(H_real.y, 0, 'e', 5) <<
            QString("%1").arg(H_real.magnitude(), 0, 'e', 5) <<
            QString("%1").arg(H_imag.x, 0, 'e', 5) <<
            QString("%1").arg(H_imag.y, 0, 'e', 5) <<
            QString("%1").arg(H_imag.magnitude(), 0, 'e', 5) <<
            QString("%1").arg(current_density_real, 0, 'e', 5) <<
            QString("%1").arg(current_density_imag, 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(current_density_real) + sqr(current_density_imag)), 0, 'e', 5) <<
            QString("%1").arg(current_density_induced_transform_real, 0, 'e', 5) <<
            QString("%1").arg(current_density_induced_transform_imag, 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(current_density_induced_transform_real) + sqr(current_density_induced_transform_imag)), 0, 'e', 5) <<
            QString("%1").arg(current_density_induced_velocity_real, 0, 'e', 5) <<
            QString("%1").arg(current_density_induced_velocity_imag, 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(current_density_induced_velocity_real) + sqr(current_density_induced_velocity_imag)), 0, 'e', 5) <<
            QString("%1").arg(current_density_total_real, 0, 'e', 5) <<
            QString("%1").arg(current_density_total_imag, 0, 'e', 5) <<
            QString("%1").arg(sqrt(sqr(current_density_total_real) + sqr(current_density_total_imag)), 0, 'e', 5) <<
            QString("%1").arg(pj, 0, 'e', 5) <<
            QString("%1").arg(wm, 0, 'e', 5) <<
            QString("%1").arg(permeability, 0, 'f', 3) <<
            QString("%1").arg(conductivity, 0, 'e', 5) <<
            QString("%1").arg(remanence, 0, 'e', 5) <<
            QString("%1").arg(remanence_angle, 0, 'e', 5) <<
            QString("%1").arg(velocity.x, 0, 'e', 5) <<
            QString("%1").arg(velocity.y, 0, 'e', 5) <<
            QString("%1").arg(FL_real.x, 0, 'e', 5) <<
            QString("%1").arg(FL_real.y, 0, 'e', 5) <<
            QString("%1").arg(FL_imag.x, 0, 'e', 5) <<
            QString("%1").arg(FL_imag.y, 0, 'e', 5);

    return QStringList(row);
}

// ****************************************************************************************************************

SurfaceIntegralValueMagnetic::SurfaceIntegralValueMagnetic() : SurfaceIntegralValue()
{
    forceMaxwellX = 0;
    forceMaxwellY = 0;

    calculate();

    forceMaxwellX /= 2.0;
    forceMaxwellY /= 2.0;
}

void SurfaceIntegralValueMagnetic::calculateVariables(int i)
{
    SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(Util::scene()->labels[e->marker]->marker);
    if (marker->permeability.number < 1 + EPS_ZERO)
    {
        if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
        {
            if (marker->permeability.number < 1.0 + EPS_ZERO)
            {
                forceMaxwellX -= pt[i][2] * tan[i][2] * 1 / (2 * MU0 * marker->permeability.number) *
                                 (dudy[i] * 2 * (tan[i][1] * dudy[i] + tan[i][0] * dudx[i]) - tan[i][1] * (sqr(dudx[i]) + sqr(dudy[i])));
            }
            else
            {
                forceMaxwellX -= 2 * M_PI * x[i] * pt[i][2] * tan[i][2] * 1 / (2 * MU0 * marker->permeability.number) *
                                 (dudy[i] * 2 * (tan[i][1] * dudy[i] + tan[i][0] * (dudx[i] + ((x[i] > 0) ? value[i] / x[i] : 0.0))) - tan[i][1] * (sqr(dudx[i] + ((x[i] > 0) ? value[i] / x[i] : 0.0)) + sqr(dudy[i])));
            }
        }

        if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
        {
            forceMaxwellY -= pt[i][2] * tan[i][2] * 1 / (2 * MU0 * marker->permeability.number) *
                             (- dudx[i] * 2 * (tan[i][1] * dudy[i] + tan[i][0] * dudx[i]) + tan[i][0] * (sqr(dudx[i]) + sqr(dudy[i]))) ;
        }
        else
        {
            forceMaxwellY -= 2 * M_PI * x[i] * pt[i][2] * tan[i][2] * 1 / (2 * MU0 * marker->permeability.number) *
                             (- (dudx[i] + ((x[i] > 0) ? value[i] / x[i] : 0.0)) * 2 * (tan[i][1] * dudy[i] + tan[i][0] * (dudx[i] + ((x[i] > 0) ? value[i] / x[i] : 0.0))) + tan[i][0] * (sqr(dudx[i] + ((x[i] > 0) ? value[i] / x[i] : 0.0)) + sqr(dudy[i]))) ;
        }
    }
}

QStringList SurfaceIntegralValueMagnetic::variables()
{
    QStringList row;
    row <<  QString("%1").arg(length, 0, 'e', 5) <<
            QString("%1").arg(surface, 0, 'e', 5) <<
            QString("%1").arg(forceMaxwellX, 0, 'e', 5) <<
            QString("%1").arg(forceMaxwellY, 0, 'e', 5);
    return QStringList(row);
}


// ****************************************************************************************************************

VolumeIntegralValueMagnetic::VolumeIntegralValueMagnetic() : VolumeIntegralValue()
{  
    currentReal = 0;
    currentImag = 0;
    currentInducedTransformReal = 0;
    currentInducedTransformImag = 0;
    currentInducedVelocityReal = 0;
    currentInducedVelocityImag = 0;
    currentTotalReal = 0;
    currentTotalImag = 0;
    powerLosses = 0;
    energy = 0;
    forceLorentzX = 0;
    forceLorentzY = 0;
    torque = 0;

    calculate();

    currentTotalReal = currentReal + currentInducedTransformReal + currentInducedVelocityReal;
    currentTotalImag = currentImag + currentInducedTransformImag + currentInducedVelocityImag;
}

void VolumeIntegralValueMagnetic::calculateVariables(int i)
{
    SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(Util::scene()->labels[e->marker]->marker);

    // current - real
    result = 0.0;
    h1_integrate_expression(marker->current_density_real.number);
    currentReal += result;

    // current - imag
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        result = 0.0;
        h1_integrate_expression(marker->current_density_imag.number);
        currentImag += result;
    }

    // current induced transform - real
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        result = 0.0;
        if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
        {
            h1_integrate_expression(2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]);
        }
        else
        {
            h1_integrate_expression(2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]);
        }
        currentInducedTransformReal += result;
    }
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
    {
        result = 0.0;
        if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
        {
            h1_integrate_expression(- marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number);
        }
        else
        {
            h1_integrate_expression(- marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number);
        }
        currentInducedTransformReal += result;
    }

    // current induced transform - imag
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        result = 0.0;
        if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
        {
            h1_integrate_expression(- 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]);
        }
        else
        {
            h1_integrate_expression(- 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]);
        }
        currentInducedTransformImag += result;
    }

    // current induced velocity - real
    result = 0.0;
    if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
    {
        h1_integrate_expression(- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]));
    }
    else
    {
        h1_integrate_expression(- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]));
    }
    currentInducedVelocityReal += result;

    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
    {
        result = 0.0;
        if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
        {
            h1_integrate_expression(- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx2[i] +
                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy2[i]));
        }
        else
        {
        }
        currentInducedVelocityImag += result;
    }

    // power losses
    if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_STEADYSTATE)
        {
            result = 0.0;
            h1_integrate_expression((marker->conductivity.number > 0.0) ?
                                    1.0 / marker->conductivity.number * sqr(
                                            marker->current_density_real.number
                                          - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                           (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                    :
                                    0.0);
            powerLosses += result;
        }
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            // TODO: add velocity
            result = 0.0;
            h1_integrate_expression((marker->conductivity.number > 0.0) ?
                                    0.5 / marker->conductivity.number * (
                                              sqr(marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i])
                                            + sqr(marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]))
                                    :
                                    0.0);
            powerLosses += result;
        }
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
        {
            // TODO: add velocity
            result = 0.0;
            h1_integrate_expression((marker->conductivity.number > 0.0) ?
                                    1.0 / marker->conductivity.number * sqr(
                                            marker->current_density_real.number
                                            - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                            - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                             (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                    :
                                    0.0);
            powerLosses += result;
        }
    }
    else
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_STEADYSTATE)
        {
            result = 0.0;
            h1_integrate_expression((marker->conductivity.number > 0.0) ?
                                    2 * M_PI * x[i] * 1.0 / marker->conductivity.number * sqr(
                                            marker->current_density_real.number
                                          - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                           (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                    :
                                    0.0);
            powerLosses += result;
        }
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            // TODO: add velocity
            result = 0.0;
            h1_integrate_expression((marker->conductivity.number > 0.0) ?
                                    2 * M_PI * x[i] * 0.5 / marker->conductivity.number * (
                                              sqr(marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i])
                                            + sqr(marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]))
                                    :
                                    0.0);
            powerLosses += result;
        }
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
        {
            // TODO: add velocity
            result = 0.0;
            h1_integrate_expression((marker->conductivity.number > 0.0) ?
                                    2 * M_PI * x[i] * 1.0 / marker->conductivity.number * sqr(
                                            marker->current_density_real.number
                                            - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                            - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                             (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                    :
                                    0.0);
            powerLosses += result;
        }
    }

    // energy
    result = 0.0;
    if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression(0.25 * (sqr(dudx1[i]) + sqr(dudy1[i]) + sqr(dudx2[i]) + sqr(dudy2[i])) / (marker->permeability.number * MU0));
        }
        else
        {
            h1_integrate_expression(0.5 * (sqr(dudx1[i]) + sqr(dudy1[i])) / (marker->permeability.number * MU0));
        }
    }
    else
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression((2 * M_PI * x[i] * 0.25 * sqr(sqrt(sqr(dudy1[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)))) / (marker->permeability.number * MU0))
                                  + (2 * M_PI * x[i] * 0.25 * sqr(sqrt(sqr(dudy2[i]) + sqr(dudx2[i] + ((x[i] > 0) ? value2[i] / x[i] : 0.0)))) / (marker->permeability.number * MU0)));
        }
        else
        {
            h1_integrate_expression(2 * M_PI * x[i] * 0.5 * sqr(sqrt(sqr(dudy1[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)))) / (marker->permeability.number * MU0));
        }
    }
    energy += result;

    // Lorentz force X
    result = 0.0;
    if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression(- 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudx1[i])
                                             + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudx2[i]))
                                    +
                                    dudx1[i] * (marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                               (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));
        }
        else
        {
            h1_integrate_expression(dudx1[i] * (marker->current_density_real.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));

        }
    }
    else
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression(- 0.5 * (- (2 * M_PI * x[i] * (marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * (dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)))
                                             + (2 * M_PI * x[i] * (marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * (dudx2[i] + ((x[i] > 0) ? value2[i] / x[i] : 0.0))))
                                    +
                                    dudx1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));
        }
        else
        {
            h1_integrate_expression(dudx1[i] * (marker->current_density_imag.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));

        }
    }
    forceLorentzX += result;

    // Lorentz force Y
    result = 0.0;
    if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression(- 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                             + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i]))
                                    +
                                    dudy1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));
        }
        else
        {
            h1_integrate_expression(dudy1[i] * (- marker->current_density_real.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                       (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));

        }
    }
    else
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression(- 0.5 * (- (2 * M_PI * x[i] * (marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                             + (2 * M_PI * x[i] * (marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i]))
                                    +
                                    dudy1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));
        }
        else
        {
            h1_integrate_expression(dudy1[i] * (marker->current_density_imag.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));

        }
    }
    forceLorentzY += result;

    // torque
    result = 0.0;
    if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
    {
        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        {
            h1_integrate_expression(y[i] * (
                    - ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudx1[i])
                    + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudx2[i])
                    +
                    dudx1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])))
                                    -
                                    x[i] * (
                                            - ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                            + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i])
                                            +
                                            dudy1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])))
                                    );
        }
        else
        {
            h1_integrate_expression(y[i] *
                                    dudx1[i] * (marker->current_density_real.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                    -
                                    x[i] *
                                    dudy1[i] * (marker->current_density_real.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));

        }
    }
    else
    {
        h1_integrate_expression(0.0);
    }
    torque += result;
}

void VolumeIntegralValueMagnetic::initSolutions()
{
    sln1 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution());
    sln2 = NULL;
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
        sln2 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution() - 1);
    if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
        sln2 = Util::scene()->sceneSolution()->sln(Util::scene()->sceneSolution()->timeStep() * Util::scene()->problemInfo()->hermes()->numberOfSolution() + 1);
}

QStringList VolumeIntegralValueMagnetic::variables()
{ 
    QStringList row;
    row <<  QString("%1").arg(volume, 0, 'e', 5) <<
            QString("%1").arg(crossSection, 0, 'e', 5) <<
            QString("%1").arg(currentReal, 0, 'e', 5) <<
            QString("%1").arg(currentImag, 0, 'e', 5) <<
            QString("%1").arg(currentInducedTransformReal, 0, 'e', 5) <<
            QString("%1").arg(currentInducedTransformImag, 0, 'e', 5) <<
            QString("%1").arg(currentInducedVelocityReal, 0, 'e', 5) <<
            QString("%1").arg(currentInducedVelocityImag, 0, 'e', 5) <<
            QString("%1").arg(currentTotalReal, 0, 'e', 5) <<
            QString("%1").arg(currentTotalImag, 0, 'e', 5) <<
            QString("%1").arg(forceLorentzX, 0, 'e', 5) <<
            QString("%1").arg(forceLorentzY, 0, 'e', 5) <<
            QString("%1").arg(torque, 0, 'e', 5) <<
            QString("%1").arg(powerLosses, 0, 'e', 5) <<
            QString("%1").arg(energy, 0, 'e', 5);
    return QStringList(row);
}

// *************************************************************************************************************************************

void ViewScalarFilterMagnetic::calculateVariable(int i)
{
    switch (m_physicFieldVariable)
    {
    case PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL:
        {
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                node->values[0][0][i] = sqrt(sqr(value1[i]) + sqr(value2[i]));
            }
            else
            {
                node->values[0][0][i] = sqrt(sqr(value1[i]) + sqr(value2[i])) * x[i];
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_REAL:
        {
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                node->values[0][0][i] = value1[i];
            }
            else
            {
                node->values[0][0][i] = - value1[i] * x[i];
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_VECTOR_POTENTIAL_IMAG:
        {
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                node->values[0][0][i] = value2[i];
            }
            else
            {
                node->values[0][0][i] = - value2[i] * x[i];
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY:
        {
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                node->values[0][0][i] = sqrt(sqr(dudx1[i]) + sqr(dudx2[i]) + sqr(dudy1[i]) + sqr(dudy2[i]));
            }
            else
            {
                node->values[0][0][i] = sqrt(sqr(dudy1[i]) + sqr(dudy2[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)) + sqr(dudx2[i] + ((x > 0) ? value2[i] / x[i] : 0.0)));
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_REAL:
        {
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy1[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - dudx1[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy1[i]) + sqr(dudx1[i]));
                    }
                    break;
                }
            }
            else
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy1[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - dudx1[i] - ((x[i] > 0) ? value1[i] / x[i] : 0.0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy1[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)));
                    }
                    break;
                }
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_FLUX_DENSITY_IMAG:
        {
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy2[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - dudx2[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy2[i]) + sqr(dudx2[i]));
                    }
                    break;
                }
            }
            else
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy2[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - dudx2[i] - ((x > 0) ? value2[i] / x[i] : 0.0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy2[i]) + sqr(dudx2[i] + ((x > 0) ? value2[i] / x[i] : 0.0)));
                    }
                    break;
                }
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                node->values[0][0][i] = sqrt(sqr(dudx1[i]) + sqr(dudx2[i]) + sqr(dudy1[i]) + sqr(dudy2[i])) / (marker->permeability.number * MU0);
            }
            else
            {
                node->values[0][0][i] = sqrt(sqr(dudy1[i]) + sqr(dudy2[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)) + sqr(dudx2[i] + ((x > 0) ? value2[i] / x[i] : 0.0))) / (marker->permeability.number * MU0);
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_REAL:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy1[i] / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - dudx1[i] / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy1[i]) + sqr(dudx1[i])) / (marker->permeability.number * MU0);
                    }
                    break;
                }
            }
            else
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy1[i] / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - (dudx1[i] - ((x[i] > 0) ? value1[i] / x[i] : 0.0)) / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy1[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0))) / (marker->permeability.number * MU0);
                    }
                    break;
                }
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_MAGNETICFIELD_IMAG:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy2[i] / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - dudx2[i] / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy2[i]) + sqr(dudx2[i])) / (marker->permeability.number * MU0);
                    }
                    break;
                }
            }
            else
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = dudy2[i] / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = - (dudx2[i] - ((x > 0) ? value2[i] / x[i] : 0.0)) / (marker->permeability.number * MU0);
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(dudy2[i]) + sqr(dudx2[i] + ((x > 0) ? value2[i] / x[i] : 0.0))) / (marker->permeability.number * MU0);
                    }
                    break;
                }
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = sqrt(
                    sqr(marker->current_density_real.number) +
                    sqr(marker->current_density_imag.number));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_REAL:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->current_density_real.number;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_IMAG:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->current_density_imag.number;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_REAL:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
            {
                node->values[0][0][i] = 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i];
            }
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
            {
                node->values[0][0][i] = - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM_IMAG:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = - 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i];
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_TRANSFORM:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = sqrt(
                    sqr(2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) +
                    sqr(2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_REAL:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]);
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY_IMAG:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx2[i] +
                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy2[i]);
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_INDUCED_VELOCITY:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * sqrt(sqr(dudx1[i]) + sqr(dudx2[i])) +
                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * sqrt(sqr(dudy1[i]) + sqr(dudy2[i])));
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_REAL:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->current_density_real.number -
                                    marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                   (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]);
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                node->values[0][0][i] += 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i];

            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
                node->values[0][0][i] -= marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL_IMAG:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->current_density_imag.number +
                                    marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                   (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]);
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                node->values[0][0][i] += 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i];
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CURRENT_DENSITY_TOTAL:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = sqrt(
                    sqr(marker->current_density_real.number +
                        2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i] +
                        marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                       (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                    +
                    sqr(marker->current_density_imag.number +
                        2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i] +
                        marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx2[i] +
                                                       (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy2[i]))

                    );
        }
        break;    
    case PHYSICFIELDVARIABLE_MAGNETIC_POWER_LOSSES:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_STEADYSTATE)
            {
                node->values[0][0][i] = (marker->conductivity.number > 0.0) ?
                                        1.0 / marker->conductivity.number * sqr(
                                        marker->current_density_real.number +
                                        - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                        :
                                        0.0;
            }
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
            {
                // TODO: add velocity
                node->values[0][0][i] = (marker->conductivity.number > 0.0) ?
                                        0.5 / marker->conductivity.number * (
                                                sqr(marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) +
                                                sqr(marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]))
                                        :
                                        0.0;
            }
            if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_TRANSIENT)
            {
                node->values[0][0][i] = (marker->conductivity.number > 0.0) ?
                                        1.0 / marker->conductivity.number * sqr(
                                        marker->current_density_real.number +
                                        - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                        - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number)
                                        :
                                        0.0;
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_LORENTZ_FORCE:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);

            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                        {
                            node->values[0][0][i] = - 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudx1[i])
                                                    +          ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudx2[i]))
                                                    +
                                                    dudx1[i] * (marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                               (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]));
                        }
                        else
                        {
                            node->values[0][0][i] = (dudx1[i] * (
                                                                marker->current_density_real.number +
                                                                - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                                - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                                ));
                        }
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                        {
                            node->values[0][0][i] = - (0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                                    +           ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i]))
                                                    +
                                                    dudy1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                             (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])));
                        }
                        else
                        {
                            node->values[0][0][i] = (dudy1[i] * (
                                                                 marker->current_density_real.number +
                                                                 - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                  (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                                 - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                                 ));
                        }
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                        {
                            node->values[0][0][i] = sqrt(sqr(
                                                    0.5 * ( - ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudx1[i])
                                                            + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudx2[i]))
                                                    +
                                                    dudx1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                             (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                                    ) + sqr(
                                                            node->values[0][0][i] = - (0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                                                                    +           ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i]))
                                                                                    +
                                                                                    dudy1[i] * (- marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])))
                                                            ));
                        }
                        else
                        {
                            node->values[0][0][i] = sqrt(
                                                      sqr(dudx1[i] * (
                                                        marker->current_density_real.number +
                                                        - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                        - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                        ))
                                                    + sqr(dudy1[i] * (
                                                        marker->current_density_real.number +
                                                        - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                        - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                        )));
                        }

                    }
                    break;
                }
            }
            else
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                        {
                            node->values[0][0][i] = - 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * (dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)))
                                                             + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * (dudx2[i] + ((x[i] > 0) ? value2[i] / x[i] : 0.0))))
                                                    +
                                                    (dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)) * (marker->current_density_real.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                                     (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]));
                        }
                        else
                        {
                            node->values[0][0][i] = ((dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)) * (
                                                                marker->current_density_real.number +
                                                                - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                 (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                                - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                                ));
                        }
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                        {
                            node->values[0][0][i] = - 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                                             + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i]))
                                                    +
                                                    - dudy1[i] * (marker->current_density_real.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                                       (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]));
                        }
                        else
                        {
                            node->values[0][0][i] = (dudy1[i] * (
                                                                 marker->current_density_real.number +
                                                                 - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                  (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                                 - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                                 ));

                        }
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                        {
                            node->values[0][0][i] = sqrt(sqr(
                                                        - 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * (dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)))
                                                                 + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * (dudx2[i] + ((x[i] > 0) ? value2[i] / x[i] : 0.0))))
                                                        +
                                                        (dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)) * (marker->current_density_imag.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                                ) + sqr(
                                                        - 0.5 * (- ((marker->current_density_real.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value2[i]) * dudy1[i])
                                                                 + ((marker->current_density_imag.number + 2 * M_PI * Util::scene()->problemInfo()->frequency * marker->conductivity.number * value1[i]) * dudy2[i]))
                                                        +
                                                        - dudy1[i] * (marker->current_density_imag.number - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                                                                           (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i]))
                                                        ));
                        }
                        else
                        {
                            node->values[0][0][i] = sqrt(
                                                      sqr((dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0)) * (
                                                        marker->current_density_real.number +
                                                        - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                        - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                        ))
                                                    + sqr(dudy1[i] * (
                                                        marker->current_density_real.number +
                                                        - marker->conductivity.number * ((marker->velocity_x.number - marker->velocity_angular.number * y[i]) * dudx1[i] +
                                                                                         (marker->velocity_y.number + marker->velocity_angular.number * x[i]) * dudy1[i])
                                                        - marker->conductivity.number * (value1[i] - value2[i]) / Util::scene()->problemInfo()->timeStep.number
                                                        )));
                        }
                    }
                    break;
                }
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_ENERGY_DENSITY:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                node->values[0][0][i] = 0.25 * (sqr(dudx1[i]) + sqr(dudy1[i])) / (marker->permeability.number * MU0);
                if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                    node->values[0][0][i] += 0.25 * (sqr(dudx2[i]) + sqr(dudy2[i])) / (marker->permeability.number * MU0);
            }
            else
            {
                node->values[0][0][i] = 0.25 * (sqr(dudy1[i]) + sqr(dudx1[i] + ((x[i] > 0) ? value1[i] / x[i] : 0.0))) / (marker->permeability.number * MU0);
                if (Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC)
                    node->values[0][0][i] += 0.25 * (sqr(dudy2[i]) + sqr(dudx2[i] + ((x > 0) ? value2[i] / x[i] : 0.0))) / (marker->permeability.number * MU0);
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_PERMEABILITY:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->permeability.number;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_CONDUCTIVITY:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->conductivity.number;
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_VELOCITY:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            if (Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR)
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = marker->velocity_x.number - marker->velocity_angular.number * y[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = marker->velocity_y.number + marker->velocity_angular.number * x[i];
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = sqrt(sqr(marker->velocity_x.number - marker->velocity_angular.number * y[i]) +
                                                     sqr(marker->velocity_y.number + marker->velocity_angular.number * x[i]));
                    }
                    break;
                }
            }
            else
            {
                switch (m_physicFieldVariableComp)
                {
                case PHYSICFIELDVARIABLECOMP_X:
                    {
                        node->values[0][0][i] = 0;
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_Y:
                    {
                        node->values[0][0][i] = marker->velocity_y.number;
                    }
                    break;
                case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                    {
                        node->values[0][0][i] = fabs(marker->velocity_y.number);
                    }
                    break;
                }
            }
        }
        break;
    case PHYSICFIELDVARIABLE_MAGNETIC_REMANENCE:
        {
            SceneLabelMagneticMarker *marker = dynamic_cast<SceneLabelMagneticMarker *>(labelMarker);
            node->values[0][0][i] = marker->remanence.number;

            switch (m_physicFieldVariableComp)
            {
            case PHYSICFIELDVARIABLECOMP_X:
                {
                    node->values[0][0][i] = marker->remanence.number * cos(marker->remanence_angle.number / 180.0 * M_PI);
                }
                break;
            case PHYSICFIELDVARIABLECOMP_Y:
                {
                    node->values[0][0][i] = marker->remanence.number * sin(marker->remanence_angle.number / 180.0 * M_PI);
                }
                break;
            case PHYSICFIELDVARIABLECOMP_MAGNITUDE:
                {
                    node->values[0][0][i] = marker->remanence.number;
                }
                break;
            }
        }
        break;
    default:
        cerr << "Physical field variable '" + physicFieldVariableString(m_physicFieldVariable).toStdString() + "' is not implemented. ViewScalarFilterMagnetic::calculateVariable()" << endl;
        throw;
        break;
    }
}

// *************************************************************************************************************************************

SceneEdgeMagneticMarker::SceneEdgeMagneticMarker(const QString &name, PhysicFieldBC type, Value value_real, Value value_imag) : SceneEdgeMarker(name, type)
{
    this->value_real = value_real;
    this->value_imag = value_imag;
}

QString SceneEdgeMagneticMarker::script()
{
    return QString("addboundary(\"%1\", \"%2\", %3, %4)").
            arg(name).
            arg(physicFieldBCToStringKey(type)).
            arg(value_real.text).
            arg(value_imag.text);
}

QMap<QString, QString> SceneEdgeMagneticMarker::data()
{
    QMap<QString, QString> out;
    switch (type)
    {
    case PHYSICFIELDBC_MAGNETIC_VECTOR_POTENTIAL:
        out["Vector potential - real (Wb/m)"] = value_real.text;
        out["Vector potential - real (Wb/m)"] = value_imag.text;
        break;
    case PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT:
        out["Surface current - real (A/m)"] = value_real.text;
        out["Surface current - imag (A/m)"] = value_imag.text;
        break;
    }
    return QMap<QString, QString>(out);
}

int SceneEdgeMagneticMarker::showDialog(QWidget *parent)
{
    DSceneEdgeMagneticMarker *dialog = new DSceneEdgeMagneticMarker(this, parent);
    return dialog->exec();
}

// *************************************************************************************************************************************

SceneLabelMagneticMarker::SceneLabelMagneticMarker(const QString &name, Value current_density_real, Value current_density_imag, Value permeability, Value conductivity, Value remanence, Value remanence_angle, Value velocity_x, Value velocity_y, Value velocity_angular)
    : SceneLabelMarker(name)
{
    this->permeability = permeability;
    this->conductivity = conductivity;
    this->current_density_real = current_density_real;
    this->current_density_imag = current_density_imag;
    this->remanence = remanence;
    this->remanence_angle = remanence_angle;
    this->velocity_x = velocity_x;
    this->velocity_y = velocity_y;
    this->velocity_angular = velocity_angular;
}

QString SceneLabelMagneticMarker::script()
{
    return QString("addmaterial(\"%1\", %2, %3, %4, %5, %6, %7, %8, %9, %10)").
            arg(name).
            arg(current_density_real.text).
            arg(current_density_imag.text).
            arg(permeability.text).
            arg(conductivity.text).
            arg(remanence.text).
            arg(remanence_angle.text).
            arg(velocity_x.text).
            arg(velocity_y.text).
            arg(velocity_angular.text);
}

QMap<QString, QString> SceneLabelMagneticMarker::data()
{
    QMap<QString, QString> out;
    out["Current density - real (A/m2)"] = current_density_real.text;
    out["Current density - imag (A/m2)"] = current_density_imag.text;
    out["Permeability (-)"] = permeability.text;
    out["Conductivity (S/m)"] = conductivity.text;
    out["Remanence (T)"] = remanence.text;
    out["Remanence angle (-)"] = remanence_angle.text;
    out["Velocity x (m/s)"] = velocity_x.text;
    out["Velocity y (m/s)"] = velocity_y.text;
    out["Angular velocity (m/s)"] = velocity_angular.text;
    return QMap<QString, QString>(out);
}

int SceneLabelMagneticMarker::showDialog(QWidget *parent)
{
    DSceneLabelMagneticMarker *dialog = new DSceneLabelMagneticMarker(parent, this);
    return dialog->exec();
}

// *************************************************************************************************************************************

DSceneEdgeMagneticMarker::DSceneEdgeMagneticMarker(SceneEdgeMagneticMarker *edgeMagneticMarker, QWidget *parent) : DSceneEdgeMarker(parent)
{
    m_edgeMarker = edgeMagneticMarker;

    createDialog();

    load();
    setSize();
}

DSceneEdgeMagneticMarker::~DSceneEdgeMagneticMarker()
{
    delete cmbType;
    delete txtValueReal;
    delete txtValueImag;
}

void DSceneEdgeMagneticMarker::createContent()
{
    cmbType = new QComboBox(this);
    cmbType->addItem("none", PHYSICFIELDBC_NONE);
    cmbType->addItem(physicFieldBCString(PHYSICFIELDBC_MAGNETIC_VECTOR_POTENTIAL), PHYSICFIELDBC_MAGNETIC_VECTOR_POTENTIAL);
    cmbType->addItem(physicFieldBCString(PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT), PHYSICFIELDBC_MAGNETIC_SURFACE_CURRENT);

    txtValueReal = new SLineEditValue(this);
    txtValueImag = new SLineEditValue(this);
    txtValueImag->setEnabled(Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC);

    connect(txtValueReal, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtValueImag, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));

    QHBoxLayout *layoutCurrentDensity = new QHBoxLayout();
    layoutCurrentDensity->addWidget(txtValueReal);
    layoutCurrentDensity->addWidget(new QLabel(" + j "));
    layoutCurrentDensity->addWidget(txtValueImag);

    layout->addWidget(new QLabel(tr("BC type:")), 1, 0);
    layout->addWidget(cmbType, 1, 1);
    layout->addWidget(new QLabel(tr("Value:")), 2, 0);
    layout->addLayout(layoutCurrentDensity, 2, 1);
}

void DSceneEdgeMagneticMarker::load()
{
    DSceneEdgeMarker::load();

    SceneEdgeMagneticMarker *edgeMagneticMarker = dynamic_cast<SceneEdgeMagneticMarker *>(m_edgeMarker);

    cmbType->setCurrentIndex(cmbType->findData(edgeMagneticMarker->type));
    txtValueReal->setValue(edgeMagneticMarker->value_real);
    txtValueImag->setValue(edgeMagneticMarker->value_imag);
}

bool DSceneEdgeMagneticMarker::save() {
    if (!DSceneEdgeMarker::save()) return false;;

    SceneEdgeMagneticMarker *edgeMagneticMarker = dynamic_cast<SceneEdgeMagneticMarker *>(m_edgeMarker);

    edgeMagneticMarker->type = (PhysicFieldBC) cmbType->itemData(cmbType->currentIndex()).toInt();

    if (txtValueReal->evaluate())
        edgeMagneticMarker->value_real  = txtValueReal->value();
    else
        return false;

    if (txtValueImag->evaluate())
        edgeMagneticMarker->value_imag  = txtValueImag->value();
    else
        return false;

    return true;
}

// *************************************************************************************************************************************

DSceneLabelMagneticMarker::DSceneLabelMagneticMarker(QWidget *parent, SceneLabelMagneticMarker *labelMagneticMarker) : DSceneLabelMarker(parent)
{
    m_labelMarker = labelMagneticMarker;

    createDialog();

    load();
    setSize();
}

DSceneLabelMagneticMarker::~DSceneLabelMagneticMarker()
{
    delete txtPermeability;
    delete txtConductivity;
    delete txtCurrentDensityReal;
    delete txtCurrentDensityImag;
}

void DSceneLabelMagneticMarker::createContent()
{
    txtPermeability = new SLineEditValue(this);
    txtConductivity = new SLineEditValue(this);
    txtCurrentDensityReal = new SLineEditValue(this);
    txtCurrentDensityImag = new SLineEditValue(this);
    txtCurrentDensityImag->setEnabled(Util::scene()->problemInfo()->analysisType == ANALYSISTYPE_HARMONIC);
    txtRemanence = new SLineEditValue(this);
    txtRemanenceAngle = new SLineEditValue(this);
    txtVelocityX = new SLineEditValue(this);
    txtVelocityX->setEnabled(Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR);
    txtVelocityY = new SLineEditValue(this);
    txtVelocityAngular = new SLineEditValue(this);
    txtVelocityAngular->setEnabled(Util::scene()->problemInfo()->problemType == PROBLEMTYPE_PLANAR);

    connect(txtPermeability, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtConductivity, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtCurrentDensityReal, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtCurrentDensityImag, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtRemanence, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtRemanenceAngle, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtVelocityX, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtVelocityY, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));
    connect(txtVelocityAngular, SIGNAL(evaluated(bool)), this, SLOT(evaluated(bool)));

    QHBoxLayout *layoutCurrentDensity = new QHBoxLayout();
    layoutCurrentDensity->addWidget(txtCurrentDensityReal);
    layoutCurrentDensity->addWidget(new QLabel(" + j "));
    layoutCurrentDensity->addWidget(txtCurrentDensityImag);

    // remanence
    QFormLayout *layoutRemanence = new QFormLayout();
    layoutRemanence->addRow(tr("Rem. flux dens. (T):"), txtRemanence);
    layoutRemanence->addRow(tr("Direction of rem. (deg.):"), txtRemanenceAngle);

    QGroupBox *grpRemanence = new QGroupBox(tr("Permanent magnet"), this);
    grpRemanence->setLayout(layoutRemanence);

    // velocity
    QFormLayout *layoutVelocity = new QFormLayout();
    layoutVelocity->addRow(tr("Velocity %1 (m/s):").arg(Util::scene()->problemInfo()->labelX().toLower()), txtVelocityX);
    layoutVelocity->addRow(tr("Velocity %1 (m/s):").arg(Util::scene()->problemInfo()->labelY().toLower()), txtVelocityY);
    layoutVelocity->addRow(tr("Velocity angular (rad/s):"), txtVelocityAngular);

    QGroupBox *grpVelocity = new QGroupBox(tr("Velocity"), this);
    grpVelocity->setLayout(layoutVelocity);

    layout->addWidget(new QLabel(tr("Permeability (-):")), 1, 0);
    layout->addWidget(txtPermeability, 1, 1);
    layout->addWidget(new QLabel(tr("Conductivity (S/m):")), 2, 0);
    layout->addWidget(txtConductivity, 2, 1);
    layout->addWidget(new QLabel(tr("Current density (A/m2):")), 3, 0);
    layout->addLayout(layoutCurrentDensity, 3, 1);
    layout->addWidget(grpRemanence, 4, 0, 1, 2);
    layout->addWidget(grpVelocity, 5, 0, 1, 2);
}

void DSceneLabelMagneticMarker::load()
{
    DSceneLabelMarker::load();

    SceneLabelMagneticMarker *labelMagneticMarker = dynamic_cast<SceneLabelMagneticMarker *>(m_labelMarker);

    txtPermeability->setValue(labelMagneticMarker->permeability);
    txtConductivity->setValue(labelMagneticMarker->conductivity);
    txtCurrentDensityReal->setValue(labelMagneticMarker->current_density_real);
    txtCurrentDensityImag->setValue(labelMagneticMarker->current_density_imag);
    txtRemanence->setValue(labelMagneticMarker->remanence);
    txtRemanenceAngle->setValue(labelMagneticMarker->remanence_angle);
    txtVelocityX->setValue(labelMagneticMarker->velocity_x);
    txtVelocityY->setValue(labelMagneticMarker->velocity_y);
    txtVelocityAngular->setValue(labelMagneticMarker->velocity_angular);
}

bool DSceneLabelMagneticMarker::save() {
    if (!DSceneLabelMarker::save()) return false;;

    SceneLabelMagneticMarker *labelMagneticMarker = dynamic_cast<SceneLabelMagneticMarker *>(m_labelMarker);

    if (txtPermeability->evaluate())
        labelMagneticMarker->permeability  = txtPermeability->value();
    else
        return false;

    if (txtConductivity->evaluate())
        labelMagneticMarker->conductivity  = txtConductivity->value();
    else
        return false;

    if (txtCurrentDensityReal->evaluate())
        labelMagneticMarker->current_density_real  = txtCurrentDensityReal->value();
    else
        return false;

    if (txtCurrentDensityImag->evaluate())
        labelMagneticMarker->current_density_imag  = txtCurrentDensityImag->value();
    else
        return false;

    if (txtRemanence->evaluate())
        labelMagneticMarker->remanence = txtRemanence->value();
    else
        return false;

    if (txtRemanenceAngle->evaluate())
        labelMagneticMarker->remanence_angle = txtRemanenceAngle->value();
    else
        return false;

    if (txtVelocityX->evaluate())
        labelMagneticMarker->velocity_x = txtVelocityX->value();
    else
        return false;

    if (txtVelocityY->evaluate())
        labelMagneticMarker->velocity_y = txtVelocityY->value();
    else
        return false;

    if (txtVelocityAngular->evaluate())
        labelMagneticMarker->velocity_angular = txtVelocityAngular->value();
    else
        return false;

    return true;
}
