import datetime
from graphviz import Digraph


def plot_graph(nodes, name=None):
    waybill = None

    if nodes:
        waybill = nodes[0].wbn

    dot = Digraph(comment='{}'.format(waybill))

    for node in nodes:
        vertex_code = 'NULL'
        edge_name = 'NULL'

        if node.vertex:
            vertex_code = node.vertex.code

        if node.edge:
            edge_name = node.edge.name

        dot.node(vertex_code, edge_name)

    if not name:
        name = '{}_{}'.format(
            waybill, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
        )
    name = 'graphs/{}'.format(name)
    dot.render(name, view=False)
