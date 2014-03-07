cdef extern from "../../agros2d-library/pythonlab/pygeometry.h":
    cdef cppclass PyGeometry:
        PyGeometry()

        void activate()

        int addNode(double x, double y) except +
        int addEdge(double x1, double y1, double x2, double y2, double angle, int segments, int curvilinear, map[string, int] &refinements, map[string, string] &boundaries) except +
        int addEdgeByNodes(int nodeStartIndex, int nodeEndIndex, double angle, int segments, int curvilinear, map[string, int] &refinements, map[string, string] &boundaries) except +
        int addLabel(double x, double y, double area, map[string, int] &refinements, map[string, int] &orders, map[string, string] &materials) except +

        void modifyEdge(int index, double angle, int segments, int curvilinear, map[string, int] &refinements, map[string, string] &boundaries) except +
        void modifyLabel(int index, double area, map[string, int] &refinements, map[string, int] &orders, map[string, string] &materials) except +

        int nodesCount()
        int edgesCount()
        int labelsCount()

        void removeNodes(vector[int] &nodes) except +
        void removeEdges(vector[int] &edges) except +
        void removeLabels(vector[int] &labels) except +

        void selectNodes(vector[int] &nodes) except +
        void selectEdges(vector[int] &edges) except +
        void selectLabels(vector[int] &labels) except +

        void selectNodeByPoint(double x, double y) except +
        void selectEdgeByPoint(double x, double y) except +
        void selectLabelByPoint(double x, double y) except +

        void selectNone()

        void moveSelection(double dx, double dy, bool copy, bool withMarkers)
        void rotateSelection(double x, double y, double angle, bool copy, bool withMarkers)
        void scaleSelection(double x, double y, double scale, bool copy, bool withMarkers)
        void removeSelection()

        void exportVTK(string filename)
        void exportSVG(string filename)
        string exportSVG()

