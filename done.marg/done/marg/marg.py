'''
This module contains the definition of the core Marg and Connection module
which are using to reflect the graph visitor and edge properties
'''
import datetime
import time

from bunch import Bunch

from graph_tool import Graph, Vertex
from graph_tool.search import astar_search, AStarVisitor
from graph_tool.topology import shortest_path as gt_shortest_path

from .utils import get_time_delta, time_object


class Connection(object):
    '''
    A class to represent a connection.
    '''
    def __init__(self):
        '''
        Initialize connection attributes
        '''
        self.__departure = None
        self.__duration = None
        self.__index = None
        self.__traversal_cost = None
        self.__cost = 0.0
        self.__enabled = True

    def __set_duration(self, duration):
        '''
        Sets the duration for this connection
        '''
        if not isinstance(duration, int):
            raise TypeError(
                'Invalid duration{}'
                'Expected int, got {}'.format(
                    duration, type(duration)))
        self.__duration = duration

    def __set_departure(self, departure):
        '''
        Sets the departure time for this connection
        '''
        if type(departure) not in [datetime.time, time.struct_time]:
            raise TypeError(
                'Invalid departure {}'
                'Expected datetime.time or time.struct_time, got {}'.format(
                    departure, type(departure)))
        self.__departure = departure

    def __set_index(self, index):
        '''
        Sets a unique identifier for this connection
        '''
        self.__index = index

    def __get_duration(self):
        '''
        Fetch the arrival time for this connection
        '''
        return self.__duration

    def __get_departure(self):
        '''
        Fetch the departure time for this connection
        '''
        return self.__departure

    def __get_index(self):
        '''
        Fetch the index for the connection
        '''
        return self.__index

    def enable(self):
        '''
        Enable this connection
        '''
        self.enable = True

    def disable(self):
        '''
        Disable this connection
        '''
        self.disable = True

    # The time at which the connection initiates
    departure = property(__get_departure, __set_departure)
    # The time at which the connection terminates
    duration = property(__get_duration, __set_duration)
    # A unique identifier for the connection
    index = property(__get_index, __set_index)

    def update_traversal(self):
        '''
        Update the traversal cost for the edge.
        Traversal cost is the time it takes to travel across this connection
        i.e. traversal_cost(connection) = connection.duration
        '''
        if self.__departure is None or self.__duration is None:
            raise ValueError(
                'Both duration and departure times have to be set before '
                'calculation of traversal cost')

        self.__traversal_cost = self.duration

    def fetch_cost(self, initial, current_cost):
        '''
        Fetch cost incurred after using this edge for transit
        Takes initializtion time of transit and cost incurred until source
        '''
        current_dt = initial + datetime.timedelta(seconds=current_cost)
        current_tm = current_dt.time()
        wait_cost = get_time_delta(current_tm, self.__departure)
        return wait_cost + self.__traversal_cost

    def get_departure_from_arrival(self, arrival):
        '''
        Fetch departure datetime given arrival datetime
        '''
        if type(arrival) is datetime.datetime:
            return arrival + datetime.timedelta(
                seconds=self.__traversal_cost)
        raise TypeError(
            'Expected arrival {} to be datetime.datetime. '
            'Got {} instead'.format(arrival, type(arrival)))


