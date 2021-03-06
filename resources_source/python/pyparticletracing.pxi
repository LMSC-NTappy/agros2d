cdef extern from "../../agros2d-library/pythonlab/pyparticletracing.h":
    cdef cppclass PyParticleTracing:
        PyParticleTracing()

        void getInitialPosition(vector[double] &position)
        void setInitialPosition(vector[double] &position) except +

        void getInitialVelocity(vector[double] &velocity)
        void setInitialVelocity(vector[double] &velocity)

        void setNumberOfParticles(int particles)  except +
        int getNumberOfParticles()

        void setStartingRadius(double radius) except +
        double getStartingRadius()

        double getParticleMass()
        void setParticleMass(double mass) except +

        double getParticleCharge()
        void setParticleCharge(double charge)

        double getDragForceDensity()
        void setDragForceDensity(double rho) except +
        double getDragForceReferenceArea()
        void setDragForceReferenceArea(double area) except +
        double getDragForceCoefficient()
        void setDragForceCoefficient(double coeff) except +

        void getCustomForce(vector[double] &force) except +
        void setCustomForce(vector[double] &force) except +

        bool getElectrostaticInteraction()
        void setElectrostaticInteraction(bool interaction)
        bool getMagneticInteraction()
        void setMagneticInteraction(bool interaction)

        bool getIncludeRelativisticCorrection()
        void setIncludeRelativisticCorrection(bool incl)

        string getButcherTableType()
        void setButcherTableType(string &tableType) except +
        double getMaximumRelativeError()
        void setMaximumRelativeError(double tolerance) except +
        int getMaximumNumberOfSteps()
        void setMaximumNumberOfSteps(int steps) except +
        double getMaximumStep()
        void setMaximumStep(double step) except +

        bool getReflectOnDifferentMaterial()
        void setReflectOnDifferentMaterial(bool reflect)
        bool getReflectOnBoundary()
        void setReflectOnBoundary(bool reflect)
        double getCoefficientOfRestitution()
        void setCoefficientOfRestitution(double coeff)  except +

        bool getColorByVelocity()
        void setColorByVelocity(bool show)
        bool getShowPoints()
        void setShowPoints(bool show)
        bool getShowBlendedFaces()
        void setShowBlendedFaces(bool show)
        int getNumShowParticlesAxi()
        void setNumShowParticlesAxi(int particles)  except +

        void solve(vector[vector[double]] &initialPositions,  vector[vector[double]] &initialVelocities,
                   vector[double] &particleCharges, vector[double] &particleMasses) except +

        int length()
        void positions(vector[vector[double]] &x, vector[vector[double]] &y, vector[vector[double]] &z)
        void velocities(vector[vector[double]] &vx, vector[vector[double]] &vy, vector[vector[double]] &vz)
        void times(vector[vector[double]] &t)