cdef class __Geometry__:
    cdef PyGeometry *thisptr

    def __cinit__(self):
        self.thisptr = new PyGeometry()
    def __dealloc__(self):
        del self.thisptr

    def activate(self):
        """Activate preprocessor mode."""
        self.thisptr.activate()

    def add_node(self, x, y):
        """Add a new node according to coordinates and return its index.

        add_node(x, y)

        Keyword arguments:
        x -- x or r coordinate of node
        y -- y or z coordinate of node
        """
        return self.thisptr.addNode(x, y)

    def remove_nodes(self, nodes = []):
        """Remove nodes according to their indexes.

        remove_nodes(nodes = [])

        Keyword arguments:
        nodes -- list of indexes of removed nodes (default [] - remove all nodes)
        """
        cdef vector[int] nodes_vector = list_to_int_vector(nodes)
        self.thisptr.removeNodes(nodes_vector)

    def add_edge(self, x1, y1, x2, y2, angle = 0.0, segments = 3, curvilinear = True, refinements = {}, boundaries = {}):
        """Add a new edge according to coordinates and return its index.

        add_edge(x1, y1, x2, y2, angle = 0.0, segments = 3, curvilinear = True, refinements = {}, boundaries = {})

        Keyword arguments:
        x1 -- x or r coordinate of start node
        y1 -- y or z coordinate of start node
        x2 -- x or r coordinate of end node
        y2 -- y or z coordinate of end node
        angle -- angle between connecting lines, which join terminal nodes of edge and center of arc (default 0.0)
        refinements -- refinement towards edge {'field' : refinement} (default {})
        boundaries -- boundary condition {'field' : 'boundary name'} (default {})
        """
        cdef map[string, int] refinements_map = dictionary_to_int_map(refinements)
        cdef map[string, string] boundaries_map = dictionary_to_string_map(boundaries)

        return self.thisptr.addEdge(x1, y1, x2, y2, angle, segments, curvilinear, refinements_map, boundaries_map)

    def add_edge_by_nodes(self, start_node_index, end_node_index, angle = 0.0, segments = 3, curvilinear = True, refinements = {}, boundaries = {}):
        """Add a new edge according to indexes of start and end node and return the index of edge.

        add_edge(start_node_index, end_node_index, angle = 0.0, segments = 3, refinements = {}, boundaries = {})

        Keyword arguments:
        start_node_index -- index of start node
        end_node_index -- index of end node
        angle -- angle between connecting lines, which join terminal nodes of edge and center of arc (default 0.0)
        refinements -- refinement towards edge {'field' : refinement} (default {})
        boundaries -- boundary condition {'field' : 'boundary name'} (default {})
        """
        cdef map[string, int] refinements_map = dictionary_to_int_map(refinements)
        cdef map[string, string] boundaries_map = dictionary_to_string_map(boundaries)

        return self.thisptr.addEdgeByNodes(start_node_index, end_node_index, angle, segments, curvilinear, refinements_map, boundaries_map)

    def modify_edge(self, index, angle = 0.0, segments = 3, curvilinear = True, refinements = {}, boundaries = {}):
        """Modify parameters of existing edge.

        modify_edge(index, angle = 0.0, segments = 3, curvilinear = True, refinements = {}, boundaries = {})

        Keyword arguments:
        index -- edge index
        angle -- angle between connecting lines, which join terminal nodes of edge and center of arc (default 0.0)
        refinements -- refinement towards edge {'field' : refinement} (default {})
        boundaries -- boundary condition {'field' : 'boundary name'} (default {})
        """
        cdef map[string, int] refinements_map = dictionary_to_int_map(refinements)
        cdef map[string, string] boundaries_map = dictionary_to_string_map(boundaries)

        self.thisptr.modifyEdge(index, angle, segments, curvilinear, refinements_map, boundaries_map)

    def remove_edges(self, edges = []):
        """Remove edges according to their indexes.

        remove_edges(edges = [])

        Keyword arguments:
        edges -- list of indexes of removed edges (default [] - remove all edges)
        """
        cdef vector[int] edges_vector = list_to_int_vector(edges)

        self.thisptr.removeEdges(edges_vector)

    def add_label(self, x, y, area = 0.0, refinements = {}, orders = {}, materials = {}):
        """Add a new label according to coordinates and return its index.

        add_label(x, y, area = 0.0, refinements = {}, orders = {}, materials = {})

        Keyword arguments:
        x -- x or r coordinate of label
        y -- y or z coordinate of label
        area -- cross section of circle which influence maximum cross section of elements (default 0.0)
        refinements -- refinement of area {'field' : refinement} (default {})
        orders -- polynomial order of area elements {'field' : refinement} (default {})
        materials -- materials {'field' : 'material name'} (default {})
        """
        cdef map[string, int] refinements_map = dictionary_to_int_map(refinements)
        cdef map[string, int] orders_map = dictionary_to_int_map(orders)
        cdef map[string, string] materials_map = dictionary_to_string_map(materials)

        return self.thisptr.addLabel(x, y, area, refinements_map, orders_map, materials_map)

    def modify_label(self, index, area = 0.0, refinements = {}, orders = {}, materials = {}):
        """Modify parameters of existing label.

        modify_label(index, area = 0.0, refinements = {}, orders = {}, materials = {})

        Keyword arguments:
        index -- label index
        area -- cross section of circle which influence maximum cross section of elements (default 0.0)
        refinements -- refinement of area {'field' : refinement} (default {})
        orders -- polynomial order of area elements {'field' : refinement} (default {})
        materials -- materials {'field' : 'material name'} (default {})
        """
        cdef map[string, int] refinements_map = dictionary_to_int_map(refinements)
        cdef map[string, int] orders_map = dictionary_to_int_map(orders)
        cdef map[string, string] materials_map = dictionary_to_string_map(materials)

        self.thisptr.modifyLabel(index, area, refinements_map, orders_map, materials_map)

    def remove_labels(self, labels = []):
        """Remove labels according to their indexes.

        remove_labels(labels = [])

        Keyword arguments:
        labels -- list of indexes of removed labels (default [] - remove all labels)
        """
        cdef vector[int] labels_vector = list_to_int_vector(labels)

        self.thisptr.removeLabels(labels_vector)

    def add_rect(self, x0, y0, width, height, boundaries = {}, materials = None):
        """Add rect by start point (lower left node), width and height.

        add_rect(x0, y0, width, height, boundaries = {}, materials = None)

        Keyword arguments:
        x0 -- x or r coordinate of start point
        y0 -- y or z coordinate of start points
        width -- width of rect
        height -- height of rect
        boundaries -- boundaries {'field' : refinement} (default {})
        materials -- materials {'field' : 'material name'} (default None)
        """
        self.add_edge(x0, y0, (x0)+width, y0, boundaries=boundaries)
        self.add_edge(x0+width, y0, (x0)+width, (y0)+height, boundaries=boundaries)
        self.add_edge(x0+width, (y0)+height, x0, (y0)+height, boundaries=boundaries)
        self.add_edge(x0, (y0)+height, x0, y0, boundaries=boundaries)

        if (materials != None):
            self.add_label((x0)+(width/2.0), (y0)+(height/2.0), materials=materials)

    def add_circle(self, x0, y0, radius, boundaries = {}, materials = None):
        """Add circle by center point and radius.

        add_circle(self, x0, y0, radius, boundaries = {}, materials = None)

        Keyword arguments:
        x0 -- x or r coordinate of center point
        y0 -- y or z coordinate of center point
        radius -- radius of center
        boundaries -- boundaries {'field' : refinement} (default {})
        materials -- materials {'field' : 'material name'} (default None)
        """
        self.add_edge(x0, (y0)-radius, (x0)+radius, y0, boundaries=boundaries, angle=90)
        self.add_edge((x0)+radius, y0, x0, (y0)+radius, boundaries=boundaries, angle=90)
        self.add_edge(x0, (y0)+radius, (x0)-radius, y0, boundaries=boundaries, angle=90)
        self.add_edge((x0)-radius, y0, x0, (y0)-radius, boundaries=boundaries, angle=90)

        if (materials != None):
            geometry.add_label(x0, y0, materials=materials)

    def add_semicircle(self, x0, y0, radius, boundaries = {}, materials = None):
        """Add semicircle by center point of full circle and radius.

        add_semicircle(self, x0, y0, radius, boundaries = {}, materials = None)

        Keyword arguments:
        x0 -- x or r coordinate of center point
        y0 -- y or z coordinate of center point
        radius -- radius of center
        boundaries -- boundaries {'field' : refinement} (default {})
        materials -- materials {'field' : 'material name'} (default None)
        """
        self.add_edge(x0, (y0)-radius, (x0)+radius, y0, boundaries=boundaries, angle=90)
        self.add_edge((x0)+radius, y0, x0, (y0)+radius, boundaries=boundaries, angle=90)
        self.add_edge(x0, (y0)+radius, x0, (y0)-radius, boundaries=boundaries, angle=0)

        if (materials != None):
            self.add_label((x0)+(radius/2.0), y0, materials=materials)

    def nodes_count(self):
        """Return count of existing nodes."""
        return self.thisptr.nodesCount()

    def edges_count(self):
        """Return count of existing edges."""
        return self.thisptr.edgesCount()

    def labels_count(self):
        """Return count of existing labels."""
        return self.thisptr.labelsCount()

    def select_nodes(self, nodes = []):
        """Select nodes according to their indexes.

        select_nodes(nodes = [])

        Keyword arguments:
        nodes -- list of indexes (default is [] - select all nodes)
        """
        cdef vector[int] nodes_vector = list_to_int_vector(nodes)

        self.thisptr.selectNodes(nodes_vector)

    def select_edges(self, edges = []):
        """Select edges according to their indexes.

        select_edges(edges = [])

        Keyword arguments:
        edges -- list of indexes (default is []  - select all edges)
        """
        cdef vector[int] edges_vector = list_to_int_vector(edges)

        self.thisptr.selectEdges(edges_vector)

    def select_labels(self, labels = []):
        """Select labels according to their indexes.

        select_labels(labels = [])

        Keyword arguments:
        labels -- list of indexes (default is [] - select all labels)
        """
        cdef vector[int] labels_vector = list_to_int_vector(labels)

        self.thisptr.selectLabels(labels_vector)

    def select_node_by_point(self, x, y):
        """Select the closest node around the point.

        select_node_by_point(x, y)

        Keyword arguments:
        x -- x or r coordinate of point
        y -- y or r coordinate of point
        """
        self.thisptr.selectNodeByPoint(x, y)

    def select_edge_by_point(self, x, y):
        """Select the closest edge around the point.

        select_edge_by_point(x, y)

        Keyword arguments:
        x -- x or r coordinate of point
        y -- y or r coordinate of point
        """
        self.thisptr.selectEdgeByPoint(x, y)

    def select_label_by_point(self, x, y):
        """Select the closest label around the point.

        select_label_by_point(x, y)

        Keyword arguments:
        x -- x or r coordinate of point
        y -- y or r coordinate of point
        """
        self.thisptr.selectLabelByPoint(x, y)

    def move_selection(self, dx, dy, int copy = False, int withMarkers = True):
        """Move selected object (nodes, edges or labels).

        move_selection(dx, dy, copy = False, withMarkers = True)

        Keyword arguments:
        dx -- movement in x axis
        dy -- movement in y axis
        copy -- make a copy of moved objects (default is False)
        withMarkers -- copy with materials or boundary conditions (default is True)
        """
        self.thisptr.moveSelection(dx, dy, int(copy), int(withMarkers))

    def rotate_selection(self, x, y, angle, copy = False, int withMarkers = True):
        """Rotate selected object (nodes, edges or labels) around the point.

        rotate_selection(x, y, angle, copy = False, withMarkers = True)

        Keyword arguments:
        x -- x or r coordinate of point
        y -- y or r coordinate of point
        angle -- angle of rotation
        copy -- make a copy of moved objects (default is False)
        withMarkers -- copy with materials or boundary conditions (default is True)
        """
        self.thisptr.rotateSelection(x, y, angle, int(copy), int(withMarkers))

    def scale_selection(self, x, y, scale, copy = False, int withMarkers = True):
        """Scale selected object (nodes, edges or labels).

        scale_selection(x, y, scale, copy = False, withMarkers = True)

        Keyword arguments:
        x -- x or r coordinate of point
        y -- y or r coordinate of point
        scale -- scale factor
        copy -- make a copy of moved objects (default is False)
        withMarkers -- copy with materials or boundary conditions (default is True)
        """
        self.thisptr.scaleSelection(x, y, scale, int(copy), int(withMarkers))

    def remove_selection(self):
        """Remove selected objects (nodes, edges or labels)."""
        self.thisptr.removeSelection()

    def select_none(self):
        """Unselect all objects (nodes, edges or labels)."""
        self.thisptr.selectNone()

    def export_vtk(self, filename):
        """Export geometry in VTK format."""
        self.thisptr.exportVTK(filename)

    def export_svg(self, filename):
        """Export geometry in SVG format."""
        self.thisptr.exportSVG(filename)

    def export_svg_image(self):
        """Export geometry in SVG format."""
        return self.thisptr.exportSVG()

geometry = __Geometry__()