class Marg(AStarVisitor):
    '''
        This class is responsible for storage and traversal
        of the graph during an A* search
    '''

    def __init__(
            self, connections, initial=datetime.datetime.now(), json=False):
        '''
            Initlialize internal graph variables and attributes
            self.graph -> Holds the graph object
            self.vprop_code
            self.eprop_conn
            self.weights = {}
        '''
        # Create a directed graph
        self.graph = Graph()

        # Entry datetime at source vertex
        self.initial = initial

        # Maps user readable vertex string to vertex index
        self.code_vindex_map = {}

        # Maps vertex index to user readable vertex string
        self.vindex_code_map = {}

        # Create an edge property to hold connection information
        self.eprop_connection = self.graph.new_edge_property('object')

        # Create an edge property to hold edge weights
        self.weights = self.graph.new_edge_property('double')

        # A distance map storing distance of vertices from source
        self.dist_map = None

        # A map storing shortest path against vertices from source
        self.path = {}

        if json:
            connections = [Bunch(x) for x in connections]

        # Create vertices and edges from user data
        self._initialize_graph(connections)
        super(Marg, self).__init__()

    def _initialize_graph(self, connections):
        '''
        Initalize graph data from user information
        Arguments:
            Connections: List of connection objects representing edges. Each connection object contains the following attributes
                connection.id:                  Edge Index
                connection.origin:              User readable notation of source vertex
                connection.destination:         User readable notation of destination vertex
                connection.cutoff_departure:    Departure time of connection from source vertex
                connection.cutoff_arrival:      Arrival time of connection at destination vertex

        # noqa
        '''
        uniques = []

        for connection in connections:
            index, source, target, departure, duration = \
                connection.id, connection.origin, connection.destination, \
                connection.cutoff_departure, connection.duration

            if source not in uniques:
                self._add_vertex(source)
                uniques.append(source)

            if target not in uniques:
                self._add_vertex(target)
                uniques.append(target)

            if index not in uniques:
                source = self.code_vindex_map[source]
                target = self.code_vindex_map[target]
                self._add_edge(
                    index, source, target, departure, duration)
                uniques.append(index)

        self._initialize_edge_weights()

    def _add_vertex(self, vertex_code):
        '''
            Add a vertex to graph
        '''
        vertex_code = str(vertex_code)
        vertex = self.graph.add_vertex()
        vindex = self.graph.vertex_index[vertex]

        self.code_vindex_map[vertex_code] = vindex
        self.vindex_code_map[vindex] = vertex_code

    def _add_edge(self, index, source, target, departure, duration):
        '''
            Add an edge to the graph
        '''
        edge = self.graph.add_edge(source, target)

        # Create a connection object to represent this connection
        connection = Connection()
        connection.departure = time_object(departure)
        connection.duration = int(duration)
        connection.index = index
        connection.update_traversal()

        self.eprop_connection[edge] = connection

    def _initialize_edge_weights(self, weight=0.0):
        '''
        Initialize the initial edge weights to weight.
        Weights is a double object reflecting edge weight.
        Defaults to 0.0
        '''
        self.dist_map = self.graph.new_vertex_property('double')
        self.path = {}

        for edge in self.graph.edges():
            self.weights[edge] = weight

    def shortest_path(self, source, t_initial, target=None):
        '''
        Find and return the shortest path from source to target
        with load originating at time t_initial.
        '''
        source = str(source)
        if isinstance(source, str) and source in self.code_vindex_map:
            vindex = self.code_vindex_map[source]
            source = self.graph.vertex(vindex)

        if not isinstance(source, Vertex):
            raise ValueError(
                'Invalid source {} specified. '
                'Does not match list of known vertices'.format(source))

        if target:
            if isinstance(target, str) and target in self.code_vindex_map:
                target = self.graph.vertex(self.code_vindex_map[target])

            if not isinstance(target, Vertex):
                raise ValueError(
                    'Invalid target {} specified. '
                    'Does not match list of known vertices'.format(target))

            return self._shortest_hops(source, target)

        path = {}

        self.initial = t_initial

        # Initialize weights to inf.
        # Update weight when visited/explored
        self._initialize_edge_weights(float('inf'))

        # Shortest path - Min Time
        self.search_shortest(source, self.weights)

        for target in self.graph.vertices():
            if target != source:
                path[
                    self.vindex_code_map[
                        self.graph.vertex_index[target]]] = self._connections(
                            source, target)
            else:
                path[
                    self.vindex_code_map[
                        self.graph.vertex_index[target]
                    ]
                ] = [{
                    'origin': self.vindex_code_map[
                        self.graph.vertex_index[target]
                    ],
                    'destination': self.vindex_code_map[
                        self.graph.vertex_index[target]
                    ],
                    'connection': None,
                    'departure': t_initial,
                    'arrival': t_initial,
                    'cost': 0
                }]
        return path

    def search_shortest(self, source, weights):
        '''
        Perform a shortest path search from source
        '''
        astar_search(self.graph, source, weights, visitor=self)

    def _connections(self, source, target):
        '''
        Generate a list of connections from source to target
        with their timings chosen against the shortest path from
        source to target
        '''
        vertex_pre_target, cost = source, float('inf')
        connection, departure, arrival = (None,)*3

        if target in self.path:
            vertex_pre_target, edge_target = self.path[target]
            connection_target = self.eprop_connection[edge_target]

            cost = self.dist_map[target]
            connection = connection_target.index
            arrival = self.initial + datetime.timedelta(seconds=cost)
            departure = connection_target.get_departure_from_arrival(arrival)

        record = {
            'origin': self.vindex_code_map[
                self.graph.vertex_index[vertex_pre_target]],
            'destination': self.vindex_code_map[
                self.graph.vertex_index[target]],
            'connection': connection,
            'departure': departure,
            'arrival': arrival,
            'cost': cost,
        }

        if vertex_pre_target == source:
            return [record]

        return self._connections(source, vertex_pre_target) + [record]

    def examine_edge(self, edge):
        '''
        Examine the edge to determine edge weight given cumulative cost
        '''
        # Cost incurred to reach source
        cost = self.dist_map[edge.source()]
        # Connection representing edge
        connection = self.eprop_connection[edge]

        # Weight given cumulative costs
        self.weights[edge] = connection.fetch_cost(self.initial, cost)

    def edge_relaxed(self, edge):
        '''
        Relax edge and add to path
        '''
        source = edge.source()
        target = edge.target()

        self.dist_map[target] = self.dist_map[source] + self.weights[edge]
        self.path[target] = source, edge

    def _shortest_hops(self, source, target):
        '''
        Returns a list of shortest path from origin to destination
        based on the number of hops
        '''
        if not isinstance(source, Vertex) or not isinstance(target, Vertex):
            raise ValueError(
                'Invalid source {} specified. '
                'Does not match list of known sources'.format(
                    source))

        path = gt_shortest_path(self.graph, source, target)[0]
        vertices = [
            self.code_vindex_map[
                self.graph.vertex_index[vertex]] for vertex in path]
        return vertices