cdef class __ParticleTracing__:
    cdef PyParticleTracing *thisptr

    def __cinit__(self):
        self.thisptr = new PyParticleTracing()
    def __dealloc__(self):
        del self.thisptr

    def solve(self, initial_positions = [], initial_velocities = [],
                    particle_charges = [], particle_masses = []):

        cdef vector[vector[double]] initial_positions_vector, initial_velocities_vector

        for position in initial_positions:
            initial_positions_vector.push_back(list_to_double_vector(position))

        for velocity in initial_velocities:
            initial_velocities_vector.push_back(list_to_double_vector(velocity))

        self.thisptr.solve(initial_positions_vector,
                           initial_velocities_vector,
                           list_to_double_vector(particle_charges),
                           list_to_double_vector(particle_masses))

    """
    def length(self):
        return self.thisptr.length()
    """

    def positions(self):
        cdef vector[vector[double]] x, y, z
        self.thisptr.positions(x, y, z)

        assert x.size() == y.size() == z.size()

        out = [[], [], []]
        for i in range(x.size()):
            out[0].append(double_vector_to_list(x[i]))
            out[1].append(double_vector_to_list(y[i]))
            out[2].append(double_vector_to_list(z[i]))

        return out

    def velocities(self):
        cdef vector[vector[double]] vx, vy, vz
        self.thisptr.velocities(vx, vy, vz)

        assert vx.size() == vy.size() == vz.size()

        out = [[], [], []]
        for i in range(vx.size()):
            out[0].append(double_vector_to_list(vx[i]))
            out[1].append(double_vector_to_list(vy[i]))
            out[2].append(double_vector_to_list(vz[i]))

        return out

    def times(self):
        cdef vector[vector[double]] t
        self.thisptr.times(t)

        out = []
        for i in range(t.size()):
            out.append(double_vector_to_list(t[i]))

        return out

    property number_of_particles:
        def __get__(self):
            return self.thisptr.getNumberOfParticles()
        def __set__(self, particles):
            self.thisptr.setNumberOfParticles(particles)

    property particles_dispersion:
        def __get__(self):
            return self.thisptr.getStartingRadius()
        def __set__(self, dispersion):
            self.thisptr.setStartingRadius(dispersion)

    property initial_position:
        def __get__(self):
            cdef vector[double] position
            self.thisptr.getInitialPosition(position)
            return double_vector_to_list(position)
        def __set__(self, position):
            self.thisptr.setInitialPosition(list_to_double_vector(position))

    property initial_velocity:
        def __get__(self):
            cdef vector[double] velocity
            self.thisptr.getInitialVelocity(velocity)
            return double_vector_to_list(velocity)
        def __set__(self, velocity):
            self.thisptr.setInitialVelocity(list_to_double_vector(velocity))

    property mass:
        def __get__(self):
            return self.thisptr.getParticleMass()
        def __set__(self, mass):
            self.thisptr.setParticleMass(mass)

    property charge:
        def __get__(self):
            return self.thisptr.getParticleCharge()
        def __set__(self, charge):
            self.thisptr.setParticleCharge(charge)

    property include_relativistic_correction:
        def __get__(self):
            return self.thisptr.getIncludeRelativisticCorrection()
        def __set__(self, correction):
            self.thisptr.setIncludeRelativisticCorrection(correction)

    property reflect_on_different_material:
        def __get__(self):
            return self.thisptr.getReflectOnDifferentMaterial()
        def __set__(self, reflect):
            self.thisptr.setReflectOnDifferentMaterial(reflect)

    property reflect_on_boundary:
        def __get__(self):
            return self.thisptr.getReflectOnBoundary()
        def __set__(self, reflect):
            self.thisptr.setReflectOnBoundary(reflect)

    property coefficient_of_restitution:
        def __get__(self):
            return self.thisptr.getCoefficientOfRestitution()
        def __set__(self, coeff):
            self.thisptr.setCoefficientOfRestitution(coeff)

    property drag_force_density:
        def __get__(self):
            return self.thisptr.getDragForceDensity()
        def __set__(self, density):
            self.thisptr.setDragForceDensity(density)

    property drag_force_reference_area:
        def __get__(self):
            return self.thisptr.getDragForceReferenceArea()
        def __set__(self, area):
            self.thisptr.setDragForceReferenceArea(area)

    property drag_force_coefficient:
        def __get__(self):
            return self.thisptr.getDragForceCoefficient()
        def __set__(self, coeff):
            self.thisptr.setDragForceCoefficient(coeff)

    property custom_force:
        def __get__(self):
            cdef vector[double] force
            self.thisptr.getCustomForce(force)
            return double_vector_to_list(force)
        def __set__(self, force):
            self.thisptr.setCustomForce(list_to_double_vector(force))

    property electrostatic_interaction:
        def __get__(self):
            return self.thisptr.getElectrostaticInteraction()
        def __set__(self, interaction):
            self.thisptr.setElectrostaticInteraction(interaction)

    property magnetic_interaction:
        def __get__(self):
            return self.thisptr.getMagneticInteraction()
        def __set__(self, interaction):
            self.thisptr.setMagneticInteraction(interaction)

    property butcher_table_type:
        def __get__(self):
            return self.thisptr.getButcherTableType().c_str()
        def __set__(self, table_type):
            self.thisptr.setButcherTableType(string(table_type))

    property maximum_number_of_steps:
        def __get__(self):
            return self.thisptr.getMaximumNumberOfSteps()
        def __set__(self, steps):
            self.thisptr.setMaximumNumberOfSteps(steps)

    property maximum_relative_error:
        def __get__(self):
            return self.thisptr.getMaximumRelativeError()
        def __set__(self, tolerance):
            self.thisptr.setMaximumRelativeError(tolerance)

    property maximum_step:
        def __get__(self):
            return self.thisptr.getMaximumStep()
        def __set__(self, step):
            self.thisptr.setMaximumStep(step)

    property collor_by_velocity:
        def __get__(self):
            return self.thisptr.getColorByVelocity()
        def __set__(self, show):
            self.thisptr.setColorByVelocity(show)

    property show_points:
        def __get__(self):
            return self.thisptr.getShowPoints()
        def __set__(self, show):
            self.thisptr.setShowPoints(show)

    property blended_faces:
        def __get__(self):
            return self.thisptr.getShowBlendedFaces()
        def __set__(self, show):
            self.thisptr.setShowBlendedFaces(show)

    property multiple_show_particles:
        def __get__(self):
            return self.thisptr.getNumShowParticlesAxi()
        def __set__(self, particles):
            self.thisptr.setNumShowParticlesAxi(particles)

particle_tracing = __ParticleTracing__()
